#include "mgs_language.h"

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#define test_pid _getpid
#else
#include <unistd.h>
#define test_pid getpid
#endif

static unsigned search_calls;
static void *CountedBsearch(const void *key, const void *items, size_t count,
                            size_t width, int (*compare)(const void *, const void *))
{
    ++search_calls;
    return bsearch(key, items, count, width, compare);
}

/* Include the implementation so the fixture measures its exact FindRecord
 * fallback as well as the public cached glyph path. */
#define bsearch CountedBsearch
#include "../src/mgs_language.c"
#undef bsearch

static void Write32(unsigned char *out, unsigned value)
{
    out[0] = (unsigned char)value;
    out[1] = (unsigned char)(value >> 8);
    out[2] = (unsigned char)(value >> 16);
    out[3] = (unsigned char)(value >> 24);
}

static void WritePack(const char *path, unsigned char marker, int with_font)
{
    enum { DESCRIPTIONS = 128, FONT_SIZE = 913, COUNT = DESCRIPTIONS + 1 };
    unsigned char header[32] = {0}, index[COUNT * 24] = {0};
    unsigned char font[FONT_SIZE] = {0};
    unsigned char desc[DESCRIPTIONS][2];
    unsigned record_count = DESCRIPTIONS + !!with_font;
    unsigned index_size = record_count * 24;
    unsigned offset = sizeof(header) + index_size;
    FILE *file;

    for (unsigned i = 0; i < DESCRIPTIONS; ++i) {
        unsigned char *entry = index + i * 24;
        desc[i][0] = (unsigned char)('A' + i % 26);
        desc[i][1] = 0;
        Write32(entry, DESCRIPTION);
        Write32(entry + 8, i);
        Write32(entry + 12, offset);
        Write32(entry + 16, sizeof(desc[i]));
        Write32(entry + 20, SDL_crc32(0, desc[i], sizeof(desc[i])));
        offset += sizeof(desc[i]);
    }
    /* 223 valid 4-pixel descriptors cover codes 32..254 and share one bitmap. */
    font[2] = 3; font[3] = 0x84; /* Big-endian table end: 900. */
    for (unsigned i = 0; i < 223; ++i) font[8 + i * 4] = 4;
    font[900] = marker;
    if (with_font) {
        unsigned char *font_entry = index + DESCRIPTIONS * 24;
        Write32(font_entry, FONT);
        Write32(font_entry + 12, offset);
        Write32(font_entry + 16, sizeof(font));
        Write32(font_entry + 20, SDL_crc32(0, font, sizeof(font)));
        offset += sizeof(font);
    }

    memcpy(header, "MGSLOC01", 8);
    Write32(header + 8, 1);
    Write32(header + 12, 2); /* German pack selects its own font. */
    Write32(header + 16, record_count);
    Write32(header + 20, offset);
    Write32(header + 24, SDL_crc32(0, index, index_size));
    file = fopen(path, "wb");
    assert(file);
    assert(fwrite(header, 1, sizeof(header), file) == sizeof(header));
    assert(fwrite(index, 1, index_size, file) == index_size);
    assert(fwrite(desc, 1, sizeof(desc), file) == sizeof(desc));
    if (with_font)
        assert(fwrite(font, 1, sizeof(font), file) == sizeof(font));
    assert(fclose(file) == 0);
}

int main(void)
{
    char first[96], second[96], no_font[96];
    unsigned descriptor = 0;
    volatile unsigned lookup_id = 0, glyph_code = 128;
    volatile uintptr_t sink = 0;
    const unsigned iterations = 500000;
    uint64_t begin, baseline_ticks, cached_ticks;

    snprintf(first, sizeof(first), "language-cache-%d-a.mgslang", (int)test_pid());
    snprintf(second, sizeof(second), "language-cache-%d-b.mgslang", (int)test_pid());
    snprintf(no_font, sizeof(no_font), "language-cache-%d-no-font.mgslang", (int)test_pid());
    WritePack(first, 0x23, 1);
    WritePack(second, 0x6a, 1);
    WritePack(no_font, 0, 0);

    assert(MgsLanguageConfigure(first, NULL));
    unsigned searches_after_configure = search_calls;
    assert(MgsLanguageGlyph(128, &descriptor)[0] == 0x23);
    assert(descriptor == 0x04000000);
    assert(search_calls == searches_after_configure);
    assert(!MgsLanguageGlyph(127, &descriptor));
    assert(!MgsLanguageGlyph(255, &descriptor));

    assert(MgsLanguageConfigure(second, NULL));
    assert(MgsLanguageGlyph(128, &descriptor)[0] == 0x6a);
    assert(MgsLanguageInspect(first)); /* Inspection owns a separate temporary pack. */
    searches_after_configure = search_calls;
    assert(MgsLanguageGlyph(128, &descriptor)[0] == 0x6a);
    assert(search_calls == searches_after_configure);

    begin = SDL_GetPerformanceCounter();
    for (unsigned i = 0; i < iterations; ++i)
        sink ^= (uintptr_t)FindRecord(&text_pack, FONT, 0, lookup_id);
    baseline_ticks = SDL_GetPerformanceCounter() - begin;
    assert(search_calls == searches_after_configure + iterations);
    begin = SDL_GetPerformanceCounter();
    for (unsigned i = 0; i < iterations; ++i)
        sink ^= (uintptr_t)MgsLanguageGlyph(glyph_code, &descriptor);
    cached_ticks = SDL_GetPerformanceCounter() - begin;
    assert(search_calls == searches_after_configure + iterations);
    printf("FONT lookup %u calls: bsearch %.2f ms, cached %.2f ms (sink=%lu)\n",
           iterations,
           baseline_ticks * 1000.0 / SDL_GetPerformanceFrequency(),
           cached_ticks * 1000.0 / SDL_GetPerformanceFrequency(),
           (unsigned long)sink);

    MgsLanguageClose();
    assert(!MgsLanguageGlyph(128, &descriptor));
    /* Every accepted pack requires FONT; rejection must clear the old cache. */
    assert(!MgsLanguageConfigure(no_font, NULL));
    assert(!MgsLanguageGlyph(128, &descriptor));
    assert(MgsLanguageConfigure(second, NULL));
    assert(MgsLanguageGlyph(128, &descriptor)[0] == 0x6a);
    assert(MgsLanguageConfigure(NULL, NULL));
    assert(!MgsLanguageGlyph(128, &descriptor));
    assert(!MgsLanguageConfigure("missing-language-cache-pack.mgslang", NULL));
    assert(!MgsLanguageGlyph(128, &descriptor));
    assert(remove(first) == 0 && remove(second) == 0 && remove(no_font) == 0);
    puts("language font cache lifecycle passed");
    return 0;
}
