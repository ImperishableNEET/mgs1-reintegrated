#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgpu.h>
#include <psyz.h>
#include "mgs_hires_font.h"

int ReiReintegratedTitleVisible;

static void write_u16(FILE *file, unsigned value)
{
    fputc(value & 255, file);
    fputc((value >> 8) & 255, file);
}

static void write_u32(FILE *file, unsigned value)
{
    write_u16(file, value);
    write_u16(file, value >> 16);
}

static void write_test_atlas(const char *path)
{
    enum { width = 2048, height = 672, row_bytes = width * 3 };
    FILE *file = fopen(path, "wb");
    assert(file);
    fputc('B', file);
    fputc('M', file);
    write_u32(file, 54 + row_bytes * height);
    write_u32(file, 0);
    write_u32(file, 54);
    write_u32(file, 40);
    write_u32(file, width);
    write_u32(file, height);
    write_u16(file, 1);
    write_u16(file, 24);
    write_u32(file, 0);
    write_u32(file, row_bytes * height);
    write_u32(file, 2835);
    write_u32(file, 2835);
    write_u32(file, 0);
    write_u32(file, 0);

    /* 'A' is glyph 33, column 1 / row 2. Draw a 4x8 body and AA edge. */
    unsigned char row[row_bytes];
    for (int bottom = 0; bottom < height; ++bottom) {
        int y = height - 1 - bottom;
        memset(row, 0, sizeof(row));
        if (y >= 2 * 112 + 2 * 8 && y < 2 * 112 + 10 * 8) {
            for (int x = 128 + 2 * 8; x < 128 + 6 * 8; ++x)
                memset(row + x * 3, 255, 3);
            for (int x = 128 + 6 * 8; x < 128 + 7 * 8; ++x)
                memset(row + x * 3, 128, 3);
        }
        assert(fwrite(row, 1, sizeof(row), file) == sizeof(row));
    }
    assert(fclose(file) == 0);
}

static void submit(void *primitive)
{
    termPrim((P_TAG *)primitive);
    DrawOTag((OT_TYPE *)primitive);
}

static unsigned short read_pixel(int x, int y)
{
    RECT area = {(short)x, (short)y, 2, 1};
    u_long data = 0;
    StoreImage(&area, &data);
    return (unsigned short)data;
}

static void expect_pixel(int x, int y, unsigned short expected)
{
    unsigned short actual = read_pixel(x, y);
    if (actual != expected)
        fprintf(stderr, "Codec font pixel (%d,%d): %04x expected %04x\n",
                x, y, actual, expected);
    assert(actual == expected);
}

static void message_sprite(SPRT *sprite, int y)
{
    memset(sprite, 0, sizeof(*sprite));
    setSprt(sprite);
    setRGB0(sprite, 128, 128, 128);
    setXY0(sprite, 20, y);
    setWH(sprite, 252, 76);
    setUV0(sprite, 0, 4);
    setClut(sprite, 960, 510);
}

static void draw_mask(int force, int preserve)
{
    DR_TPAGE packet = {0};
    setlen(&packet, 1);
    packet.code[0] = 0xe6000000u | (unsigned)force | ((unsigned)preserve << 1);
    submit(&packet);
}

static void tag_message(SPRT *sprite, int *codec_kcb,
                        const unsigned short *palette)
{
    MgsHiResFontClear(codec_kcb);
    MgsHiResFontGlyph(codec_kcb, 'A', 0, 0, 0, 8, 0);
    MgsHiResFontUpdate(codec_kcb, palette);
    MgsHiResFontRegisterSprite(&sprite->r0, codec_kcb, sprite->x0,
                                sprite->y0, sprite->w, sprite->h);
}

