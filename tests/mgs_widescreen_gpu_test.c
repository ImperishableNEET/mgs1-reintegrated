#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libgpu.h>
#include <psyz.h>

/* Standalone renderer fixture has no game title overlay. */
int ReiReintegratedTitleVisible;

static void submit(void *primitive)
{
    termPrim((P_TAG *)primitive);
    DrawOTag((OT_TYPE *)primitive);
}

static void draw_area(int buffer, int origin, int left, int width)
{
    DRAWENV environment;
    SetDefDrawEnv(&environment, buffer + left, 0, width, 224);
    environment.ofs[0] = (short)(buffer + origin);
    environment.ofs[1] = 0;
    environment.dtd = 0;
    PutDrawEnv(&environment);
}

static void tile(int x, int y, int width, int height, int red, int green, int blue, int semi)
{
    TILE primitive = {0};
    setTile(&primitive);
    setSemiTrans(&primitive, semi);
    setXY0(&primitive, x, y);
    setWH(&primitive, width, height);
    setRGB0(&primitive, red, green, blue);
    submit(&primitive);
}

static unsigned short read_pixel(int x, int y)
{
    RECT area = {(short)x, (short)y, 2, 1};
    u_long data = 0;
    StoreImage(&area, &data);
    return (unsigned short)data;
}

static void pixel(int x, int y, unsigned short expected)
{
    unsigned short data = read_pixel(x, y);
    if (data != expected)
        fprintf(stderr, "widescreen pixel (%d,%d): expected %04x, got %04lx\n",
                x, y, expected, (unsigned long)data);
    assert(data == expected);
}

static void clear_frame(int buffer)
{
    RECT area = {(short)buffer, 0, 320, 224};
    assert(Psyz_GpuSetFrameTransform(1, 1, 0, 0) == 0);
    draw_area(buffer, 0, 0, 320);
    ClearImage(&area, 0, 0, 0);
}

/* A one-texel vertical stroke can disappear when a 3/4-width HUD is
 * rasterized directly into the native framebuffer. At 2x it covers at
 * least one sample, so all four distinct texels must survive. */
static void check_hud_thin_stroke(int scale)
{
    assert(Psyz_VideoConfigureWindow(320 * scale, 224 * scale, 0) == 0);
    assert(Psyz_VideoSetPresentation(320, 224, 224) == 0);
    uint16_t colors[4 * 8];
    const uint16_t stripes[4] = {0x001f, 0x03e0, 0x7c00, 0x7fff};
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 4; ++x)
            colors[y * 4 + x] = stripes[x];
    RECT source = {640, 256, 4, 8};
    LoadImage(&source, (u_long *)colors);

    clear_frame(0);
    assert(Psyz_GpuSetTextureAlignment(1) == 0);
    assert(Psyz_GpuSetFrameTransform(3, 4, 160, 1) == 0);
    DR_TPAGE mode = {0};
    setDrawTPage(&mode, 0, 0, getTPage(2, 0, 640, 256));
    submit(&mode);
    SPRT stroke = {0};
    setSprt(&stroke);
    setShadeTex(&stroke, 1);
    setXY0(&stroke, 0, 64);
    setWH(&stroke, 4, 8);
    setUV0(&stroke, 0, 0);
    submit(&stroke);
    DrawSync(0);

    int width, height;
    unsigned char *frame = Psyz_VideoAllocPresentedFrame(&width, &height);
    assert(frame && width == 320 * scale && height == 224 * scale);
    unsigned colors_seen = 0;
    for (int x = 40 * scale; x < 43 * scale; ++x) {
        const unsigned char *rgb = frame + ((size_t)(68 * scale) * width + x) * 3;
        if (rgb[0] > 200 && rgb[1] < 40 && rgb[2] < 40) colors_seen |= 1;
        if (rgb[1] > 200 && rgb[0] < 40 && rgb[2] < 40) colors_seen |= 2;
        if (rgb[2] > 200 && rgb[0] < 40 && rgb[1] < 40) colors_seen |= 4;
        if (rgb[0] > 200 && rgb[1] > 200 && rgb[2] > 200) colors_seen |= 8;
    }
    free(frame);
    printf("HUD thin-stroke colors at %dx: mask=%x\n", scale, colors_seen);
    assert(scale == 2 ? colors_seen == 15 : colors_seen != 15);
    assert(Psyz_GpuSetTextureAlignment(0) == 0);
    assert(Psyz_VideoSetPresentation(16, 9, 224) == 0);
}

