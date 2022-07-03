#pragma once
#include <stdint.h>
#include "kiss_fft130/kiss_fftr.h"

#pragma warning(disable: 4201)
#include "matoya.h"

struct audio_processing {
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
};

struct visualiser {
	// Raylib
	uint32_t height;

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
};

struct context {
	// Global attributes
	bool running;

	// Visualisers
	uint32_t vis_count;
	struct visualiser *vis_array;

	// Audio Processing
	struct audio_processing *processing;

	// Performance and safety
	MTY_Mutex *lock;
};

struct display {
	// Absolute Position of a display
	int32_t abs_x;
	int32_t abs_y;

	// Relative position of a display
	uint32_t rel_x;
	uint32_t rel_y;

	// Dimensions
	uint32_t width;
	uint32_t height;
};

struct display_list {
	uint16_t disp_count;
	uint16_t array_size;
	struct display *disp_array;
};

struct bounds {
	// X Bounds
	int32_t x_min;
	int32_t x_max;

	// Y Bounds
	int32_t y_min;
	int32_t y_max;

	// Dimensions
	uint32_t width;
	uint32_t height;

	// Displays
	struct display_list *displays; 
};