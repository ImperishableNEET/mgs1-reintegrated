#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgpu.h>
#include <psyz.h>

int ReiReintegratedTitleVisible;

static const void *addresses[4];
static PsyzProjectedVertex projected[4];
static int missing_vertex = -1;
static int geometry_flags = PSYZ_GEOMETRY_VALID;
static unsigned scale;

static int Geometry(const void *xy, PsyzProjectedVertex *vertex)
{
    for (int i = 0; i < 4; ++i)
        if (xy == addresses[i] && i != missing_vertex) {
            *vertex = projected[i];
            return geometry_flags;
        }
    return 0;
}

static void RegisterQuad(short *x0, short *x1, short *x2, short *x3, float left, float right, float offset)
{
    short *xy[4] = {x0, x1, x2, x3};
    for (int i = 0; i < 4; ++i) {
        addresses[i] = xy[i];
        projected[i] = (PsyzProjectedVertex){xy[i][0] + offset, xy[i][1], i & 1 ? right : left};
    }
}

static void Submit(void *packet)
{
    termPrim((P_TAG *)packet);
    DrawOTag((OT_TYPE *)packet);
}

static void Clear(void)
{
    RECT area = {0, 0, 320, 224};
    ClearImage(&area, 0, 0, 0);
    Psyz_GpuResetGeometryDepth();
}

static unsigned char *Capture(void)
{
    DrawSync(0);
    int width, height;
    unsigned char *image = Psyz_VideoAllocPresentedFrame(&width, &height);
    assert(image && width == 320 * scale && height == 224 * scale);
    return image;
}

static void Solid(float depth, int red, int green, int semi, float offset)
{
    POLY_F4 quad = {0};
    setPolyF4(&quad);
    setXYWH(&quad, 16, 16, 128, 128);
    setRGB0(&quad, red, green, 0);
    setSemiTrans(&quad, semi);
    RegisterQuad(&quad.x0, &quad.x1, &quad.x2, &quad.x3, depth, depth, offset);
    Submit(&quad);
}

static void CheckDepth(void)
{
    for (int enabled = 0; enabled < 2; ++enabled) {
        assert(Psyz_GpuConfigureGeometry(Geometry, 0, 0, enabled) == 0);
        Clear();
        Solid(100, 255, 0, 0, 0);
        Solid(400, 0, 255, 0, 0);
        unsigned char *image = Capture();
        const unsigned char *pixel = image + (80 * scale * 320 * scale + 80 * scale) * 3;
        assert(pixel[enabled ? 0 : 1] > 240 && pixel[enabled ? 1 : 0] == 0);
        free(image);
    }
    Solid(500, 0, 255, 1, 0);
    unsigned char *image = Capture();
    assert(image[(80 * scale * 320 * scale + 80 * scale) * 3] > 240);
    free(image);
    Clear();
    Solid(400, 0, 255, 0, 0);
    image = Capture();
    assert(image[(80 * scale * 320 * scale + 80 * scale) * 3 + 1] > 240);
    free(image);
}

static unsigned SampleTexture(int correction, int missing)
{
    assert(Psyz_GpuConfigureGeometry(Geometry, correction, 0, 0) == 0);
    Clear();
    POLY_FT4 quad = {0};
    setPolyFT4(&quad);
    setShadeTex(&quad, 1);
    setXYWH(&quad, 16, 16, 128, 128);
    setUVWH(&quad, 0, 0, 64, 64);
    quad.tpage = getTPage(2, 0, 640, 256);
    RegisterQuad(&quad.x0, &quad.x1, &quad.x2, &quad.x3, 100, 400, 0);
    missing_vertex = missing;
    Submit(&quad);
    missing_vertex = -1;
    unsigned char *image = Capture();
    unsigned value = image[(80 * scale * 320 * scale + 80 * scale) * 3 + 1];
    free(image);
    return value;
}

