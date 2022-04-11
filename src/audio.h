#include <stdint.h>

typedef struct audio_processing audio_processing;

void audio_destroy(audio_processing *);
uint32_t audio_init(audio_processing *);

void calc_band_freqs(visualiser *vis_ctx);

void work_thread(void *opaque);