static void check_indexed_hud_buffers(void)
{
    enum { scale = 2 };
    uint16_t glyph[8 * 16];
    uint16_t palette[16] = {0, 0x001f, 0x03e0, 0x7c00, 0x7fff};
    for (int y = 0; y < 16; ++y)
        for (int word = 0; word < 8; ++word) {
            uint16_t packed = 0;
            for (int nibble = 0; nibble < 4; ++nibble) {
                int x = word * 4 + nibble;
                unsigned index = (x == 0 || x == 16 || y == 0 || y == 15 ||
                                  (y == 7 && (x < 12 || x >= 16))) ?
                                 (unsigned)(x / 8 + 1) : 0;
                packed |= (uint16_t)(index << (4 * nibble));
            }
            glyph[y * 8 + word] = packed;
        }
    RECT texture = {640, 256, 8, 16};
    RECT clut = {960, 480, 16, 1};
    LoadImage(&texture, (u_long *)glyph);
    LoadImage(&clut, (u_long *)palette);
    assert(Psyz_VideoConfigureWindow(320 * scale, 224 * scale, 0) == 0);
    assert(Psyz_VideoSetPresentation(320, 224, 224) == 0);
    assert(Psyz_GpuSetTextureAlignment(1) == 0);

    unsigned char *frames[2];
    for (int index = 0; index < 2; ++index) {
        int buffer = index * 320;
        clear_frame(buffer);
        assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 1) == 0);
        POLY_FT4 quad = {0};
        setPolyFT4(&quad);
        setShadeTex(&quad, 1);
        setXY4(&quad, 0, 64, 32, 64, 0, 80, 32, 80);
        setUV4(&quad, 0, 0, 32, 0, 0, 16, 32, 16);
        quad.tpage = getTPage(0, 0, 640, 256);
        quad.clut = getClut(960, 480);
        submit(&quad);
        DR_TPAGE mode = {0};
        setDrawTPage(&mode, 0, 0, getTPage(0, 0, 640, 256));
        submit(&mode);
        const int positions[3] = {0, 31, 147};
        const int widths[3] = {8, 17, 32};
        for (int glyph = 0; glyph < 3; ++glyph) {
            SPRT sprite = {0};
            setSprt(&sprite);
            setShadeTex(&sprite, 1);
            setXY0(&sprite, positions[glyph], 96 + glyph * 24);
            setWH(&sprite, widths[glyph], 16);
            setUV0(&sprite, 0, 0);
            setClut(&sprite, 960, 480);
            submit(&sprite);
        }
        DrawSync(0);
        DISPENV display;
        SetDefDispEnv(&display, buffer, 0, 320, 240);
        PutDispEnv(&display);
        int width, height;
        frames[index] = Psyz_VideoAllocPresentedFrame(&width, &height);
        assert(frames[index] && width == 320 * scale && height == 224 * scale);
    }

    unsigned colored = 0, different = 0;
    for (int y = 62 * scale; y < 82 * scale; ++y)
        for (int x = 38 * scale; x < 66 * scale; ++x) {
            size_t offset = ((size_t)y * 320 * scale + x) * 3;
            const unsigned char *a = frames[0] + offset;
            const unsigned char *b = frames[1] + offset;
            colored += a[0] > 100 || a[1] > 100 || a[2] > 100;
            different += memcmp(a, b, 3) != 0;
        }
    unsigned sprite_colored = 0, sprite_different = 0;
    for (int y = 94 * scale; y < 162 * scale; ++y)
        for (int x = 38 * scale; x < 180 * scale; ++x) {
            size_t offset = ((size_t)y * 320 * scale + x) * 3;
            const unsigned char *a = frames[0] + offset;
            const unsigned char *b = frames[1] + offset;
            sprite_colored += a[0] > 100 || a[1] > 100 || a[2] > 100;
            sprite_different += memcmp(a, b, 3) != 0;
        }
    free(frames[0]);
    free(frames[1]);
    printf("Indexed HUD buffer comparison at 2x: quad=%u/%u, sprites=%u/%u colored/different\n",
           colored, different, sprite_colored, sprite_different);
    assert(colored > 0 && different == 0);
    assert(sprite_colored > 0 && sprite_different == 0);
    assert(Psyz_GpuSetTextureAlignment(0) == 0);
    assert(Psyz_VideoSetPresentation(16, 9, 224) == 0);
}

