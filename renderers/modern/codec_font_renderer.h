// SPDX-License-Identifier: MPL-2.0
/* Optional private Codec atlas rendering. Included by sdl3_gpu.c so that the
 * replacement participates in its existing OT, scissor and VRAM bookkeeping. */
typedef struct CodecFontSpan {
    unsigned char x, y, width, coverage;
} CodecFontSpan;

typedef struct CoverageFontAtlas {
    CodecFontSpan *spans;
    unsigned first[96], count[95];
} CoverageFontAtlas;
static CoverageFontAtlas codec_atlas, interface_atlas;
#define codec_font_spans codec_atlas.spans
#define codec_font_first codec_atlas.first
#define codec_font_count codec_atlas.count
static bool codec_font_gpu_failed;
static bool codec_font_draw_logged;

static void FreeCodecFontAtlas(void) {
    SDL_free(codec_font_spans);
    codec_font_spans = NULL;
    memset(codec_font_count, 0, sizeof(codec_font_count));
    SDL_free(interface_atlas.spans);
    memset(&interface_atlas, 0, sizeof(interface_atlas));
}

static bool LoadCoverageFontAtlas(const char *environment, CoverageFontAtlas *font) {
    const char* path = SDL_getenv(environment);
    if (!path || !*path) return false;
    SDL_Surface* loaded = SDL_LoadBMP(path);
    if (!loaded) {
        WARNF("Codec font atlas unavailable; using original text: %s", SDL_GetError());
        return false;
    }
    SDL_Surface* atlas = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!atlas || atlas->w != 2048 || atlas->h != 672) {
        WARNF("Codec font atlas must be a 2048x672 grayscale BMP; using original text");
        SDL_DestroySurface(atlas);
        return false;
    }
    /* ASCII32..126, sixteen columns, 128x112 cells, eight samples per game
     * pixel. The atlas already includes its vertical bearings. */
    CodecFontSpan* spans = SDL_malloc(95u * 128u * 112u * sizeof(*spans));
    if (!spans || !SDL_LockSurface(atlas)) {
        SDL_free(spans);
        SDL_DestroySurface(atlas);
        return false;
    }
    CoverageFontAtlas candidate = {0};
    unsigned count = 0;
    bool valid = true;
    for (unsigned glyph = 0; glyph < 95 && valid; ++glyph) {
        candidate.first[glyph] = count;
        for (int y = 0; y < 112 && valid; ++y) {
            const u8* row = (const u8*)atlas->pixels +
                ((glyph / 16) * 112 + y) * atlas->pitch + (glyph % 16) * 128 * 4;
            for (int x = 0; x < 128;) {
                const u8 coverage = row[x * 4];
                if (coverage != row[x * 4 + 1] || coverage != row[x * 4 + 2]) {
                    valid = false;
                    break;
                }
                int end = x + 1;
                while (end < 128 && row[end * 4] == coverage &&
                       row[end * 4 + 1] == coverage && row[end * 4 + 2] == coverage)
                    ++end;
                if (coverage)
                    spans[count++] = (CodecFontSpan){(u8)x, (u8)y, (u8)(end - x), coverage};
                x = end;
            }
        }
        candidate.count[glyph] = count - candidate.first[glyph];
    }
    SDL_UnlockSurface(atlas);
    SDL_DestroySurface(atlas);
    if (!valid) {
        SDL_free(spans);
        WARNF("Codec font atlas is not grayscale; using original text");
        return false;
    }
    /* Parsing reserves the worst case; retain only the spans actually used.
     * A failed shrink leaves the valid allocation intact. Keep one element for
     * an empty atlas so its existing non-null/fallback behavior is unchanged. */
    CodecFontSpan* compact = SDL_realloc(spans, (count ? count : 1u) * sizeof(*spans));
    if (compact) spans = compact;
    SDL_free(font->spans);
    /* Restore counts from offsets after successful complete validation. */
    candidate.first[95] = count;
    for (unsigned i = 0; i < 95; ++i)
        candidate.count[i] = candidate.first[i + 1] - candidate.first[i];
    candidate.spans = spans;
    *font = candidate;
    codec_font_gpu_failed = false;
    codec_font_draw_logged = false;
    INFOF("optional coverage font atlas loaded (%u coverage spans)", count);
    return true;
}

static bool LoadCodecFontAtlas(void) {
    bool codec = LoadCoverageFontAtlas("REI_CODEC_FONT_ATLAS", &codec_atlas);
    bool interface_font = LoadCoverageFontAtlas("REI_INTERFACE_FONT_ATLAS", &interface_atlas);
    return codec || interface_font;
}

