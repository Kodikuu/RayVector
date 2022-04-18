#pragma once
#include <stdint.h>
#include <Audioclient.h>

// init
uint32_t audio_get_default_client(IAudioClient **);
uint32_t audio_get_format(IAudioClient *, WAVEFORMATEX **);
uint32_t audio_start_capture(IAudioClient *, WAVEFORMATEX *, IAudioCaptureClient **);

// runtime
uint32_t audio_get_next_size(IAudioCaptureClient *, uint32_t *);
uint32_t audio_get_next_buffer(IAudioCaptureClient *, void **, uint32_t *, DWORD *);

// free
void audio_free_client(IAudioClient **);
void audio_free_format(WAVEFORMATEX **);
void audio_stop_capture(IAudioClient *, IAudioCaptureClient **);

// runtime free
void audio_free_buffer(IAudioCaptureClient *, uint32_t);
