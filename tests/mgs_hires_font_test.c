#include "mgs_hires_font.h"

#include <assert.h>
#include <stdlib.h>

int main(void)
{
    int kcb, subtitle_kcb, first_packet, second_packet, subtitle_packet;
    int unsupported_packet, after_detach_packet;
    unsigned short first_palette[16] = {0};
    unsigned short second_palette[16] = {0};
    first_palette[3] = 0x6739;
    second_palette[3] = 0x3bef;

#ifdef _WIN32
    assert(_putenv_s("REI_CODEC_FONT_ATLAS", "fixture") == 0);
#else
    assert(setenv("REI_CODEC_FONT_ATLAS", "fixture", 1) == 0);
#endif
    MgsHiResFontAttach(&kcb);
    MgsHiResFontClear(&kcb);
    MgsHiResFontGlyph(&kcb, 'A', 7, 2, 0, 8, 1);
    MgsHiResFontUpdate(&kcb, first_palette);
    MgsHiResFontRegisterSprite(&first_packet, &kcb, 42, 132, 252, 76);

    /* Jimaku and Codec keep independent buffers while their OTs coexist. */
    MgsHiResFontAttach(&subtitle_kcb);
    MgsHiResFontClear(&subtitle_kcb);
    MgsHiResFontGlyph(&subtitle_kcb, 'S', 4, 6, 0, 7, 0);
    MgsHiResFontUpdate(&subtitle_kcb, second_palette);
    MgsHiResFontRegisterSprite(&subtitle_packet, &subtitle_kcb, 20, 196, 256, 20);

    /* Updating the KCB must not rewrite an earlier queued packet. */
    MgsHiResFontClear(&kcb);
    MgsHiResFontGlyph(&kcb, 'B', 13, 20, 1, 7, 0);
    MgsHiResFontUpdate(&kcb, second_palette);
    MgsHiResFontRegisterSprite(&second_packet, &kcb, 43, 132, 252, 76);
    const MgsHiResFontSnapshot *first = MgsHiResFontTakeSprite(&first_packet);
    assert(first && first->count == 1 && first->glyphs[0].code == 'A');
    assert(first->glyphs[0].x == 7 && first->glyphs[0].y_offset == 1);
    assert(first->palette[3] == first_palette[3] && first->x == 42);
    assert(!MgsHiResFontTakeSprite(&first_packet));
    const MgsHiResFontSnapshot *subtitle = MgsHiResFontTakeSprite(&subtitle_packet);
    assert(subtitle && subtitle->glyphs[0].code == 'S');
    assert(subtitle->palette[3] == second_palette[3] && subtitle->y == 196);
    const MgsHiResFontSnapshot *second = MgsHiResFontTakeSprite(&second_packet);
    assert(second && second->count == 1 && second->glyphs[0].code == 'B');
    assert(second->palette[3] == second_palette[3] && second->x == 43);

    /* One unsupported glyph makes the whole board use its native sprite. */
    MgsHiResFontClear(&kcb);
    MgsHiResFontGlyph(&kcb, 'A', 7, 2, 0, 8, 1);
    MgsHiResFontUnsupported(&kcb);
    MgsHiResFontUpdate(&kcb, first_palette);
    MgsHiResFontRegisterSprite(&unsupported_packet, &kcb, 42, 132, 252, 76);
    assert(!MgsHiResFontTakeSprite(&unsupported_packet));

    MgsHiResFontClear(&kcb);
    MgsHiResFontGlyph(&kcb, 'C', 5, 2, 0, 7, 0);
    MgsHiResFontUpdate(&kcb, first_palette);
    MgsHiResFontRegisterSprite(&after_detach_packet, &kcb, 42, 132, 252, 76);
    MgsHiResFontDetach(&kcb);
    const MgsHiResFontSnapshot *after_detach =
        MgsHiResFontTakeSprite(&after_detach_packet);
    assert(after_detach && after_detach->glyphs[0].code == 'C');

    /* Two queued contact OTs plus subtitle outlines must retain all tags. */
    int contact_packets[65];
    MgsHiResFontAttach(&kcb);
    MgsHiResFontClear(&kcb);
    MgsHiResFontGlyph(&kcb, 'A', 98, 12, 0, 8, 0);
    MgsHiResFontUpdate(&kcb, first_palette);
    for (int i = 0; i < 64; ++i)
        MgsHiResFontRegisterSpriteRegion(&contact_packets[i], &kcb,
                                         64 + i, 133, 96, 12, 96, 12);
    MgsHiResFontSetSpriteForeColor(&contact_packets[0], 0, 0x3a4b);
    /* Replacing an existing packet while full must not consume another slot. */
    MgsHiResFontRegisterSpriteRegion(&contact_packets[0], &kcb,
                                     500, 133, 96, 12, 96, 12);
    MgsHiResFontRegisterSpriteRegion(&contact_packets[64], &kcb,
                                     64, 133, 96, 12, 96, 12);
    assert(!MgsHiResFontTakeSprite(&contact_packets[64]));
    assert(!MgsHiResFontTakeSprite(NULL));
    for (int i = 0; i < 64; ++i) {
        const MgsHiResFontSnapshot *contact =
            MgsHiResFontTakeSprite(&contact_packets[i]);
        assert(contact && contact->x == (i == 0 ? 500 : 64 + i) && contact->source_x == 96 &&
               contact->source_y == 12 && contact->glyphs[0].code == 'A');
        assert(contact->palette[3] == first_palette[3]);
    }
    MgsHiResFontRegisterSpriteRegion(&contact_packets[64], &kcb,
                                     700, 133, 96, 12, 96, 12);
    const MgsHiResFontSnapshot *reused = MgsHiResFontTakeSprite(&contact_packets[64]);
    assert(reused && reused->x == 700 && reused->glyphs[0].code == 'A');
    assert(!MgsHiResFontTakeSprite(&contact_packets[64]));
    MgsHiResFontDetach(&kcb);
    MgsHiResFontDetach(&subtitle_kcb);
#ifdef _WIN32
    assert(_putenv_s("REI_INTERFACE_FONT_ATLAS", "fixture") == 0);
#else
    assert(setenv("REI_INTERFACE_FONT_ATLAS", "fixture", 1) == 0);
#endif
    unsigned short interface_palette[16] = {0, 0xc210};
    MgsHiResFontSetInterfacePalette(interface_palette);
    int interface_packets[1025];
    for (int i = 0; i < 1024; ++i)
        MgsHiResFontRegisterInterfaceGlyph(&interface_packets[i], 'A', 0x1234);
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[1024], 'B', 0x1234);
    assert(!MgsHiResFontTakeInterfaceGlyph(&interface_packets[1024], NULL, NULL));
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[500], 'C', 0x1234);
    for (int i = 0; i < 1024; ++i)
        assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[i], NULL, NULL) == (i == 500 ? 'C' : 'A'));
    assert(!MgsHiResFontTakeInterfaceGlyph(NULL, NULL, NULL));
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[0], 128, 0x1234);
    assert(!MgsHiResFontTakeInterfaceGlyph(&interface_packets[0], NULL, NULL));
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[0], 'D', 0x1234);
    assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[0], NULL, NULL) == 'D');
    assert(!MgsHiResFontTakeInterfaceGlyph(&interface_packets[0], NULL, NULL));
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[0], 'A', 0x1234);
    interface_palette[1] = 0x001f;
    MgsHiResFontSetInterfacePalette(interface_palette);
    unsigned short captured_fore = 0;
    unsigned captured_uv_clut = 0;
    assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[0], &captured_fore, &captured_uv_clut) == 'A');
    assert(captured_uv_clut == 0x1234);
    assert(captured_fore == 0xc210); /* queued palette is immutable */
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[0], 'A', 0x1234);
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[0], '*', 0x1234);
    assert(!MgsHiResFontTakeInterfaceGlyph(&interface_packets[0], NULL, NULL));
    /* A hole before a live packet must not turn replacement into duplication.
     * The conservative scan range resets only when the queue is fully drained. */
    for (int i = 0; i < 8; ++i)
        MgsHiResFontRegisterInterfaceGlyph(&interface_packets[i], 'A' + i, 0x1200 + i);
    assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[1], NULL, NULL) == 'B');
    assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[7], NULL, NULL) == 'H');
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[6], 'Z', 0x5678);
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[8], '9', 0xabcd);
    assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[6], &captured_fore, &captured_uv_clut) == 'Z');
    assert(captured_fore == 0x001f && captured_uv_clut == 0x5678);
    assert(!MgsHiResFontTakeInterfaceGlyph(&interface_packets[6], NULL, NULL));
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[5], '*', 0x1234);
    assert(!MgsHiResFontTakeInterfaceGlyph(&interface_packets[5], NULL, NULL));
    for (int i = 4; i >= 0; --i)
        if (i != 1)
            assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[i], NULL, NULL) == (unsigned)('A' + i));
    assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[8], NULL, NULL) == '9');
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[1024], 'Q', 0xffff);
    assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[1024], NULL, NULL) == 'Q');
    interface_palette[2] = 0x03e0; /* multiple nonblack indices unsupported */
    MgsHiResFontSetInterfacePalette(interface_palette);
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[0], 'A', 0x1234);
    assert(!MgsHiResFontTakeInterfaceGlyph(&interface_packets[0], NULL, NULL));
    interface_palette[2] = 0x8000;
    MgsHiResFontSetInterfacePalette(interface_palette);
    unsigned char panel[216 * 12 / 2] = {0};
    MgsHiResFontValidateInterfacePixels(panel, 216, 12);
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[0], 'A', 0x1234);
    assert(MgsHiResFontTakeInterfaceGlyph(&interface_packets[0], NULL, NULL) == 'A');
    panel[1200] = 0x20;
    MgsHiResFontValidateInterfacePixels(panel, 216, 12);
    MgsHiResFontRegisterInterfaceGlyph(&interface_packets[0], 'A', 0x1234);
    assert(!MgsHiResFontTakeInterfaceGlyph(&interface_packets[0], NULL, NULL));
    return 0;
}
