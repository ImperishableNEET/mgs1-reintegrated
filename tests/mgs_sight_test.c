#include "mgs_sight.h"

#include <assert.h>
#include <libgpu.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const uint32_t polygon[] = {
        0x05000005, 0x28030201, 0x00020001, 0x00040003, 0x00060005, 0x00080007
    };
    const uint32_t text[] = {0xff, 0x00404040};
    POLY_F4 packet;
    POLY_F4 before;

    memset(&packet, 0xa5, sizeof(packet));
    before = packet;
    assert(MgsSightCopyPacket(&packet, sizeof(packet) - 1,
                              polygon, sizeof(polygon)) == 0);
    assert(MgsSightCopyPacket(&packet, sizeof(packet),
                              polygon, sizeof(polygon) - 1) == 0);
    assert(MgsSightCopyPacket(&packet, sizeof(packet), polygon, 3) == 0);
    assert(MgsSightCopyPacket(&packet, sizeof(packet), text, sizeof(text)) == 0);
    assert(memcmp(&packet, &before, sizeof(packet)) == 0);

    assert(MgsSightCopyPacket(&packet, sizeof(packet),
                              polygon, sizeof(polygon)) == sizeof(packet));
    assert(getlen(&packet) == 5);
    assert(getaddr(&packet) == 0);
    assert(memcmp(&packet.r0, polygon + 1, sizeof(polygon) - 4) == 0);
    assert(packet.x3 == 7 && packet.y3 == 8);
    /* Authored rifle2 wedges retain the circular inner edge. */
    setPolyF4(&packet);
    setRGB0(&packet, 0, 0, 0);
    setXY4(&packet, 287, 110, 320, 110, 285, 133, 320, 133);
    before = packet;
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 160, 0);
    assert(memcmp(&packet, &before, sizeof(packet)) == 0);
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 0);
    assert(packet.x1 == 374 && packet.x3 == 374);
    assert(packet.x0 == 287 && packet.x2 == 285);
    assert(packet.y0 == 110 && packet.y2 == 133);
    setXY4(&packet, 32, 133, 0, 133, 31, 111, 0, 111);
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 284, 0);
    assert(packet.x1 == -124 && packet.x3 == -124);
    assert(packet.x0 == 32 && packet.x2 == 31);
    /* Camera SGT uses a different corner order for its right border. */
    setXY4(&packet, 320, 8, 320, 216, 312, 8, 312, 216);
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 0);
    assert(packet.x0 == 374 && packet.x1 == 374);
    assert(packet.x2 == 312 && packet.x3 == 312);
    setXY4(&packet, 0, 0, 320, 0, 0, 8, 320, 8);
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 0);
    assert(packet.x0 == -54 && packet.x2 == -54);
    assert(packet.x1 == 374 && packet.x3 == 374);
    setXY4(&packet, 0, 8, 0, 8, 10, 8, 10, 8);
    before = packet;
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 0);
    assert(memcmp(&packet, &before, sizeof(packet)) == 0);
    setXY4(&packet, 0, 0, 320, 0, 0, 8, 320, 8);
    packet.code = 0x2a; /* Semi-transparent crosshair/effect is unchanged. */
    before = packet;
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 0);
    assert(memcmp(&packet, &before, sizeof(packet)) == 0);
    packet.code = 0x28;
    packet.r0 = 1;
    before = packet;
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 0);
    assert(memcmp(&packet, &before, sizeof(packet)) == 0);
    packet.r0 = 0;
    before = packet;
    MgsSightExpandEdgeMask(&packet, sizeof(packet) - 1, 214, 0);
    assert(memcmp(&packet, &before, sizeof(packet)) == 0);
    setXY4(&packet, 150, 100, 160, 100, 150, 120, 160, 120);
    before = packet;
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 0);
    assert(memcmp(&packet, &before, sizeof(packet)) == 0);
    /* Actual gas-mask outer panel: white subtractive (ABR=2), not black. */
    setPolyF4(&packet);
    setSemiTrans(&packet, 1);
    setRGB0(&packet, 255, 255, 255);
    setXY4(&packet, 318, 78, 320, 78, 318, 116, 320, 116);
    before = packet;
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 0);
    assert(memcmp(&packet, &before, sizeof(packet)) == 0);
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 2);
    assert(packet.x1 == 374 && packet.x3 == 374);
    assert(packet.x0 == 318 && packet.x2 == 318);
    setXY4(&packet, 0, 78, 0, 0, 2, 78, 2, 0);
    MgsSightExpandEdgeMask(&packet, sizeof(packet), 214, 2);
    assert(packet.x0 == -54 && packet.x1 == -54);
    assert(packet.x2 == 2 && packet.x3 == 2);
    puts("Sight retail packet conversion and bounds passed.");
    return 0;
}
