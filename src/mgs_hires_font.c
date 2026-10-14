#include "mgs_hires_font.h"

#include <stdlib.h>
#include <string.h>

/* The Codec message and Jimaku captions use separate persistent KCBs. */
typedef struct FontContext {
    const void *kcb;
    MgsHiResFontSnapshot pending;
    int supported, ready;
} FontContext;
static FontContext contexts[2];

typedef struct SpriteSnapshot {
    const void *packet;
    MgsHiResFontSnapshot text;
} SpriteSnapshot;

/* Contact names can queue eight rows plus eight selected-row glow copies. */
static SpriteSnapshot sprites[64];
static unsigned queued_sprites;

static int Enabled(void)
{
    const char *atlas = getenv("REI_CODEC_FONT_ATLAS");
    return atlas && *atlas;
}

static FontContext *FindContext(const void *kcb)
{
    if (!kcb)
        return NULL;
    for (unsigned i = 0; i < sizeof(contexts) / sizeof(contexts[0]); ++i)
        if (contexts[i].kcb == kcb)
            return &contexts[i];
    return NULL;
}

void MgsHiResFontAttach(const void *kcb)
{
    if (!Enabled() || !kcb)
        return;
    FontContext *context = FindContext(kcb);
    if (!context) {
        for (unsigned i = 0; i < sizeof(contexts) / sizeof(contexts[0]); ++i)
            if (!contexts[i].kcb) {
                context = &contexts[i];
                break;
            }
    }
    if (!context)
        return;
    memset(context, 0, sizeof(*context));
    context->kcb = kcb;
    context->supported = 1;
}

void MgsHiResFontDetach(const void *kcb)
{
    FontContext *context = FindContext(kcb);
    if (context)
        context->kcb = NULL;
    /* Already queued sprites retain their own snapshots until consumed. */
}

void MgsHiResFontClear(const void *kcb)
{
    FontContext *context = FindContext(kcb);
    if (!context)
        return;
    context->pending.count = 0;
    context->supported = 1;
    context->ready = 0;
}

void MgsHiResFontGlyph(const void *kcb, unsigned code, int x, int y,
                       int color, int width, int y_offset)
{
    FontContext *context = FindContext(kcb);
    if (!context || !context->supported)
        return;
    if (code < 32 || code > 126 || color < 0 || color > 3 ||
        width < 0 || width > 15 || y_offset < 0 || y_offset > 15 ||
        context->pending.count == MGS_HIRES_FONT_MAX_GLYPHS) {
        context->supported = 0;
        return;
    }
    MgsHiResGlyph *glyph = &context->pending.glyphs[context->pending.count++];
    *glyph = (MgsHiResGlyph){(short)x, (short)y,
                             (unsigned char)code, (unsigned char)color,
                             (unsigned char)width, (unsigned char)y_offset};
}

void MgsHiResFontUnsupported(const void *kcb)
{
    FontContext *context = FindContext(kcb);
    if (context)
        context->supported = 0;
}

void MgsHiResFontUpdate(const void *kcb, const void *palette)
{
    FontContext *context = FindContext(kcb);
    if (!context)
        return;
    context->ready = context->supported && context->pending.count != 0 && palette != NULL;
    if (context->ready)
        memcpy(context->pending.palette, palette, sizeof(context->pending.palette));
}

void MgsHiResFontRegisterSprite(const void *packet, const void *kcb,
                                int x, int y, int width, int height)
{
    MgsHiResFontRegisterSpriteRegion(packet, kcb, x, y, width, height, 0, 0);
}

void MgsHiResFontRegisterSpriteRegion(const void *packet, const void *kcb,
                                      int x, int y, int width, int height,
                                      int source_x, int source_y)
{
    FontContext *context = FindContext(kcb);
    if (!context || !context->ready || !packet)
        return;
    for (unsigned i = 0; i < sizeof(sprites) / sizeof(sprites[0]); ++i) {
        if (!sprites[i].packet || sprites[i].packet == packet) {
            if (!sprites[i].packet)
                ++queued_sprites;
            sprites[i].text = context->pending;
            sprites[i].text.x = (short)x;
            sprites[i].text.y = (short)y;
            sprites[i].text.width = (short)width;
            sprites[i].text.height = (short)height;
            sprites[i].text.source_x = (short)source_x;
            sprites[i].text.source_y = (short)source_y;
            sprites[i].packet = packet;
            return;
        }
    }
    /* Never evict an unconsumed packet: the original sprite is safe fallback. */
}