static void CheckTextures(void)
{
    uint16_t pixels[64 * 64];
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            pixels[y * 64 + x] = (uint16_t)(1 | ((x / 2) << 5));
    RECT texture = {640, 256, 64, 64};
    LoadImage(&texture, (u_long *)pixels);
    unsigned affine = SampleTexture(0, -1);
    unsigned perspective = SampleTexture(1, -1);
    assert(affine >= 115 && affine <= 140);
    assert(perspective >= 40 && perspective <= 60);
    assert(SampleTexture(1, 2) == affine);
}

static void CheckFramebufferTextureAffine(void)
{
    uint16_t pixels[320 * 224];
    for (unsigned y = 0; y < 224; ++y)
        for (unsigned x = 0; x < 320; ++x)
            pixels[y * 320 + x] = (uint16_t)(1u << 10 | (x / 10u) << 5 | y / 8u);
    DRAWENV centered, ordinary;
    SetDefDrawEnv(&centered, 0, 0, 320, 224);
    centered.ofs[0] = 160; centered.ofs[1] = 112; centered.dtd = 0;
    SetDefDrawEnv(&ordinary, 0, 0, 320, 224); ordinary.dtd = 0;
    for (unsigned half = 0; half < 2; ++half)
        for (unsigned shifted = 0; shifted < 2; ++shifted)
            for (int depth = 0; depth < 2; ++depth) {
                unsigned char *affine = NULL;
                for (int correction = 0; correction < 2; ++correction) {
                    assert(Psyz_GpuConfigureGeometry(Geometry, correction, 1, depth) == 0);
                    PutDrawEnv(&ordinary);
                    Clear();
                    RECT front = {(short)(half * 320), 0, 320, 224};
                    LoadImage(&front, (u_long *)pixels);
                    PutDrawEnv(&centered);
                    POLY_FT4 quad = {0};
                    setPolyFT4(&quad); setShadeTex(&quad, 1);
                    setXYWH(&quad, shifted ? 16 : -80, -32, 128, 64);
                    short *xy[] = {&quad.x0, &quad.x1, &quad.x2, &quad.x3};
                    unsigned char *uv[] = {&quad.u0, &quad.u1, &quad.u2, &quad.u3};
                    for (unsigned i = 0; i < 4; ++i) {
                        /* Same screen-derived mapping as EQ_VisibleUnit2. */
                        uv[i][0] = (unsigned char)(xy[i][0] - (xy[i][0] / 4 - 160) - shifted * 64);
                        uv[i][1] = (unsigned char)(xy[i][1] + 112);
                    }
                    quad.tpage = getTPage(2, 0, half * 320 + shifted * 64, 0);
                    RegisterQuad(&quad.x0, &quad.x1, &quad.x2, &quad.x3, 100, 400, 0.375f);
                    Submit(&quad);
                    unsigned char *image = Capture();
                    if (!correction) affine = image;
                    else {
                        assert(memcmp(affine, image, (size_t)320 * 224 * scale * scale * 3) == 0);
                        free(image);
                        free(affine);
                    }
                }
            }
    PutDrawEnv(&ordinary);
    puts("Framebuffer-derived model UVs: four source pages remain affine with texture correction, subpixels and depth on/off.");
}

