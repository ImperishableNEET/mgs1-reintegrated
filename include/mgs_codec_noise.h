#ifndef MGS_CODEC_NOISE_H
#define MGS_CODEC_NOISE_H

#include <stdint.h>

#define MGS_CODEC_NOISE_WORDS (26u * 89u)

/* The recovered native arena has no arbitrary PS1 RAM contents to sample.
 * Generate 52x89 8-bit texel indices for the original Codec palette instead.
 * Sample is the original single rand()%32 result, preserving the global RNG
 * call count. Index zero is excluded because PS1 textures treat it as clear. */
static inline void MgsCodecFillNoise(uint16_t pixels[MGS_CODEC_NOISE_WORDS],
                                     unsigned sample, unsigned phase,
                                     unsigned face)
{
    uint32_t state = 0x9e3779b9u ^ ((sample + 1u) * 0x85ebca6bu) ^
                     ((phase + 1u) * 0xc2b2ae35u) ^
                     ((face + 1u) * 0x27d4eb2fu);
    for (unsigned i = 0; i < MGS_CODEC_NOISE_WORDS; ++i) {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        unsigned lo = (state >> 8) & 255u;
        unsigned hi = (state >> 16) & 255u;
        if (!lo) lo = 1;
        if (!hi) hi = 1;
        pixels[i] = (uint16_t)(lo | (hi << 8));
    }
}

#endif