static void check_native_upload_at_2x(void)
{
    enum { scale = 2, x0 = 32, y0 = 32, columns = 16, rows = 4 };
    uint16_t pattern[columns * rows];
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < columns; ++x)
            pattern[y * columns + x] = (x & 1) ? 0 : 0x001f;

    assert(Psyz_VideoConfigureWindow(320 * scale, 224 * scale, 0) == 0);
    assert(Psyz_VideoSetPresentation(320, 224, 224) == 0);
    clear_frame(0);
    RECT region = {x0, y0, columns, rows};
    LoadImage(&region, (u_long *)pattern);
    DrawSync(0);
    DISPENV display;
    SetDefDispEnv(&display, 0, 0, 320, 240);
    PutDispEnv(&display);
    int width, height;
    unsigned char *frame = Psyz_VideoAllocPresentedFrame(&width, &height);
    assert(frame && width == 320 * scale && height == 224 * scale);
    unsigned mismatched = 0;
    for (int y = 0; y < rows * scale; ++y)
        for (int x = 0; x < columns * scale; ++x) {
            const unsigned char *rgb = frame +
                ((size_t)(y0 * scale + y) * width + x0 * scale + x) * 3;
            unsigned expected_red = ((x / scale) & 1) ? 0 : 255;
            mismatched += rgb[0] != expected_red || rgb[1] != 0 || rgb[2] != 0;
        }
    free(frame);
    printf("Native VRAM upload replication at 2x: mismatched=%u/%u\n",
           mismatched, columns * rows * scale * scale);
    assert(mismatched == 0);
    assert(Psyz_VideoSetPresentation(16, 9, 224) == 0);
}

static void check_recovered_glyph_packets(void)
{
    enum { scale = 2 };
    /* radio.c places number glyphs at v=232 and PAUSE letters at v=242
     * on the 4-bit page (960,256). Use synthetic pixels with those exact
     * atlas addresses, sprite sizes, colors, and screen positions. */
    uint16_t atlas[64 * 16] = {0};
    const int pause_uv[5] = {120, 0, 160, 144, 32};
    for (int row = 0; row < 16; ++row) {
        int glyph_width = row < 7 ? 6 : row >= 10 ? 8 : 0;
        if (!glyph_width) continue;
        int count = row < 7 ? 2 : 5;
        for (int glyph = 0; glyph < count; ++glyph) {
            int base = row < 7 ? (glyph ? 224 : 210) : pause_uv[glyph];
            for (int col = 0; col < glyph_width; ++col) {
                if (row != 0 && row != 6 && row != 10 && row != 15 &&
                    col != 0 && col != glyph_width - 1) continue;
                int word = row * 64 + (base + col) / 4;
                atlas[word] |= (uint16_t)((row < 7 ? 1 : 2) <<
                                          (4 * ((base + col) & 3)));
            }
        }
    }
    RECT texture = {960, 488, 64, 16};
    LoadImage(&texture, (u_long *)atlas);
    uint16_t palettes[32] = {0};
    palettes[1] = palettes[16 + 2] = 0x7fff;
    RECT clut = {960, 511, 32, 1};
    LoadImage(&clut, (u_long *)palettes);
    assert(Psyz_VideoConfigureWindow(320 * scale, 224 * scale, 0) == 0);
    assert(Psyz_VideoSetPresentation(320, 224, 224) == 0);
    assert(Psyz_GpuSetTextureAlignment(1) == 0);

    unsigned char *frames[2];
    for (int index = 0; index < 2; ++index) {
        int buffer = index * 320;
        clear_frame(buffer);
        assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 1) == 0);
        DR_TPAGE mode = {0};
        setDrawTPage(&mode, 0, 0, getTPage(0, 0, 960, 256));
        submit(&mode);
        /* jimaku.c: centered PAUSE at (160,80), five 8x6 glyphs,
         * 9-pixel advances; radio.c maps letters to these UVs. */
        for (int glyph = 0; glyph < 5; ++glyph) {
            SPRT sprite = {0};
            setSprt(&sprite);
            setRGB0(&sprite, 200, 200, 200);
            setXY0(&sprite, 138 + glyph * 9, 80);
            setWH(&sprite, 8, 6);
            setUV0(&sprite, pause_uv[glyph], 242);
            setClut(&sprite, 976, 511);
            submit(&sprite);
        }
        /* radio.c: 999/999 uses 6x7 glyphs at u=210, slash u=224,
         * and six-pixel advances. Inventory x is dynamic; exercise all
         * four input-x phases of the 3/4 transform. */
        for (int phase = 0; phase < 4; ++phase)
            for (int glyph = 0; glyph < 7; ++glyph) {
                SPRT sprite = {0};
                setSprt(&sprite);
                setRGB0(&sprite, 87, 87, 87);
                setXY0(&sprite, 100 + phase + glyph * 6, 96 + phase * 20);
                setWH(&sprite, 6, 7);
                setUV0(&sprite, glyph == 3 ? 224 : 210, 232);
                setClut(&sprite, 960, 511);
                submit(&sprite);
            }
        DrawSync(0);
        DISPENV display;
        SetDefDispEnv(&display, buffer, 0, 320, 240);
        PutDispEnv(&display);
        int width, height;
        frames[index] = Psyz_VideoAllocPresentedFrame(&width, &height);
        assert(frames[index] && width == 320 * scale && height == 224 * scale);
    }
    unsigned pause_colored = 0, pause_diff = 0;
    for (int y = 78 * scale; y < 88 * scale; ++y)
        for (int x = 108 * scale; x < 183 * scale; ++x) {
            size_t offset = ((size_t)y * 320 * scale + x) * 3;
            const unsigned char *a = frames[0] + offset;
            const unsigned char *b = frames[1] + offset;
            unsigned colored = a[0] > 100 || a[1] > 100 || a[2] > 100;
            unsigned diff = memcmp(a, b, 3) != 0;
            pause_colored += colored;
            pause_diff += diff;
        }
    unsigned number_colored[4] = {0}, number_diff[4] = {0};
    for (int phase = 0; phase < 4; ++phase)
        for (int y = (94 + phase * 20) * scale; y < (105 + phase * 20) * scale; ++y)
            for (int x = 108 * scale; x < 183 * scale; ++x) {
                size_t offset = ((size_t)y * 320 * scale + x) * 3;
                const unsigned char *a = frames[0] + offset;
                const unsigned char *b = frames[1] + offset;
                unsigned colored = a[0] > 100 || a[1] > 100 || a[2] > 100;
                unsigned diff = memcmp(a, b, 3) != 0;
                if (diff && number_diff[phase] < 2)
                    fprintf(stderr, "number phase%d pixel (%d,%d): %u,%u,%u vs %u,%u,%u\n",
                            phase, x, y, a[0], a[1], a[2], b[0], b[1], b[2]);
                number_colored[phase] += colored;
                number_diff[phase] += diff;
            }
    free(frames[0]);
    free(frames[1]);
    fprintf(stderr, "Recovered glyph packets at 2x: PAUSE=%u/%u; 999/999 phases=%u/%u,%u/%u,%u/%u,%u/%u colored/different\n",
            pause_colored, pause_diff, number_colored[0], number_diff[0],
            number_colored[1], number_diff[1], number_colored[2], number_diff[2],
            number_colored[3], number_diff[3]);
    assert(pause_colored > 0 && pause_diff == 0);
    for (int phase = 0; phase < 4; ++phase)
        assert(number_colored[phase] > 0 && number_diff[phase] == 0);
    assert(Psyz_GpuSetTextureAlignment(0) == 0);
    assert(Psyz_VideoSetPresentation(16, 9, 224) == 0);
}