static void CheckHighResolutionFramebufferTexture(void)
{
    if (scale != 4) return;
    for (int depth = 0; depth < 2; ++depth) {
        assert(Psyz_GpuConfigureGeometry(Geometry, 1, 1, depth) == 0);
        Clear();
        RECT green = {0, 0, 320, 224};
        ClearImage(&green, 0, 248, 0);
        DrawSync(0);
        Psyz_GpuWriteGP0(0xe6000001u); DrawSync(0);
        POLY_F4 source = {0};
        setPolyF4(&source); setRGB0(&source, 248, 0, 0);
        setXYWH(&source, 64, 64, 1, 32);
        RegisterQuad(&source.x0, &source.x1, &source.x2, &source.x3, 1000, 1000, 0);
        projected[1].x -= 0.5f; projected[3].x -= 0.5f;
        Submit(&source); DrawSync(0);
        Psyz_GpuWriteGP0(0xe6000000u); DrawSync(0);
        POLY_FT4 copy = {0};
        setPolyFT4(&copy); setShadeTex(&copy, 1);
        setXYWH(&copy, 160, 64, 1, 32); setUVWH(&copy, 64, 64, 1, 32);
        copy.tpage = getTPage(2, 0, 0, 0);
        RegisterQuad(&copy.x0, &copy.x1, &copy.x2, &copy.x3, 100, 400, 0);
        Submit(&copy);
        /* A following untextured batch must flush the source snapshot first. */
        Solid(50, 0, 0, 0, 0);
        uint16_t indices[16 * 16], palette[16] = {0, 0x7c00};
        for (unsigned i = 0; i < 16 * 16; ++i) indices[i] = 0x1111;
        RECT indexed_texture = {768, 256, 16, 16}, indexed_palette = {960, 480, 16, 1};
        LoadImage(&indexed_texture, (u_long *)indices);
        LoadImage(&indexed_palette, (u_long *)palette);
        POLY_FT4 indexed = {0};
        setPolyFT4(&indexed); setShadeTex(&indexed, 1);
        setXYWH(&indexed, 200, 80, 16, 16); setUVWH(&indexed, 0, 0, 16, 16);
        indexed.tpage = getTPage(0, 0, 768, 256); setClut(&indexed, 960, 480);
        RegisterQuad(&indexed.x0, &indexed.x1, &indexed.x2, &indexed.x3, 100, 100, 0);
        Submit(&indexed);
        unsigned char *image = Capture();
        unsigned reds = 0, greens = 0;
        for (unsigned x = 160 * scale; x < 161 * scale; ++x) {
            const unsigned char *pixel = image + (80 * scale * 320 * scale + x) * 3;
            reds += pixel[0] > 240 && pixel[1] == 0;
            greens += pixel[1] > 240 && pixel[0] == 0;
        }
        const unsigned char *blue = image + (88 * scale * 320 * scale + 208 * scale) * 3;
        assert(blue[0] == 0 && blue[1] == 0 && blue[2] > 240);
        free(image);
        assert(reds > 0 && greens > 0 && reds + greens == scale);
        RECT point = {160, 80, 1, 1}; u_long stored = 0;
        StoreImage(&point, &stored);
        unsigned color = stored & 0x7fff;
        assert(color == 0x001f || color == 0x03e0);
        assert((stored & 0x8000) == (color == 0x001f ? 0x8000 : 0));
        /* Following ordinary model textures keep their existing interpolation. */
        unsigned perspective = SampleTexture(1, -1);
        assert(perspective >= 40 && perspective <= 60);
    }
    puts("4x model framebuffer feedback retains subpixel colors and STP with depth off/on.");
}

