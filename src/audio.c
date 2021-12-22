#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define _USE_MATH_DEFINES
#include "math.h"

#include "common.h"

#include "winlib.h"
#include "kiss_fft130/kiss_fftr.h"

#define SAMPLES_PER_SEC 48000

typedef struct internal {
    IAudioClient *audio;
    WAVEFORMATEX *wfx;
    IAudioCaptureClient *capture;

    kiss_fftr_cfg fft_cfg;
} internal;

void audio_destroy(audio_processing *audio_ctx) {
    internal *internals = audio_ctx->internals;
    if (audio_ctx->internals) {
        if (internals->capture)
            audio_stop_capture(internals->audio, &internals->capture);
        if (internals->wfx)
            audio_free_format(&internals->wfx);
        if (internals->audio)
            audio_free_client(&internals->audio);

        free(audio_ctx->internals);
        audio_ctx->internals = NULL;
    }

    if (internals->fft_cfg) {
        kiss_fftr_free(internals->fft_cfg);
    }

    if (audio_ctx->buffer_input) {
        free(audio_ctx->buffer_input);
        audio_ctx->buffer_input = NULL;
    }
    if (audio_ctx->fft_input) {
        free(audio_ctx->fft_input);
        audio_ctx->fft_input = NULL;
    }

    if (audio_ctx->raw_capture) {
        free(audio_ctx->raw_capture);
        audio_ctx->raw_capture = NULL;
    }

    if (audio_ctx->fft_output_raw) {
        free(audio_ctx->fft_output_raw);
        audio_ctx->fft_output_raw = NULL;
    }

    if (audio_ctx->fft_output_filtered) {
        free(audio_ctx->fft_output_filtered);
        audio_ctx->fft_output_filtered = NULL;
    }

    if (audio_ctx->hanning_window) {
        free(audio_ctx->hanning_window);
        audio_ctx->hanning_window = NULL;
    }
}

void calc_hanning_window(audio_processing *audio_ctx);

uint32_t audio_init(audio_processing **audio_ctx) {
    uint32_t e = 0;

    audio_processing *ctx = *audio_ctx;

    if (ctx->internals) {
        audio_destroy(ctx);
    }
    internal *internals = (*audio_ctx)->internals = calloc(1, sizeof(internal));

    e = audio_get_default_client(&internals->audio);
    if (e != S_OK)
        goto except;

    e = audio_get_format(internals->audio, &internals->wfx);
    if (e != S_OK)
        goto except;

    e = audio_start_capture(internals->audio, internals->wfx, &internals->capture);
    if (e != S_OK)
        goto except;

    ctx->fft_size = internals->wfx->nSamplesPerSec * ctx->fft_ms / 1000;
    ctx->channels = internals->wfx->nChannels >= 2 ? 2 : internals->wfx->nChannels;

    internals->fft_cfg = kiss_fftr_alloc(ctx->fft_size, 0, NULL, NULL);

    ctx->buffer_input = calloc(ctx->fft_size, sizeof(float));
    ctx->fft_input = calloc(ctx->fft_size, sizeof(float));
    ctx->raw_capture = calloc(ctx->fft_size*ctx->channels, sizeof(kiss_fft_cpx));
    ctx->fft_output_raw = calloc(ctx->fft_size, sizeof(kiss_fft_cpx));

    ctx->fft_output_filtered = calloc(ctx->fft_size, sizeof(float));
    ctx->hanning_window = calloc(ctx->fft_size, sizeof(double));
    calc_hanning_window(ctx);

    
except:
    if (e != S_OK) {
        audio_destroy(ctx);
    }

    return e;
}

void BufferShiftFloat(float **bufferA, uint32_t sizeA, uint32_t sizeB) {
    
    float *tmp_buf = calloc(1, sizeA * sizeof(float));
    memcpy(tmp_buf, *bufferA, sizeA * sizeof(float));
    memcpy(*bufferA, (float *) tmp_buf+sizeB, (sizeA-sizeB) * sizeof(float));
    free (tmp_buf);
}

void BufferDemux(float *bufferA, float* bufferB, uint32_t sizeA) {
    for (uint32_t i = 0; i < sizeA; i++) {
        bufferB[i] = (bufferA[i*2] + bufferA[i*2 + 1]) / 2;
    }
}

void calc_hanning_window(audio_processing *audio_ctx) {
    for(uint32_t bin = 0; bin < audio_ctx->fft_size; bin++) {
        audio_ctx->hanning_window[bin]	= (double) (0.5 * (1.0 - cos(M_PI*2*bin/(audio_ctx->fft_size-1))));
    }
}

void calc_band_freqs(visualiser *vis_ctx) {
    if (vis_ctx->band_freqs)
        free(vis_ctx->band_freqs);

    vis_ctx->band_freqs = calloc(vis_ctx->bands, sizeof(float));

	const double step	= (log(vis_ctx->freq_max/vis_ctx->freq_min) / vis_ctx->bands) / log(2.0);
    vis_ctx->band_freqs[0]		= (float) (vis_ctx->freq_min * pow(2.0, step/2.0));

    for(uint32_t band = 1; band < vis_ctx->bands; ++band) {
        vis_ctx->band_freqs[band] = (float) (vis_ctx->band_freqs[band-1] * pow(2.0, step));
    }
}

