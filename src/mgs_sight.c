#include "mgs_sight.h"

#include <libgpu.h>
#include <stdint.h>
#include <string.h>

void MgsSightExpandEdgeMask(void *packet, size_t size, int half_width, int blend_mode)
{
    POLY_F4 *mask = packet;
    short *x[4];
    short y[4];
    int edge, i, j, vertical;
    if (half_width <= 160 || half_width > 16383 || size < sizeof(*mask))
        return;
    /* Retail sight masks are black wedges. Extend only their vertical outer
       edge; the other vertices describe the circular aperture. */
    if (getlen(mask) != 5 ||
        !((mask->code == 0x28 && !mask->r0 && !mask->g0 && !mask->b0) ||
          (mask->code == 0x2a && blend_mode == 2 &&
           mask->r0 == 255 && mask->g0 == 255 && mask->b0 == 255)))
        return;
    x[0] = &mask->x0; x[1] = &mask->x1;
    x[2] = &mask->x2; x[3] = &mask->x3;
    y[0] = mask->y0; y[1] = mask->y1;
    y[2] = mask->y2; y[3] = mask->y3;
    /* SGT assets use both strip and perimeter vertex ordering. */
    for (edge = 0; edge <= 320; edge += 320)
    {
        vertical = 0;
        for (i = 0; i < 4; ++i)
            for (j = i + 1; j < 4; ++j)
                if (*x[i] == edge && *x[j] == edge && y[i] != y[j])
                    vertical = 1;
        if (vertical)
            for (i = 0; i < 4; ++i)
                if (*x[i] == edge)
                    *x[i] = edge == 0 ? 160 - half_width : 160 + half_width;
    }
}

size_t MgsSightCopyPacket(void *destination, size_t capacity,
                         const void *source, size_t available)
{
    uint32_t tag;
    size_t payload;
    size_t size;

    if (available < sizeof(tag))
    {
        return 0;
    }
    memcpy(&tag, source, sizeof(tag));
    payload = (tag >> 24) * sizeof(uint32_t);
    size = offsetof(P_TAG, r0) + payload;
    if (payload == 0 || available - sizeof(tag) < payload || capacity < size)
    {
        return 0;
    }
    setaddr(destination, NULL);
    setlen(destination, tag >> 24);
    memcpy((char *)destination + offsetof(P_TAG, r0),
           (const char *)source + sizeof(tag), payload);
    return size;
}
