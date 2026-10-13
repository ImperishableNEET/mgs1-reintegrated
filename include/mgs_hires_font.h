#ifndef MGS_HIRES_FONT_H
#define MGS_HIRES_FONT_H

#define MGS_HIRES_FONT_MAX_GLYPHS 256

typedef struct MgsHiResGlyph {
    short x, y;
    unsigned char code, color, width, y_offset;
} MgsHiResGlyph;

typedef struct MgsHiResFontSnapshot {
    short x, y, width, height;
    short source_x, source_y;
    unsigned short palette[16];
    unsigned short count;
    MgsHiResGlyph glyphs[MGS_HIRES_FONT_MAX_GLYPHS];
} MgsHiResFontSnapshot;

/* Custom interface sprites retain their original packet coordinates/spacing. */
void MgsHiResFontSetInterfacePalette(const unsigned short *palette);
void MgsHiResFontValidateInterfacePixels(const void *pixels, int width, int height);
void MgsHiResFontRegisterInterfaceGlyph(const void *packet, unsigned code, unsigned uv_clut);
unsigned MgsHiResFontTakeInterfaceGlyph(const void *packet, unsigned short *fore, unsigned *uv_clut);

/* The renderer consumes snapshots by packet identity at Draw_PushPrim time. */
void MgsHiResFontAttach(const void *kcb);
void MgsHiResFontDetach(const void *kcb);
void MgsHiResFontClear(const void *kcb);
void MgsHiResFontGlyph(const void *kcb, unsigned code, int x, int y,
                       int color, int width, int y_offset);
void MgsHiResFontUnsupported(const void *kcb);
void MgsHiResFontUpdate(const void *kcb, const void *palette);
void MgsHiResFontRegisterSprite(const void *packet, const void *kcb,
                                int x, int y, int width, int height);
void MgsHiResFontRegisterSpriteRegion(const void *packet, const void *kcb,
                                      int x, int y, int width, int height,
                                      int source_x, int source_y);
/* source_x/y select the captured glyph row/column, which may differ from
 * the native sprite UV when that sprite crops part of its bitmap glyphs. */
void MgsHiResFontSetSpriteForeColor(const void *packet, unsigned color,
                                    unsigned short fore);
const MgsHiResFontSnapshot *MgsHiResFontTakeSprite(const void *packet);

#endif
