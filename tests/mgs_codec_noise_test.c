#include "mgs_codec_noise.h"

#include <assert.h>
#include <string.h>

int main(void)
{
    uint16_t first[MGS_CODEC_NOISE_WORDS];
    uint16_t again[MGS_CODEC_NOISE_WORDS];
    uint16_t next[MGS_CODEC_NOISE_WORDS];
    unsigned tone_counts[6] = {0};
    unsigned changed = 0;

    MgsCodecFillNoise(first, 7, 0, 0);
    MgsCodecFillNoise(again, 7, 0, 0);
    MgsCodecFillNoise(next, 8, 1, 0);
    assert(memcmp(first, again, sizeof(first)) == 0);
    for (unsigned i = 0; i < MGS_CODEC_NOISE_WORDS; ++i) {
        unsigned lo = first[i] & 255u;
        unsigned hi = first[i] >> 8;
        assert(lo && hi); /* no transparent 8-bit texels */
        ++tone_counts[lo % 6];
        ++tone_counts[hi % 6];
        changed += first[i] != next[i];
    }
    assert(changed > MGS_CODEC_NOISE_WORDS * 9 / 10);
    for (unsigned i = 0; i < 6; ++i)
        assert(tone_counts[i] > 500); /* all six original palette tones appear */
    return 0;
}
