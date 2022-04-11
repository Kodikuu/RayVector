#pragma once
#include <stdint.h>
#include "kiss_fft130/kiss_fftr.h"
#include "matoya.h"

typedef struct audio_processing {
    // Set in main
    uint32_t fft_ms; // FFT Size in milliseconds of samples
    uint32_t work_running;

    // Used in processing
    uint32_t fft_size;

    uint32_t channels;
    float *raw_capture;
    float *buffer_input;    // Audio Buffer - One per channel, up to 2
    float *fft_input;       // Hanning Window applied - One per channel, up to 2
    kiss_fft_cpx *fft_output_raw;  // Raw FFT
    float *fft_output_filtered;      // Filtered FFT
    double *hanning_window;

    void *internals;
} audio_processing;

typedef struct visualiser {
    // Raylib
    uint32_t position_x;
    uint32_t position_y;
    uint32_t height;
    uint32_t width;

#ifdef Color
    Color colour;
#else
    uint32_t colour;        // Color red value
#endif

    // Shared
    uint32_t bands;
    float *band_data;

    // For Audio Processing
    float *band_freqs;
    uint32_t freq_min;
    uint32_t freq_max;
    uint32_t sensitivity;

} visualiser;

typedef struct context {
    // Global attributes
    uint32_t resolution_w;
    uint32_t resolution_h;
    uint32_t refresh_rate;
    uint32_t running;

    // Visualisers
    uint32_t vis_count;
    visualiser *vis_array;

    // Audio Processing
    audio_processing *processing;

	// Performance and safety
	MTY_Mutex *lock;
} context;