static bool DrawCodecFontChunk(const Vertex* vertices,
                              unsigned count, const SDL_GPUViewport* viewport,
                              const SDL_Rect* clip, const SDL_Rect* dirty, bool additive) {
    SDL_GPUCommandBuffer* cmd = AcquireCmd();
    void* mapped = SDL_MapGPUTransferBuffer(device, presentation_transfer, true);
    if (!mapped) return false;
    memcpy(mapped, vertices, count * 2 * sizeof(*vertices));
    SDL_UnmapGPUTransferBuffer(device, presentation_transfer);
    SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
    if (!copy) return false;
    const SDL_GPUTransferBufferLocation source = {.transfer_buffer = presentation_transfer};
    const SDL_GPUBufferRegion destination = {
        .buffer = presentation_vbuf, .size = count * 2 * sizeof(*vertices)};
    SDL_UploadToGPUBuffer(copy, &source, &destination, true);
    SDL_EndGPUCopyPass(copy);
    const SDL_GPUColorTargetInfo target = {
        .texture = GetRenderTarget(), .load_op = SDL_GPU_LOADOP_LOAD,
        .store_op = SDL_GPU_STOREOP_STORE};
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, NULL);
    if (!pass) return false;
    SDL_SetGPUViewport(pass, viewport);
    SDL_SetGPUScissor(pass, clip);
    SDL_BindGPUGraphicsPipeline(pass, additive ? pipe_codec_font_add : pipe_codec_font);
    const SDL_GPUBufferBinding binding = {.buffer = presentation_vbuf};
    SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
    SDL_DrawGPUPrimitives(pass, count, 1, 0, 0);
    /* Coverage blends RGB, but PS1 alpha is a discrete VRAM mask bit. */
    SDL_BindGPUGraphicsPipeline(pass, pipe_codec_mask);
    SDL_DrawGPUPrimitives(pass, count, 1, count, 0);
    ++gpu_work.font_chunks;
    gpu_work.draws += 2;
    gpu_work.vertices += count * 2;
    SDL_EndGPURenderPass(pass);
    if (internal_res > 1) {
        MarkNativeResolve(*dirty);
        MarkScaledVramDirty(*dirty);
    } else MarkVramDirty(*dirty);
    YieldGpuWorkIfNeeded();
    return true;
}