static void codec_digit(int x, int y, int digit, int large)
{
    static const unsigned char segments[10] =
        {0x77, 0x24, 0x5d, 0x6d, 0x2e, 0x6b, 0x7b, 0x25, 0x7f, 0x6f};
    static const signed char small[7][4] = {
        {1,0,5,2}, {0,1,2,6}, {5,1,2,6}, {1,6,5,2},
        {0,8,2,6}, {5,8,2,6}, {1,13,5,2}};
    static const signed char big[7][4] = {
        {1,0,7,2}, {0,1,2,7}, {7,1,2,7}, {1,7,7,2},
        {0,9,2,7}, {7,9,2,7}, {1,15,7,2}};
    static const signed char shape[7] = {2,4,3,1,4,3,1};
    for (int i = 0; i < 7; ++i) {
        if (!(segments[digit] & (1 << i))) continue;
        const signed char *c = large ? big[i] : small[i];
        LINE_F4 line = {0};
        setLineF4(&line);
        setRGB0(&line, 0x2e, 0x47, 0x3d);
        line.x0 = x + c[0]; line.y0 = y + c[1];
        if (shape[i] < 3) {
            line.x1 = line.x0 + c[2] - 1; line.y1 = line.y0;
            line.x2 = line.x1; line.y2 = line.y0 + c[3] - 1;
            line.x3 = line.x0; line.y3 = line.y2;
            if (shape[i] == 1) { ++line.x0; --line.x1; }
            else { --line.x2; ++line.x3; }
        } else {
            line.x1 = line.x0; line.y1 = line.y0 + c[3] - 1;
            line.x2 = line.x1 + c[2] - 1; line.y2 = line.y1;
            line.x3 = line.x2; line.y3 = line.y0;
            if (shape[i] == 3) { ++line.y0; --line.y1; }
            else { --line.y2; ++line.y3; }
        }
        submit(&line);
    }
}