void MgsHiResFontSetSpriteForeColor(const void *packet, unsigned color,
                                    unsigned short fore)
{
    if (!packet || color > 3)
        return;
    for (unsigned i = 0; i < sizeof(sprites) / sizeof(sprites[0]); ++i)
        if (sprites[i].packet == packet) {
            sprites[i].text.palette[color * 4 + 3] = fore;
            return;
        }
}

const MgsHiResFontSnapshot *MgsHiResFontTakeSprite(const void *packet)
{
    if (!packet || !queued_sprites)
        return NULL;
    for (unsigned i = 0; i < sizeof(sprites) / sizeof(sprites[0]); ++i) {
        if (sprites[i].packet == packet) {
            sprites[i].packet = NULL;
            --queued_sprites;
            return &sprites[i].text;
        }
    }
    return NULL;
}

/* A save/load screen may enqueue hundreds of individual block-font sprites.
 * This queue stores metadata only, never game font pixels. Overflow preserves
 * the original packet. Missing atlas leaves registration disabled. */
typedef struct InterfaceGlyph { const void *packet; unsigned char code; unsigned short fore; unsigned uv_clut; } InterfaceGlyph;
static InterfaceGlyph interface_glyphs[1024];
static unsigned interface_count;
/* Only this prefix can contain queued packets. Most boards use far fewer
 * glyphs than the overflow-safe capacity; holes still retain native order. */
static unsigned interface_extent;
static unsigned short interface_fore;
static int interface_palette_valid;
void MgsHiResFontSetInterfacePalette(const unsigned short *palette)
{
    interface_palette_valid = 0;
    if (!palette || palette[0] || !(palette[1] & 0x7fff)) return;
    for (unsigned i = 2; i < 16; ++i)
        if (palette[i] & 0x7fff) return;
    interface_fore = palette[1];
    interface_palette_valid = 1;
}
void MgsHiResFontValidateInterfacePixels(const void *pixels, int width, int height)
{
    /* Source panel45 is a 216x12 4bpp binary alphabet. Unknown/modded indexed
     * artwork using opaque-black or additional color entries keeps its bitmap. */
    if (!pixels || width != 216 || height != 12) {
        interface_palette_valid = 0;
        return;
    }
    const unsigned char *data = pixels;
    for (unsigned i = 0; i < 216u * 12u / 2u; ++i)
        if ((data[i] & 15) > 1 || (data[i] >> 4) > 1) {
            interface_palette_valid = 0;
            return;
        }
}
void MgsHiResFontRegisterInterfaceGlyph(const void *packet, unsigned code, unsigned uv_clut)
{
    const char *atlas = getenv("REI_INTERFACE_FONT_ATLAS");
    if (!packet || !atlas || !*atlas || !interface_palette_valid ||
        !((code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9') ||
          code == '.' || code == ':' || code == '!' || code == '?' || code == '+' || code == '-')) {
        /* Reused packets must not inherit a previously queued supported glyph. */
        MgsHiResFontTakeInterfaceGlyph(packet, NULL, NULL);
        return;
    }
    unsigned empty = interface_extent;
    for (unsigned i = 0; i < interface_extent; ++i) {
        if (interface_glyphs[i].packet == packet) {
            interface_glyphs[i] = (InterfaceGlyph){packet, (unsigned char)code, interface_fore, uv_clut};
            return;
        }
        if (!interface_glyphs[i].packet && empty == interface_extent) empty = i;
    }
    if (empty < 1024) {
        ++interface_count;
        interface_glyphs[empty] = (InterfaceGlyph){packet, (unsigned char)code, interface_fore, uv_clut};
        if (empty == interface_extent) ++interface_extent;
    }
}
unsigned MgsHiResFontTakeInterfaceGlyph(const void *packet, unsigned short *fore, unsigned *uv_clut)
{
    if (!packet || !interface_count) return 0;
    for (unsigned i = 0; i < interface_extent; ++i) {
        if (interface_glyphs[i].packet == packet) {
            unsigned code = interface_glyphs[i].code;
            if (fore) *fore = interface_glyphs[i].fore;
            if (uv_clut) *uv_clut = interface_glyphs[i].uv_clut;
            interface_glyphs[i].packet = NULL;
            if (--interface_count == 0)
                interface_extent = 0;
            return code;
        }
    }
    return 0;
}