static bool DrawCodecFontSprite(const MgsHiResFontSnapshot* text,
                               const u_long* packet, int length, const CoverageFontAtlas *font) {
    const unsigned opcode = packet[0] >> 24;
    const bool additive = opcode == 0x66;
    if (codec_font_gpu_failed || !pipe_codec_font || !pipe_codec_font_add || !pipe_codec_mask || !font->spans || !presentation_vbuf ||
        !presentation_transfer || length < 4 || (opcode != 0x64 && !additive) ||
        preserve_masked || !text->count ||
        text->count > MGS_HIRES_FONT_MAX_GLYPHS || text->width <= 0 || text->height <= 0)
        return false;
    /* Validate the complete sprite before suppressing any original text. */
    for (unsigned i = 0; i < text->count; ++i) {
        const MgsHiResGlyph* glyph = &text->glyphs[i];
        if (glyph->code < 32 || glyph->code > 126 || glyph->color > 3 ||
            text->palette[glyph->color * 4] != 0 ||
            text->palette[glyph->color * 4 + 3] == 0 ||
            (glyph->x - text->source_x) * 8 < -32768 ||
            (glyph->x - text->source_x) * 8 + 128 > 32767 ||
            (glyph->y - text->source_y) * 8 < -32768 ||
            (glyph->y - text->source_y) * 8 + 112 > 32767 ||
            (glyph->code != ' ' && !font->count[glyph->code - 32])) return false;
    }
    const float transform = frame_transform_bypass ? 1.0f : frame_scale_x;
    const float grid = GetDrawGridXScale();
    const float scale_x = grid * transform;
    if (scale_x <= 0) return false;
    const float origin_x = (text->x * grid + draw_offset.x - frame_centre_x) *
                           transform + frame_centre_x;
    const float origin_y = text->y + draw_offset.y;
    SDL_Rect board = {(int)floorf(origin_x), (int)floorf(origin_y),
        (int)ceilf(origin_x + text->width * scale_x) - (int)floorf(origin_x), text->height};
    SDL_Rect dirty;
    if (!SDL_GetRectIntersection(&board, &scissor_rect, &dirty)) return true;
    SDL_Rect target_bounds = {0, 0, VRAM_W, VRAM_H};
    if (!SDL_GetRectIntersection(&dirty, &target_bounds, &dirty)) return true;
    const SDL_Rect clip = {dirty.x * (int)internal_res, dirty.y * (int)internal_res,
        dirty.w * (int)internal_res, dirty.h * (int)internal_res};
    Draw_FlushBuffer();
    Vertex* vertices = presentation_vertices;
    bool drew = false;
    enum { batch_spans = PRESENTATION_VERTEX_CAPACITY / 12 };
    Vertex *mask_vertices = vertices + batch_spans * 6;
    unsigned queued = 0;
    /* Clear shaders clip to a 1024x512 coordinate domain. A band of 112x48
     * logical pixels leaves room for each 16x14 atlas cell without NDC clipping.
     * Band changes flush in original glyph order, preserving overlapping ink. */
    int band_x = 0, band_y = 0;
    SDL_GPUViewport viewport = {
        .x = origin_x * internal_res, .y = origin_y * internal_res,
        .w = VRAM_W * internal_res * scale_x / 8.0f,
        .h = VRAM_H * internal_res / 8.0f,
        .min_depth = 0, .max_depth = 1};
    for (unsigned i = 0; i < text->count; ++i) {
        const MgsHiResGlyph* glyph = &text->glyphs[i];
        /* A contact sprite selects one of eight rows from a shared KCB.
         * Draw only glyphs placed in this row, never the adjacent names. */
        if (glyph->x < text->source_x ||
            glyph->x >= text->source_x + text->width ||
            glyph->y < text->source_y ||
            glyph->y >= text->source_y + text->height) continue;
        unsigned index = glyph->code - 32;
        unsigned remaining = font->count[index], offset = font->first[index];
        unsigned short color = text->palette[glyph->color * 4 + 3];
        const u8 r = (u8)(CLAMP(((color & 31) * (packet[0] & 255)) / 128, 0, 31) * 255 / 31);
        const u8 g = (u8)(CLAMP((((color >> 5) & 31) * ((packet[0] >> 8) & 255)) / 128, 0, 31) * 255 / 31);
        const u8 b = (u8)(CLAMP((((color >> 10) & 31) * ((packet[0] >> 16) & 255)) / 128, 0, 31) * 255 / 31);
        int next_x = ((glyph->x - text->source_x) / 112) * 112;
        int next_y = ((glyph->y - text->source_y) / 48) * 48;
        if (remaining && (next_x != band_x || next_y != band_y)) {
            if (queued) {
                memmove(vertices + queued * 6, mask_vertices, queued * 6 * sizeof(*vertices));
                if (!DrawCodecFontChunk(vertices, queued * 6, &viewport, &clip, &dirty, additive)) {
                    WARNF("Codec font GPU upload failed: %s", SDL_GetError());
                    codec_font_gpu_failed = true;
                    if (!drew) return false;
                    goto completed;
                }
                drew = true; queued = 0;
            }
            band_x = next_x; band_y = next_y;
            viewport.x = (origin_x + band_x * scale_x) * internal_res;
            viewport.y = (origin_y + band_y) * internal_res;
        }
        while (remaining) {
            const CodecFontSpan* run = &font->spans[offset];
            Vertex *body = vertices + queued * 6;
            Vertex *mask = mask_vertices + queued * 6;
            PresentationRect(body,
                (glyph->x - text->source_x - band_x) * 8 + run->x,
                (glyph->y - text->source_y - band_y) * 8 + run->y,
                run->width, 1, r, g, b, run->coverage);
            memcpy(mask, body, 6 * sizeof(*mask));
            for (unsigned j = 0; j < 6; ++j)
                mask[j].a = force_mask_bit || (color & 0x8000) ? 255 : 0;
            ++queued; --remaining; ++offset;
            if (queued == batch_spans) {
                memmove(vertices + queued * 6, mask_vertices, queued * 6 * sizeof(*vertices));
                if (!DrawCodecFontChunk(vertices, queued * 6, &viewport, &clip, &dirty, additive)) {
                    WARNF("Codec font GPU upload failed: %s", SDL_GetError());
                    codec_font_gpu_failed = true;
                    if (!drew) return false;
                    goto completed;
                }
                drew = true;
                queued = 0;
            }
        }
    }
    if (queued) {
        memmove(vertices + queued * 6, mask_vertices, queued * 6 * sizeof(*vertices));
        if (!DrawCodecFontChunk(vertices, queued * 6, &viewport, &clip, &dirty, additive)) {
            WARNF("Codec font GPU upload failed: %s", SDL_GetError());
            codec_font_gpu_failed = true;
            if (!drew) return false;
        } else drew = true;
    }
completed:
    if (drew) {
        if (!codec_font_draw_logged) {
            INFOF("optional high-resolution Codec text drawn (%u glyphs)", text->count);
            codec_font_draw_logged = true;
        }
        if (internal_res > 1) {
            MarkNativeResolve(dirty);
            MarkScaledVramDirty(dirty);
        } else MarkVramDirty(dirty);
    }
    return true;
}