static void check_codec_frequency_buffers(void)
{
    enum { scale = 2, source_w = 140, source_h = 89 };
    assert(Psyz_VideoConfigureWindow(320 * scale, 224 * scale, 0) == 0);
    assert(Psyz_VideoSetPresentation(320, 224, 224) == 0);
    /* Ordinary Codec uses native VRAM feedback, not CANVAS sampling. */
    assert(Psyz_GpuSetFramebufferSampling(0) == 0);
    assert(Psyz_GpuSetTextureAlignment(1) == 0);
    Psyz_VideoSetDitheringMode(PSYZ_DITHER_AUTO);

    unsigned char *baseline = NULL;
    unsigned failures = 0;
    for (int meter = 6; meter <= 9; ++meter) {
    unsigned char *frames[2];
    uint16_t native[2][source_w * source_h];
    for (int index = 0; index < 2; ++index) {
        int buffer = index * 320;
        clear_frame(buffer);
        assert(Psyz_GpuSetFrameTransform(1, 1, buffer + 160, 0) == 0);
        DR_TPAGE mode = {0};
        setDrawTPage(&mode, 1, 0, getTPage(0, 0, 960, 256));
        submit(&mode);
        Psyz_GpuWriteGP0(0xe6000001u);
        /* Reverse OT execution: source meter and digits precede feedback. */
        for (int row = 1; row < 44; ++row) {
            if (row % 5 == 0 || meter * 5 <= 44 - row) continue;
            int width = 300 / row + 3;
            if (width > 76) width = 76;
            tile(32, 149 + row, width, 1, 0x2e, 0x47, 0x3d, 0);
        }
        const int digits[5] = {1, 4, 0, 8, 5};
        const int xs[5] = {51, 61, 71, 86, 98};
        for (int i = 4; i >= 0; --i)
            codec_digit(xs[i], i >= 2 ? 176 : 178, digits[i], i >= 2);
        tile(82, 191, 2, 2, 0x2e, 0x47, 0x3d, 0);
        RECT native_source = {(short)buffer, 128, source_w, source_h};
        StoreImage(&native_source, (u_long *)native[index]);
        setDrawTPage(&mode, 0, 1, getTPage(2, 1, index * 256, 0));
        submit(&mode);
        /* radio.c helper4: central feedback SPRT plus eight one-pixel
         * neighbors. Preserve reverse OT order, with the center last. */
        for (int copy = 8; copy >= -1; --copy) {
                if (copy == 4) continue;
                int dx = copy < 0 ? 0 : copy % 3 - 1;
                int dy = copy < 0 ? 0 : copy / 3 - 1;
                SPRT glow = {0};
                setSprt(&glow);
                setSemiTrans(&glow, 1);
                int shade = dx == 0 && dy == 0 ? 128 :
                            abs(dx) + abs(dy) == 2 ? 32 : 64;
                setRGB0(&glow, shade, shade, shade);
                setXY0(&glow, 90 + dx, 30 + dy);
                setWH(&glow, source_w, source_h);
                setUV0(&glow, index ? 64 : 0, 128);
                submit(&glow);
        }
        DrawSync(0);
        DISPENV display;
        SetDefDispEnv(&display, buffer, 0, 320, 240);
        PutDispEnv(&display);
        int width, height;
        frames[index] = Psyz_VideoAllocPresentedFrame(&width, &height);
        assert(frames[index] && width == 320 * scale && height == 224 * scale);
    }

    unsigned number_colored = 0, number_diff = 0, glow_colored = 0, glow_diff = 0;
    unsigned speech_diff = 0;
    unsigned native_diff = 0;
    for (int i = 0; i < source_w * source_h; ++i) {
        if (native[0][i] != native[1][i]) {
            if (native_diff < 4)
                fprintf(stderr, "Codec meter%d native (%d,%d): %04x / %04x\n",
                        meter, i % source_w, 128 + i / source_w,
                        native[0][i], native[1][i]);
            ++native_diff;
        }
    }
    for (int y = 28 * scale; y < 198 * scale; ++y)
        for (int x = 48 * scale; x < 232 * scale; ++x) {
            int number = y >= 174 * scale && x < 115 * scale;
            int glow = y < 122 * scale && x >= 88 * scale;
            if (!number && !glow) continue;
            size_t offset = ((size_t)y * 320 * scale + x) * 3;
            const unsigned char *a = frames[0] + offset;
            const unsigned char *b = frames[1] + offset;
            unsigned colored = a[0] > 40 || a[1] > 40 || a[2] > 40;
            unsigned diff = memcmp(a, b, 3) != 0;
            if (number) { number_colored += colored; number_diff += diff; }
            else { glow_colored += colored; glow_diff += diff; }
            if (baseline && y >= 77 * scale && y < 99 * scale &&
                x >= 140 * scale && x < 201 * scale)
                speech_diff += memcmp(a, baseline + offset, 3) != 0;
        }
    if (!baseline) baseline = frames[0];
    else free(frames[0]);
    free(frames[1]);
    fprintf(stderr, "Codec 140.85 at 2x meter%d: lines=%u/%u, feedback=%u/%u colored/different, speechdiff=%u, native=%u\n",
            meter, number_colored, number_diff, glow_colored, glow_diff, speech_diff, native_diff);
    assert(number_colored > 0 && glow_colored > 0);
    failures += number_diff + glow_diff + speech_diff + native_diff;
    }
    free(baseline);
    assert(failures == 0);
    Psyz_GpuWriteGP0(0xe6000000u);
    Psyz_VideoSetDitheringMode(PSYZ_DITHER_OFF);
    assert(Psyz_GpuSetFramebufferSampling(0) == 0);
    assert(Psyz_GpuSetTextureAlignment(0) == 0);
    assert(Psyz_VideoSetPresentation(16, 9, 224) == 0);
}