static void CheckBatchPaletteResolve(void)
{
    assert(Psyz_GpuConfigureGeometry(NULL, 0, 0, 0) == 0);
    Clear();
    uint16_t indices[16 * 16], colors[16];
    for (unsigned i = 0; i < 16 * 16; ++i) indices[i] = 0x1111;
    for (unsigned i = 0; i < 16; ++i) colors[i] = 0x03e0;
    RECT texture = {768, 256, 16, 16}, safe_palette = {256, 480, 16, 1};
    LoadImage(&texture, (u_long *)indices);
    LoadImage(&safe_palette, (u_long *)colors);

    /* Leave a GPU-written CLUT pending in scaled VRAM, outside the drawing
     * area. Only the second polygon's palette intersects that pending write. */
    DRAWENV palette_draw, normal_draw;
    GetDrawEnv(&normal_draw);
    SetDefDrawEnv(&palette_draw, 0, 480, 16, 1);
    palette_draw.dtd = 0;
    PutDrawEnv(&palette_draw);
    TILE clut = {0};
    setTile(&clut);
    setXY0(&clut, 0, 0);
    setWH(&clut, 16, 1);
    setRGB0(&clut, 255, 0, 0);
    Submit(&clut);
    PutDrawEnv(&normal_draw);

    POLY_FT4 quads[2] = {0};
    for (int i = 0; i < 2; ++i) {
        setPolyFT4(&quads[i]);
        setShadeTex(&quads[i], 1);
        setXYWH(&quads[i], 16 + i * 64, 16, 32, 32);
        setUVWH(&quads[i], 0, 0, 8, 8);
        quads[i].tpage = getTPage(0, 0, 768, 256);
        setClut(&quads[i], i ? 0 : 256, 480);
        Submit(&quads[i]);
    }
    unsigned char *image = Capture();
    const unsigned char *green = image + (24 * scale * 320 * scale + 24 * scale) * 3;
    const unsigned char *red = image + (24 * scale * 320 * scale + 88 * scale) * 3;
    assert(green[0] == 0 && green[1] > 240 && green[2] == 0);
    assert(red[0] > 240 && red[1] == 0 && red[2] == 0);
    free(image);
    puts("Batched texture reuse: changed CLUT resolves pending GPU palette writes.");
}

static void CheckCrossingDepth(void)
{
    assert(Psyz_GpuConfigureGeometry(Geometry, 1, 1, 1) == 0);
    Clear();
    POLY_F4 sloped = {0};
    setPolyF4(&sloped);
    setXYWH(&sloped, 16, 16, 128, 128);
    setRGB0(&sloped, 255, 0, 0);
    RegisterQuad(&sloped.x0, &sloped.x1, &sloped.x2, &sloped.x3, 100, 400, 0);
    Submit(&sloped);
    Solid(200, 0, 255, 0, 0);
    unsigned char *image = Capture();
    size_t row = 80 * scale * 320 * scale;
    assert(image[(row + 32 * scale) * 3] > 240);
    assert(image[(row + 128 * scale) * 3 + 1] > 240);
    free(image);
    assert(Psyz_GpuSetTextureAlignment(1) == 0);
    Solid(900, 0, 255, 0, 0);
    image = Capture();
    assert(image[(row + 32 * scale) * 3 + 1] > 240);
    free(image);
    assert(Psyz_GpuSetTextureAlignment(0) == 0);
}

static int LeftEdge(int precision)
{
    assert(Psyz_GpuConfigureGeometry(Geometry, 0, precision, 0) == 0);
    Clear();
    Solid(100, 255, 0, 0, 0.75f);
    unsigned char *image = Capture();
    int x;
    for (x = 0; x < 320 * (int)scale; ++x)
        if (image[(80 * scale * 320 * scale + x) * 3] > 200) break;
    free(image);
    return x;
}

static unsigned char *CodecBorder(int poison)
{
    assert(Psyz_GpuSetTextureAlignment(0) == 0);
    assert(Psyz_GpuConfigureGeometry(Geometry, 0, poison, 0) == 0);
    Clear();
    /* Populate every vertex reused by the three line segments with model
     * offsets. Clearing the framebuffer does not clear the CPU vertex buffer. */
    for (int i = 0; i < 3; ++i) Solid(100, 255, 0, 0, 37.75f);
    DrawSync(0);
    Clear();
    assert(Psyz_GpuSetTextureAlignment(1) == 0);
    LINE_F4 border = {0};
    setLineF4(&border);
    setRGB0(&border, 0, 255, 0);
    setXY4(&border, 31, 30, 83, 30, 83, 119, 31, 119);
    Submit(&border);
    return Capture();
}

static void CheckCodecLineIsolation(void)
{
    unsigned char *legacy = CodecBorder(0);
    unsigned char *precise = CodecBorder(1);
    size_t bytes = (size_t)320 * 224 * scale * scale * 3;
    size_t changed = 0;
    for (size_t i = 0; i < bytes; ++i) changed += legacy[i] != precise[i];
    printf("Codec border after projected geometry: %zu changed bytes\n", changed);
    fflush(stdout);
    free(legacy);
    free(precise);
    assert(changed == 0);
    assert(Psyz_GpuSetTextureAlignment(0) == 0);
}

