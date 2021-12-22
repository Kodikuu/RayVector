void SetFlags(void *);

void BufferShiftAppend(void *bufferA, uint32_t sizeA, void *bufferB, uint32_t sizeB);

// IMM Device Interface

struct audio_device;

uint32_t InitAudio(struct audio_device **);
void FreeAudio(struct audio_device **);

uint32_t IsAudioFloat(struct audio_device *);


void GetAudioSize(struct audio_device *, uint32_t *);
void GetAudioBuffer(struct audio_device *, void *, uint32_t *);