static void check_world(int buffer, int origin)
{
    clear_frame(buffer);
    draw_area(buffer, origin, 0, 320);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 0) == 0);
    tile(-40-origin, 32, 8, 8, 248, 0, 0, 0);
    tile(0-origin, 32, 8, 8, 0, 248, 0, 0);
    for (int x = 8; x < 18; ++x)
        pixel(buffer+x, 34, x >= 10 && x < 16 ? 0x001f : 0);
    pixel(buffer+42, 34, 0x03e0);
    pixel(buffer+48, 34, 0);
}

static void check_hud_clip_and_restore(int buffer)
{
    clear_frame(buffer);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 0) == 0);
    tile(-40, 32, 8, 8, 248, 0, 0, 0);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 1) == 0);
    draw_area(buffer, 0, 20, 10);
    tile(0, 32, 64, 8, 0, 248, 0, 0);
    draw_area(buffer, 0, 0, 320);
    tile(-32, 52, 8, 8, 248, 0, 0, 0);
    tile(352, 52, 8, 8, 0, 248, 0, 0);
    pixel(buffer+18, 54, 0x001f);
    pixel(buffer+306, 54, 0x03e0);
    assert(Psyz_GpuSetFrameTransform(1, 1, 0, 0) == 0);
    draw_area(buffer, 0, 0, 320);
    tile(100, 32, 8, 8, 0, 0, 248, 0);
    pixel(buffer+12, 34, 0x001f);
    for (int x = 53; x < 65; ++x)
        pixel(buffer+x, 34, x >= 55 && x < 63 ? 0x03e0 : 0);
    pixel(buffer+102, 34, 0x7c00);
    pixel(buffer+115, 34, 0);
}

static void full_frame(int polygon, int semi)
{
    if (!polygon)
    {
        tile(0, 0, 320, 224, 0, 0, semi ? 64 : 128, semi);
        return;
    }
    POLY_F4 primitive = {0};
    setPolyF4(&primitive);
    setSemiTrans(&primitive, semi);
    setRGB0(&primitive, 0, 0, semi ? 64 : 128);
    setXY4(&primitive, 0, 0, 320, 0, 0, 224, 320, 224);
    submit(&primitive);
}

static void check_full_frame(int buffer, int polygon)
{
    clear_frame(buffer);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 1) == 0);
    full_frame(polygon, 0);
    pixel(buffer+2, 100, 0x4000);
    pixel(buffer+160, 100, 0x4000);
    pixel(buffer+317, 100, 0x4000);
    DR_TPAGE mode = {0};
    setDrawTPage(&mode, 0, 0, getTPage(2, 2, 0, 0));
    submit(&mode);
    full_frame(polygon, 1);
    pixel(buffer+2, 100, 0x2000);
    pixel(buffer+160, 100, 0x2000);
    pixel(buffer+317, 100, 0x2000);
    tile(0, 32, 16, 8, 0, 248, 0, 0);
    pixel(buffer+44, 34, 0x03e0);
    pixel(buffer+4, 34, 0x2000);
}