static unsigned char *GroundShadow(int depth, int ordered, float near_depth, float far_depth)
{
    assert(Psyz_GpuConfigureGeometry(Geometry, 1, 1, depth) == 0);
    Clear();
    Solid(100, 255, 255, 0, 0);
    POLY_FT4 shadow = {0};
    setPolyFT4(&shadow);
    setSemiTrans(&shadow, 1);
    setShadeTex(&shadow, 1);
    setXYWH(&shadow, 16, 16, 128, 128);
    setUVWH(&shadow, 0, 0, 64, 64);
    shadow.tpage = getTPage(2, 2, 640, 256);
    /* Almost coplanar, with interpolation crossing the floor. The original
     * game places this decal by OT raise rather than physical separation. */
    RegisterQuad(&shadow.x0, &shadow.x1, &shadow.x2, &shadow.x3, near_depth, far_depth, 0);
    geometry_flags = PSYZ_GEOMETRY_VALID | (ordered ? PSYZ_GEOMETRY_ORDERED : 0);
    Submit(&shadow);
    geometry_flags = PSYZ_GEOMETRY_VALID;
    unsigned char *image = Capture();
    if (depth && ordered) {
        /* The decal must not erase or write the solid floor's depth. */
        Solid(100.02f, 255, 0, 0, 0);
        unsigned char *behind = Capture();
        size_t bytes = (size_t)320 * 224 * scale * scale * 3;
        for (size_t i = 0; i < bytes; ++i) assert(image[i] == behind[i]);
        free(behind);
    }
    return image;
}

static void CheckOrderedEffect(const char *name, float near_depth, float far_depth)
{
    uint16_t pixels[64 * 64];
    for (int i = 0; i < 64 * 64; ++i) pixels[i] = 0x8421;
    RECT texture = {640, 256, 64, 64};
    LoadImage(&texture, (u_long *)pixels);
    unsigned char *original = GroundShadow(0, 0, near_depth, far_depth);
    unsigned char *untagged = GroundShadow(1, 0, near_depth, far_depth);
    unsigned char *ordered = GroundShadow(1, 1, near_depth, far_depth);
    size_t bytes = (size_t)320 * 224 * scale * scale * 3;
    size_t defect = 0, changed = 0;
    for (size_t i = 0; i < bytes; ++i) {
        defect += original[i] != untagged[i];
        changed += original[i] != ordered[i];
    }
    printf("%s: untagged=%zu, ordered=%zu changed bytes\n", name, defect, changed);
    fflush(stdout);
    assert(defect > 0 && changed == 0);
    free(original);
    free(untagged);
    free(ordered);
}

static void CheckResolutionTransitions(void)
{
    assert(Psyz_GpuConfigureGeometry(NULL, 0, 0, 0) == 0);
    Clear();
    TILE tile = {0};
    setTile(&tile);
    setXY0(&tile, 32, 32);
    setWH(&tile, 48, 48);
    setRGB0(&tile, 255, 0, 0);
    for (int i = 0; i < 24; ++i) {
        assert(Psyz_VideoSetInternalResolution(i & 1 ? 2 : 7) == 0);
        Submit(&tile);
        DrawSync(0);
    }
    assert(Psyz_VideoSetInternalResolution(scale) == 0);
    unsigned char *image = Capture();
    size_t pixel = ((size_t)48 * scale * 320 * scale + 48 * scale) * 3;
    assert(image[pixel] > 240 && image[pixel + 1] == 0);
    free(image);
    puts("24 subtitle-style 2x/7x resolution transitions preserved VRAM.");
}