int main(int argc, char **argv)
{
    int scale = argc == 1 ? 2 : argc == 2 && strcmp(argv[1], "8x") == 0 ? 8 : 0;
    assert(scale != 0);
    const char *atlas = scale == 8 ? "mgs_hires_font_gpu_fixture_8x.bmp"
                                   : "mgs_hires_font_gpu_fixture_2x.bmp";
    write_test_atlas(atlas);
#ifdef _WIN32
    assert(_putenv_s("REI_CODEC_FONT_ATLAS", atlas) == 0);
#else
    assert(setenv("REI_CODEC_FONT_ATLAS", atlas, 1) == 0);
#endif
    assert(Psyz_VideoConfigureWindow(320 * scale, 224 * scale, 0) == 0);
    assert(Psyz_VideoSetPresentation(320, 224, 224) == 0);
    assert(Psyz_VideoSetInternalResolution(scale) == 0);
    ResetGraph(0);
    DISPENV display;
    SetDefDispEnv(&display, 0, 0, 320, 240);
    PutDispEnv(&display);
    SetDispMask(1);

    DRAWENV draw;
    SetDefDrawEnv(&draw, 0, 0, 320, 224);
    draw.dtd = 0;
    PutDrawEnv(&draw);
    RECT frame = {0, 0, 320, 224};
    ClearImage(&frame, 0, 0, 0);

    uint16_t texture[63 * 76] = {0};
    for (int y = 20; y < 28; ++y)
        for (int x = 40; x < 44; ++x)
            texture[y * 63 + x / 4] |= (uint16_t)(3u << ((x % 4) * 4));
    unsigned short palette[16] = {0};
    palette[3] = 0x001f;
    RECT texture_rect = {960, 260, 63, 76};
    RECT clut_rect = {960, 510, 16, 1};
    LoadImage(&texture_rect, (u_long *)texture);
    LoadImage(&clut_rect, (u_long *)palette);
    DR_TPAGE mode = {0};
    setDrawTPage(&mode, 0, 0, getTPage(0, 0, 960, 256));
    submit(&mode);

    int codec_kcb;
    MgsHiResFontAttach(&codec_kcb);
    SPRT enhanced;
    message_sprite(&enhanced, 20);
    tag_message(&enhanced, &codec_kcb, palette);
    submit(&enhanced);
    DrawSync(0);
    expect_pixel(24, 25, 0x001f); /* authored high-resolution glyph */
    expect_pixel(62, 42, 0);      /* native sprite was suppressed */
    unsigned short aa = read_pixel(26, 25);
    assert((aa & 31) > 0 && (aa & 31) < 31);

    MgsHiResFontClear(&codec_kcb);
    MgsHiResFontGlyph(&codec_kcb, 'A', 0, 0, 0, 8, 0);
    MgsHiResFontUnsupported(&codec_kcb); /* mixed/unsupported board */
    MgsHiResFontUpdate(&codec_kcb, palette);
    SPRT original;
    message_sprite(&original, 110);
    MgsHiResFontRegisterSprite(&original.r0, &codec_kcb, original.x0,
                                original.y0, original.w, original.h);
    submit(&original);
    DrawSync(0);
    expect_pixel(24, 115, 0);     /* atlas was not used without a tag */
    expect_pixel(62, 132, 0x001f); /* original 4-bpp bitmap still draws */

    draw_mask(1, 0);
    SPRT masked;
    message_sprite(&masked, 60);
    tag_message(&masked, &codec_kcb, palette);
    submit(&masked);
    DrawSync(0);
    expect_pixel(24, 65, 0x801f); /* high-res path keeps the PS1 mask bit */
    draw_mask(0, 1);
    SPRT preserved;
    message_sprite(&preserved, 150);
    tag_message(&preserved, &codec_kcb, palette);
    submit(&preserved);
    DrawSync(0);
    expect_pixel(24, 155, 0);      /* preserve-masked mode falls back */
    expect_pixel(62, 172, 0x001f); /* original sprite survives fallback */
    draw_mask(0, 0);

    TILE cover = {0};
    setTile(&cover);
    setXY0(&cover, 22, 23);
    setWH(&cover, 8, 8);
    setRGB0(&cover, 0, 0, 248);
    submit(&cover);               /* a later OT must cover the glyph */
    DrawSync(0);
    expect_pixel(24, 25, 0x7c00);

    /* Contact names are cropped 96x12 rows of the shared Codec font VRAM. */
    SPRT contact = {0};
    setSprt(&contact);
    setRGB0(&contact, 128, 128, 128);
    setXY0(&contact, 120, 80);
    setWH(&contact, 96, 12);
    setUV0(&contact, 96, 16);
    setClut(&contact, 960, 510);
    MgsHiResFontClear(&codec_kcb);
    MgsHiResFontGlyph(&codec_kcb, 'A', 98, 12, 0, 8, 0);
    MgsHiResFontGlyph(&codec_kcb, 'A', 98, 24, 0, 8, 0); /* next name row */
    MgsHiResFontGlyph(&codec_kcb, 'A', 2, 12, 0, 8, 0);  /* other column */
    MgsHiResFontUpdate(&codec_kcb, palette);
    MgsHiResFontRegisterSpriteRegion(&contact.r0, &codec_kcb,
        contact.x0, contact.y0, contact.w, contact.h,
        contact.u0, contact.v0 - 4);
    submit(&contact);
    DrawSync(0);
    expect_pixel(124, 84, 0x001f);
    expect_pixel(124, 89, 0x001f); /* bottom of the intended row is retained */
    expect_pixel(124, 91, 0);      /* next row cannot bleed into this sprite */
    expect_pixel(122, 84, 0); /* source crop moves x=98 to screen x=122 */

    TILE blue = {0};
    setTile(&blue);
    setXY0(&blue, 120, 100);
    setWH(&blue, 96, 12);
    setRGB0(&blue, 0, 0, 248);
    submit(&blue);
    SPRT selected = contact;
    selected.y0 = 100;
    setRGB0(&selected, 192, 192, 192);
    setSemiTrans(&selected, 1); /* selected contact uses additive semitransparency */
    MgsHiResFontRegisterSpriteRegion(&selected.r0, &codec_kcb,
        selected.x0, selected.y0, selected.w, selected.h,
        selected.u0, selected.v0 - 4);
    submit(&selected);
    DrawSync(0);
    expect_pixel(124, 104, 0x7c1f); /* atlas red adds to existing blue */

    /* Save/load labels use 14-pixel rows, with native UV four pixels lower. */
    SPRT save_row = {0};
    setSprt(&save_row);
    setRGB0(&save_row, 128, 128, 128);
    setXY0(&save_row, 20, 120);
    setWH(&save_row, 128, 14);
    setUV0(&save_row, 0, 18);
    setClut(&save_row, 960, 510);
    MgsHiResFontClear(&codec_kcb);
    MgsHiResFontGlyph(&codec_kcb, 'A', 0, 14, 0, 8, 0);
    MgsHiResFontGlyph(&codec_kcb, 'A', 0, 28, 0, 8, 0);
    MgsHiResFontGlyph(&codec_kcb, 'A', 128, 14, 0, 8, 0);
    MgsHiResFontUpdate(&codec_kcb, palette);
    MgsHiResFontRegisterSpriteRegion(&save_row.r0, &codec_kcb,
        save_row.x0, save_row.y0, save_row.w, save_row.h,
        save_row.u0, save_row.v0 - 4);
    submit(&save_row);
    DrawSync(0);
    expect_pixel(24, 129, 0x001f); /* entire cap stays inside its row */
    expect_pixel(24, 133, 0);      /* adjacent row and column stay excluded */

    MgsHiResFontDetach(&codec_kcb);
    assert(remove(atlas) == 0);
    printf("Codec high-resolution atlas %dx: replacement, contact and save crops, additive, AA, mask, fallback and OT order passed\n", scale);
    return 0;
}