static void check_world_screen_tiles(int buffer, int origin)
{
    clear_frame(buffer);
    DRAWENV environment;
    SetDefDrawEnv(&environment, buffer, 16, 320, 192);
    environment.ofs[0] = (short)(buffer + origin);
    environment.ofs[1] = 112;
    environment.dtd = 0;
    PutDrawEnv(&environment);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 0) == 0);
    tile(-origin, -112, 320, 224, 0, 0, 128, 0);
    DR_TPAGE mode = {0};
    setDrawTPage(&mode, 0, 1, getTPage(0, 1, 0, 0));
    submit(&mode);
    tile(-origin, -112, 320, 224, 64, 16, 0, 1);
    for (int x = 2; x <= 318; x += 158) {
        pixel(buffer + x, 100, 0x4048);
        pixel(buffer + x, 14, 0);
        pixel(buffer + x, 210, 0);
    }
    tile(-origin, -80, 320, 1, 0, 0, 0, 0);
    pixel(buffer + 2, 32, 0);
    pixel(buffer + 317, 32, 0);
    pixel(buffer + 2, 33, 0x4048);
    tile(-origin, -64, 16, 8, 0, 248, 0, 0);
    pixel(buffer + 44, 50, 0x03e0);
    pixel(buffer + 4, 50, 0x4048);
    tile(-origin, 16, 319, 8, 0, 0, 0, 0);
    pixel(buffer + 160, 130, 0);
    pixel(buffer + 4, 130, 0x4048);
}

static void check_world_polygon_and_partial_area(int buffer)
{
    clear_frame(buffer);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 0) == 0);
    full_frame(1, 0);
    pixel(buffer + 2, 100, 0);
    pixel(buffer + 160, 100, 0x4000);
    pixel(buffer + 317, 100, 0);
    draw_area(buffer, 0, 0, 300);
    tile(0, 32, 320, 8, 0, 248, 0, 0);
    pixel(buffer + 2, 34, 0);
    pixel(buffer + 44, 34, 0x03e0);
    pixel(buffer + 290, 34, 0);
}

static void flat_band(int polygon, int y, int height, int blue, int semi)
{
    if (!polygon) {
        tile(0, y, 320, height, 0, 0, blue, semi);
        return;
    }
    POLY_F4 band = {0};
    setPolyF4(&band);
    setSemiTrans(&band, semi);
    setRGB0(&band, 0, 0, blue);
    setXY4(&band, 0, y, 320, y, 0, y + height, 320, y + height);
    submit(&band);
}

static void check_bands(int buffer, int polygon)
{
    clear_frame(buffer);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 1) == 0);
    full_frame(polygon, 0);
    flat_band(polygon, -8, 16, 0, 0);
    pixel(buffer + 2, 4, 0);
    pixel(buffer + 2, 12, 0x4000);
    flat_band(polygon, 0, 24, 0, 0);
    flat_band(polygon, 184, 40, 0, 0);
    DR_TPAGE mode = {0};
    setDrawTPage(&mode, 0, 0, getTPage(2, 2, 0, 0));
    submit(&mode);
    flat_band(polygon, 96, 16, 64, 1);
    for (int x = 2; x <= 318; x += 158) {
        pixel(buffer + x, 12, 0);
        pixel(buffer + x, 200, 0);
        pixel(buffer + x, 80, 0x4000);
        pixel(buffer + x, 104, 0x2000);
    }
    tile(0, 40, 16, 8, 0, 248, 0, 0);
    pixel(buffer + 44, 42, 0x03e0);
    pixel(buffer + 4, 42, 0x4000);
}

static void check_gradient_bands(int buffer)
{
    clear_frame(buffer);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 1) == 0);
    POLY_G4 band = {0};
    setPolyG4(&band);
    setXY4(&band, 0, 32, 320, 32, 0, 64, 320, 64);
    setRGB0(&band, 248, 0, 0);
    setRGB1(&band, 248, 0, 0);
    setRGB2(&band, 0, 0, 0);
    setRGB3(&band, 0, 0, 0);
    submit(&band);
    unsigned short upper = read_pixel(buffer + 160, 40);
    unsigned short lower = read_pixel(buffer + 160, 56);
    assert(upper > lower && lower > 0);
    pixel(buffer + 2, 40, upper);
    pixel(buffer + 317, 40, upper);
    pixel(buffer + 2, 56, lower);
    pixel(buffer + 317, 56, lower);
    pixel(buffer + 2, 30, 0);
    setXY4(&band, 0, 96, 320, 96, 0, 128, 320, 128);
    setRGB1(&band, 0, 0, 0);
    setRGB2(&band, 248, 0, 0);
    submit(&band);
    pixel(buffer + 2, 104, 0);
    assert(read_pixel(buffer + 160, 104) > 0);
}

static void copy_framebuffer_half(int sprite, int x, int page, int u)
{
    if (sprite) {
        DR_TPAGE mode = {0};
        setDrawTPage(&mode, 0, 0, getTPage(2, 0, page, 256));
        submit(&mode);
        SPRT copy = {0};
        setSprt(&copy);
        setShadeTex(&copy, 1);
        setXY0(&copy, x, 32);
        setWH(&copy, 160, 64);
        setUV0(&copy, u, 0);
        submit(&copy);
        return;
    }
    POLY_FT4 copy = {0};
    setPolyFT4(&copy);
    setShadeTex(&copy, 1);
    setXY4(&copy, x, 32, x + 160, 32, x, 96, x + 160, 96);
    setUV4(&copy, u, 0, u + 160, 0, u, 64, u + 160, 64);
    copy.tpage = getTPage(2, 0, page, 256);
    submit(&copy);
}