static void CheckStealthFeedback(void)
{
    uint16_t pixels[320 * 224];
    for (int y = 0; y < 224; ++y)
        for (int x = 0; x < 320; ++x)
            pixels[y * 320 + x] = (uint16_t)(1 | ((x % 31) << 5) | ((y % 31) << 10));
    RECT previous = {320, 0, 320, 224};
    RECT material = {640, 256, 64, 64};
    LoadImage(&previous, (u_long *)pixels);
    LoadImage(&material, (u_long *)pixels);
    for (int depth = 0; depth <= 1; ++depth) {
        assert(Psyz_GpuConfigureGeometry(Geometry, 1, 1, depth) == 0);
        for (int frame = 0; frame < 8; ++frame) {
            Clear();
            Solid(1000, 128, 128, 0, 0);
            Solid(900, 32, 32, 1, 0); /* allocate/copy the scaled destination snapshot */
            for (int i = 0; i < 256; ++i) {
                POLY_GT4 quad = {0};
                setPolyGT4(&quad);
                setRGB0(&quad, 128, 128, 128);
                setRGB1(&quad, 128, 128, 128);
                setRGB2(&quad, 128, 128, 128);
                setRGB3(&quad, 128, 128, 128);
                int x = 32 + (i % 16) * 12, y = 16 + (i / 16) * 12;
                setXYWH(&quad, x, y, 12, 12);
                /* Kogaku3's sweep mixes normal materials with direct-color
                 * framebuffer pages. Exercise current and prior buffers. */
                int revealed = (i / 16) < frame * 2;
                quad.tpage = getTPage(2, 0, revealed ? (frame & 1 ? 0 : 320) : 640,
                                     revealed ? 0 : 256);
                setUVWH(&quad, revealed ? x : 0, revealed ? y : 0, 12, 12);
                RegisterQuad(&quad.x0, &quad.x1, &quad.x2, &quad.x3,
                             100 + i, 200 + i, 0.25f);
                Submit(&quad);
            }
            unsigned char *image = Capture();
            free(image);
        }
    }
    /* Match the failing close-up's ~218 feedback resolves. Crossing the
     * binary resolve boundaries makes each face expensive at 7x. */
    Clear();
    Solid(1000, 128, 128, 0, 0);
    POLY_F4 sentinel = {0};
    setPolyF4(&sentinel);
    setXYWH(&sentinel, 288, 192, 16, 16);
    setRGB0(&sentinel, 255, 0, 0);
    DrawSync(0); /* execute ClearImage's queued mask reset first */
    Psyz_GpuWriteGP0(0xe6000001u);
    DrawSync(0);
    RegisterQuad(&sentinel.x0, &sentinel.x1, &sentinel.x2, &sentinel.x3, 50, 50, 0);
    Submit(&sentinel);
    DrawSync(0);
    Psyz_GpuWriteGP0(0xe6000000u);
    for (int i = 0; i < 218; ++i) {
        POLY_FT4 closeup = {0};
        setPolyFT4(&closeup);
        setShadeTex(&closeup, 1);
        setXYWH(&closeup, 32, 32, 256, 160);
        setUVWH(&closeup, 0, 0, 255, 160);
        closeup.tpage = getTPage(2, 0, 0, 0);
        RegisterQuad(&closeup.x0, &closeup.x1, &closeup.x2, &closeup.x3, 500, 500, 0);
        Submit(&closeup);
    }
    setRGB0(&sentinel, 0, 255, 0);
    RegisterQuad(&sentinel.x0, &sentinel.x1, &sentinel.x2, &sentinel.x3, 100, 100, 0);
    Submit(&sentinel); /* prior depth survives intermediate submissions */
    DrawSync(0);
    RECT sentinel_pixel = {296, 200, 1, 1};
    u_long stored = 0;
    StoreImage(&sentinel_pixel, &stored);
    printf("Depth/mask sentinel after feedback: %04lx\n", stored & 0xffffu);
    fflush(stdout);
    assert((stored & 0xffffu) == 0x801fu);
    Psyz_GpuWriteGP0(0xe6000002u);
    DrawSync(0);
    RegisterQuad(&sentinel.x0, &sentinel.x1, &sentinel.x2, &sentinel.x3, 25, 25, 0);
    Submit(&sentinel); /* preserved VRAM mask rejects even a nearer face */
    DrawSync(0);
    StoreImage(&sentinel_pixel, &stored);
    assert((stored & 0xffffu) == 0x801fu);
    Psyz_GpuWriteGP0(0xe6000000u);
    unsigned char *image = Capture();
    free(image);
    puts("7x stealth: 4096 mixed GT4s plus 218 large feedback faces completed with depth off/on.");
}

