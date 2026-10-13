/* Manual renderer specimen: supplied licensed atlas, no game resources. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgpu.h>
#include <psyz.h>
#include "mgs_hires_font.h"

int ReiReintegratedTitleVisible;

static void label(const char *text, int x, int y, int r, int g, int b)
{
    for (; *text; ++text) {
        if (*text == ' ') { x += 4; continue; }
        if (*text == 'I') ++x;
        SPRT sprite = {0};
        setSprt(&sprite); setRGB0(&sprite, r, g, b);
        setXY0(&sprite, x, y); setWH(&sprite, 8, 6);
        MgsHiResFontRegisterInterfaceGlyph(&sprite.r0, (unsigned char)*text,
                                           0);
        termPrim((P_TAG *)&sprite);
        DrawOTag((OT_TYPE *)&sprite);
        x += *text == 'I' ? 4 : 9;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "Usage: specimen atlas.bmp output.ppm\n"); return 2; }
#ifdef _WIN32
    assert(_putenv_s("REI_INTERFACE_FONT_ATLAS", argv[1]) == 0);
#else
    assert(setenv("REI_INTERFACE_FONT_ATLAS", argv[1], 1) == 0);
#endif
    assert(Psyz_VideoConfigureWindow(1920, 1080, 0) == 0);
    assert(Psyz_VideoSetPresentation(16, 9, 224) == 0);
    assert(Psyz_VideoSetInternalResolution(8) == 0);
    ResetGraph(0);
    DISPENV display; SetDefDispEnv(&display, 0, 0, 320, 240);
    PutDispEnv(&display); SetDispMask(1);
    DRAWENV draw; SetDefDrawEnv(&draw, 0, 0, 320, 224);
    draw.dtd = 0; PutDrawEnv(&draw);
    RECT area = {0, 0, 320, 224}; ClearImage(&area, 0, 0, 0);
    unsigned short palette[16] = {0, 0xc210};
    unsigned char source[216 * 12 / 2] = {0};
    MgsHiResFontSetInterfacePalette(palette);
    MgsHiResFontValidateInterfacePixels(source, 216, 12);
    label("PAUSE", 137, 25, 248, 248, 248);
    label("SAVE DATA", 25, 55, 72, 160, 128);
    label("SELECT MEMORY CARD", 75, 85, 72, 160, 128);
    label("FREE: 13 BLOCKS", 50, 115, 72, 160, 128);
    label("NEW FILE", 110, 145, 248, 248, 248);
    label("0123456789", 25, 175, 160, 248, 184);
    DrawSync(0);
    int width, height;
    unsigned char *pixels = Psyz_VideoAllocPresentedFrame(&width, &height);
    assert(pixels && width == 1920 && height == 1080);
    FILE *file = fopen(argv[2], "wb"); assert(file);
    fprintf(file, "P6\n%d %d\n255\n", width, height);
    assert(fwrite(pixels, 3, (size_t)width * height, file) == (size_t)width * height);
    fclose(file); free(pixels);
    return 0;
}
