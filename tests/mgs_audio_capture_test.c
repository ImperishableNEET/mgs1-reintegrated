#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { unsigned char bytes[16384]; size_t length; } CaptureFile;
static CaptureFile file;
static const char *capture_path;
static int opens, writes, closes, open_failure, write_failure, close_failure;
static const char *TestGetenv(const char *name) {
    assert(!strcmp(name, "PSYZ_AUDIO_CAPTURE_PATH"));
    return capture_path;
}
static CaptureFile *TestOpen(const char *path, const char *mode) {
    assert(!strcmp(path, capture_path) && !strcmp(mode, "wbx"));
    ++opens;
    return open_failure ? NULL : &file;
}
static size_t TestWrite(const void *data, size_t size, size_t count, CaptureFile *out) {
    assert(out == &file && size == 1);
    ++writes;
    if (write_failure) count = 2;
    assert(file.length + count <= sizeof(file.bytes));
    memcpy(file.bytes + file.length, data, count);
    file.length += count;
    return count;
}
static int TestClose(CaptureFile *out) {
    assert(out == &file);
    ++closes;
    return close_failure ? -1 : 0;
}
#define FILE CaptureFile
#define getenv TestGetenv
#define fopen TestOpen
#define fwrite TestWrite
#define fclose TestClose
#define WARNF(...) ((void)0)
#define INFOF(...) ((void)0)
#define BUFFER_FRAMES 2048
#define PSYZ_SPU_SAMPLE_RATE 44100
#include "audio_capture_helpers.inc"

static void Reset(void) {
    assert(!pcm_capture);
    memset(&file, 0, sizeof(file));
    capture_path = NULL;
    opens = writes = closes = open_failure = write_failure = close_failure = 0;
}

int main(void) {
    const short samples[] = {-32768, -1, 0, 32767};
    const unsigned char expected[] = {0, 128, 255, 255, 0, 0, 255, 127};
    Reset();
    OpenPcmCapture(); CapturePcmSamples(samples, 2); ClosePcmCapture();
    assert(!opens && !writes && !closes);
    capture_path = "";
    OpenPcmCapture(); assert(!opens);
    capture_path = "existing.pcm"; open_failure = 1;
    OpenPcmCapture(); CapturePcmSamples(samples, 2);
    assert(opens == 1 && !writes && !closes && !pcm_capture);

    Reset(); capture_path = "new.pcm";
    OpenPcmCapture(); CapturePcmSamples(samples, 0); CapturePcmSamples(samples, 2);
    assert(writes == 1 && file.length == 8 && !memcmp(file.bytes, expected, 8));
    assert(pcm_capture_bytes == 8);
    ClosePcmCapture(); ClosePcmCapture(); assert(closes == 1);

    Reset(); capture_path = "bounded.pcm";
    OpenPcmCapture(); pcm_capture_bytes = PCM_CAPTURE_LIMIT - 4;
    CapturePcmSamples(samples, 2); CapturePcmSamples(samples, 2);
    assert(file.length == 4 && !memcmp(file.bytes, expected, 4));
    assert(pcm_capture_bytes == PCM_CAPTURE_LIMIT && closes == 1 && !pcm_capture);

    Reset(); capture_path = "failed.pcm"; write_failure = close_failure = 1;
    OpenPcmCapture(); CapturePcmSamples(samples, 2); CapturePcmSamples(samples, 2);
    assert(writes == 1 && closes == 1 && pcm_capture_bytes == 2 && !pcm_capture);

    Reset(); capture_path = "chunks.pcm";
    short large[4098] = {0};
    OpenPcmCapture(); CapturePcmSamples(large, 2049);
    assert(writes == 2 && file.length == 8196 && pcm_capture_bytes == 8196);
    ClosePcmCapture();
    puts("PCM capture: disabled, exclusive mode, s16le, cap, failures and chunking passed");
    return 0;
}