static void CheckMovieSliceUploads(void)
{
    /* Repeated strips exceed staging capacity without a presentation boundary.
     * Distinct columns catch premature reuse of a recorded upload's bytes. */
    u_short pixels[16 * 160];
    for (unsigned frame = 0; frame < 14; ++frame) {
        for (unsigned column = 0; column < 20; ++column) {
            u_short color = (u_short)(1 + column + frame * 32);
            for (unsigned i = 0; i < 16 * 160; ++i) pixels[i] = color;
            RECT strip = {(short)(column * 16), 24, 16, 160};
            LoadImage(&strip, (u_long *)pixels);
        }
    }
    for (unsigned column = 0; column < 20; ++column) {
        RECT point = {(short)(column * 16 + 8), 80, 1, 1};
        u_long result = 0;
        StoreImage(&point, &result);
        assert((result & 0xffffu) == 1 + column + 13 * 32);
    }
    puts("Movie slice staging: appended uploads and capacity wrap preserve pixels.");
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    assert(argc == 2);
    int stealth = !strcmp(argv[1], "stealth");
    int ordered_effect = !strcmp(argv[1], "ordered-effect");
    scale = stealth ? 7 : ordered_effect ? 4 : (unsigned)atoi(argv[1]);
    assert(scale == 1 || scale == 4 || stealth);
    assert(Psyz_VideoConfigureWindow(320 * scale, 224 * scale, 0) == 0);
    assert(Psyz_VideoSetPresentation(320, 224, 224) == 0);
    assert(Psyz_VideoSetInternalResolution(scale) == 0);
    ResetGraph(0);
    Psyz_VideoSetDitheringMode(PSYZ_DITHER_OFF);
    DRAWENV draw;
    DISPENV display;
    SetDefDrawEnv(&draw, 0, 0, 320, 224);
    draw.dtd = 0;
    SetDefDispEnv(&display, 0, 0, 320, 240);
    PutDispEnv(&display);
    PutDrawEnv(&draw);
    SetDispMask(1);
    DrawSync(0);
    if (stealth) { CheckStealthFeedback(); return 0; }
    if (ordered_effect) {
        CheckDepth();
        CheckOrderedEffect("Electric floor ordering", 110.0f, 120.0f);
        CheckOrderedEffect("Office coplanar overlay", 99.99f, 100.01f);
        return 0;
    }
    CheckMovieSliceUploads();
    CheckDepth();
    CheckTextures();
    CheckFramebufferTextureAffine();
    CheckHighResolutionFramebufferTexture();
    CheckBatchPaletteResolve();
    CheckCrossingDepth();
    int legacy = LeftEdge(0), precise = LeftEdge(1);
    assert(precise - legacy == (int)(scale * 0.75f + 0.5f));
    CheckCodecLineIsolation();
    CheckOrderedEffect("Ground shadow", 99.99f, 100.01f);
    /* Electric-floor OT raise intentionally overrides distinct physical depth. */
    CheckOrderedEffect("Electric floor ordering", 110.0f, 120.0f);
    CheckResolutionTransitions();
    printf("Modern GPU %ux: depth occlusion/reset, transparency, perspective UVs, missing depth and subpixel coverage passed.\n", scale);
    return 0;
}
