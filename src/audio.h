#pragma once
#include "common.h"

void audio_destroy(struct audio_processing *);
uint32_t audio_init(struct audio_processing *);

void calc_band_freqs(struct visualiser *vis_ctx);

void work_thread(void *opaque);