void apply_hanning(audio_processing *audio_ctx) {

    for (uint32_t i = 0; i<audio_ctx->fft_size; i++) {
        audio_ctx->fft_input[i] = audio_ctx->buffer_input[i] * audio_ctx->hanning_window[i];
    }
}

void apply_fft_filter(audio_processing *audio_ctx) {
    double kfft_a = exp(-2 / (0.1 * 1));
    double kfft_d = exp(-2 / (0.1 * 1));
    double scalar = 1 / sqrt(audio_ctx->fft_size);
    memset(audio_ctx->fft_output_filtered, 0, audio_ctx->fft_size * sizeof(float));
    for (uint32_t bin = 0; bin < audio_ctx->fft_size; bin++) {
        double x0 = audio_ctx->fft_output_filtered[bin];
        kiss_fft_cpx piece = audio_ctx->fft_output_raw[bin];
        double x1 = (piece.r*piece.r + piece.i*piece.i) * scalar;
        if (x0<x1) {
            x0 = x1 + kfft_a*(x0-x1);
        } else {
            x0 = x1 + kfft_d*(x0-x1);
        }
        audio_ctx->fft_output_filtered[bin] = x0;
    }
}

void apply_fft_binning(audio_processing *audio_ctx, visualiser *vis_ctx) {
    const float df = 10.0f;
    const float scalar = 2.0f / SAMPLES_PER_SEC;

    memset(vis_ctx->band_data, 0, sizeof(float)*vis_ctx->bands);
    uint32_t bin = 1;
    uint32_t band = 0;
    float f0 = 0.0f;

    while ((bin <= audio_ctx->fft_size) && band < vis_ctx->bands) {
        float fLin1 = ((float)bin+0.5f)*df;
        float fLog1 = vis_ctx->band_freqs[band];

        float x = audio_ctx->fft_output_filtered[bin];
        if (fLin1 < vis_ctx->freq_min) {
            f0 = fLin1;
            bin += 1;
        } else if(fLin1 <= fLog1) {
            vis_ctx->band_data[band] += (fLin1-f0) * x * scalar;
            f0 = fLin1;
            bin += 1;
        } else {
            vis_ctx->band_data[band] += (fLog1-f0) * x * scalar;
            f0			= fLog1;
            band		+= 1;
        }
    }
}

void apply_sensitivity(audio_processing *audio_ctx, visualiser *vis_ctx) {
    for (uint32_t i = 0; i < vis_ctx->bands; i++) {
        vis_ctx->band_data[i] = max(0.0f, min(1.0f, vis_ctx->band_data[i])); // Limit domain
        vis_ctx->band_data[i] = max(0.0, (10.0/vis_ctx->sensitivity)*log10(vis_ctx->band_data[i])+1.0); // Sensitivity
    }
}

void work_thread(void *opaque) {
    context *ctx = (context *) opaque;
    ctx->processing->work_running = 1;

    uint32_t e = audio_init(&ctx->processing);
    internal *internals = ctx->processing->internals;
    while (ctx->running && e == S_OK) {
        // capture
        uint32_t new_frames = 0;
        audio_get_next_size(internals->capture, &new_frames);

        if (!new_frames) {
            Sleep(1);
            continue;
        }

        BufferShiftFloat((float **) &ctx->processing->buffer_input, ctx->processing->fft_size, new_frames);
        float *tmp_buf = NULL;
        DWORD flags = 0;
        audio_get_next_buffer(internals->capture, &tmp_buf, &new_frames, &flags);
        
        uint32_t shift_val = ctx->processing->fft_size < new_frames ? ctx->processing->fft_size : new_frames;

        if (ctx->processing->channels == 2) {
            BufferDemux(tmp_buf, ctx->processing->buffer_input+(ctx->processing->fft_size-shift_val), shift_val);
        } else {
            memcpy(ctx->processing->buffer_input+(ctx->processing->fft_size-shift_val), tmp_buf, shift_val);
        }
        audio_free_buffer(internals->capture, new_frames);

        // fft process
        for (uint32_t i = 0; i < ctx->processing->channels; i++) {
            apply_hanning(ctx->processing);
            memset(ctx->processing->fft_output_raw, 0, ctx->processing->fft_size * sizeof(kiss_fft_cpx));
            kiss_fftr(internals->fft_cfg, ctx->processing->fft_input, ctx->processing->fft_output_raw);
            apply_fft_filter(ctx->processing);

        }

        for (uint32_t k = 0; k < ctx->vis_count; k++) {
            apply_fft_binning(ctx->processing, &ctx->vis_array[k]);
            apply_sensitivity(ctx->processing, &ctx->vis_array[k]);
        }
    }
    audio_destroy(ctx->processing);
    ctx->processing->work_running = 0;
}