static void check_framebuffer_copy(int buffer, int sprite)
{
    clear_frame(buffer);
    DRAWENV source;
    SetDefDrawEnv(&source, 640, 256, 320, 64);
    source.dtd = 0;
    PutDrawEnv(&source);
    tile(0, 0, 160, 64, 248, 0, 0, 0);
    tile(160, 0, 160, 64, 0, 248, 0, 0);
    draw_area(buffer, 0, 0, 320);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 1) == 0);
    assert(Psyz_GpuSetFrameTransform(1, 1, 0, 0) == 0);
    copy_framebuffer_half(sprite, 0, 640, 0);
    copy_framebuffer_half(sprite, 160, 768, 32);
    pixel(buffer + 2, 40, 0x001f);
    pixel(buffer + 150, 40, 0x001f);
    pixel(buffer + 170, 40, 0x03e0);
    pixel(buffer + 317, 40, 0x03e0);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 0) == 0);
    tile(-40, 112, 8, 8, 248, 0, 0, 0);
    pixel(buffer + 12, 114, 0x001f);
    assert(Psyz_GpuSetFrameTransform(3, 4, buffer + 160, 1) == 0);
    tile(0, 128, 16, 8, 0, 0, 248, 0);
    pixel(buffer + 44, 130, 0x7c00);
    pixel(buffer + 4, 130, 0);
}

static void check_grid_and_transfers(void)
{
    clear_frame(0);
    assert(Psyz_GpuSetHorizontalGrid(2, 1) == 0);
    assert(Psyz_GpuSetFrameTransform(3, 4, 160, 0) == 0);
    tile(80, 80, 32, 8, 248, 0, 0, 0);
    pixel(72, 82, 0x001f);
    pixel(83, 82, 0);
    RECT transfer = {900, 300, 2, 1};
    u_long colors = 0x03e0801f;
    LoadImage(&transfer, &colors);
    pixel(900, 300, 0x801f);
    pixel(901, 300, 0x03e0);
    assert(Psyz_GpuSetHorizontalGrid(1, 1) == 0);
    assert(Psyz_GpuSetFrameTransform(1, 1, 0, 0) == 0);
    DRAWENV offscreen;
    SetDefDrawEnv(&offscreen, 800, 300, 32, 32);
    PutDrawEnv(&offscreen);
    tile(4, 4, 8, 8, 0, 248, 0, 0);
    pixel(806, 306, 0x03e0);
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    int scale = argc == 1 ? 1 : strcmp(argv[1], "scaled") == 0 ? 2 :
                strcmp(argv[1], "8x") == 0 ? 8 : 0;
    assert(argc <= 2 && scale != 0);
    assert(Psyz_VideoConfigureWindow(800, 450, 0) == 0);
    assert(Psyz_VideoSetPresentation(16, 9, 224) == 0);
    assert(Psyz_VideoSetInternalResolution(scale) == 0);
    Psyz_VideoSetDitheringMode(PSYZ_DITHER_OFF);
    ResetGraph(0);
    DISPENV display;
    SetDefDispEnv(&display, 0, 0, 320, 240);
    PutDispEnv(&display);
    SetDispMask(1);
    DrawSync(0);
    if (scale <= 2) check_hud_thin_stroke(scale);
    if (scale == 2) check_indexed_hud_buffers();
    if (scale == 2) check_native_upload_at_2x();
    if (scale == 2) check_recovered_glyph_packets();
    if (scale == 2) check_codec_frequency_buffers();
    for (int buffer = 0; buffer <= 320; buffer += 320)
    {
        check_world(buffer, 0);
        check_world(buffer, 160);
        check_hud_clip_and_restore(buffer);
        check_full_frame(buffer, 0);
        check_full_frame(buffer, 1);
        check_world_screen_tiles(buffer, 0);
        check_world_screen_tiles(buffer, 160);
        check_world_polygon_and_partial_area(buffer);
        check_bands(buffer, 0);
        check_bands(buffer, 1);
        check_gradient_bands(buffer);
        check_framebuffer_copy(buffer, 0);
        check_framebuffer_copy(buffer, 1);
    }
    check_grid_and_transfers();
    printf("Widescreen GPU %dx: wider world, HUD clipping, both buffers, fades and raw transfers passed\n",
           scale);
    return 0;
}
