#include <stdint.h>
#include <stdio.h>

#include <wtypes.h>
#include <WinUser.h>

// IMMDevice Interface
#define CINTERFACE
#define COBJMACROS
#include <initguid.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

#define AUDCLNT_STREAMFLAGS_LOOPBACK                 0x00020000

DEFINE_GUID(OWN_IID_MMDeviceEnumerator,     0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(OWN_IID_IMMDeviceEnumerator,    0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(OWN_IID_IAudioClient,			0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
DEFINE_GUID(OWN_IID_IAudioCaptureClient,    0xC8ADBD64, 0xE71E, 0x48a0, 0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17);

void SetFlags(void *handle) {
    HWND hwnd = (HWND) handle;
    
    LONG exstyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW;
    LONG style = WS_CHILD | WS_VISIBLE;
    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowLong(hwnd, GWL_EXSTYLE, exstyle);

    EnableWindow(hwnd, 0);
}

void BufferShiftAppend(void *bufferA, uint32_t sizeA, void *bufferB, uint32_t sizeB) {
    void *tmp_buf = calloc(1, sizeA);
    memcpy(tmp_buf, bufferA, sizeA);
    memcpy(bufferA, (uint8_t *) tmp_buf+sizeB, sizeA-sizeB);
    memcpy((uint8_t *) bufferA+sizeA-sizeB, bufferB, sizeB);
    free (tmp_buf);
}

struct audio_device {
    IMMDevice *device;
    IAudioClient *audio;
    IAudioCaptureClient *capture;
    uint32_t is_format_float;
};

void FreeAudio(struct audio_device **device_ctx);


uint32_t InitAudio(struct audio_device **device_ctx) {
    struct audio_device *ctx = *device_ctx = calloc(1, sizeof(struct audio_device));

    uint32_t e = CoInitialize(NULL);
    if (e != S_OK)
        return e;

    IMMDeviceEnumerator *imm_enumerator = NULL;
    WAVEFORMATEX *wfx = NULL;

    e = CoCreateInstance(&OWN_IID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &OWN_IID_IMMDeviceEnumerator, &imm_enumerator);
    if (e != S_OK)
        goto except;
    
    e = IMMDeviceEnumerator_GetDefaultAudioEndpoint(imm_enumerator, eRender, eConsole, &ctx->device);
    if (e != S_OK)
        goto except;

    e = IMMDevice_Activate(ctx->device, &OWN_IID_IAudioClient, CLSCTX_ALL, NULL, &ctx->audio);
    if (e != S_OK)
        goto except;
    
    wfx = calloc(1, sizeof(WAVEFORMATEX));
    wfx->cbSize = sizeof(WAVEFORMATEX);
    IAudioClient_GetMixFormat(ctx->audio, &wfx);

    switch (wfx->wFormatTag) {
        case WAVE_FORMAT_PCM:
            ctx->is_format_float = 0;
            break;
        default:
            ctx->is_format_float = 1;
            break;
    }

    e = IAudioClient_Initialize(ctx->audio, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 10000000, 0, wfx, NULL);
    if (e != S_OK)
        goto except;
    
    e = IAudioClient_GetService(ctx->audio, &OWN_IID_IAudioCaptureClient, &ctx->capture);
    if (e != S_OK)
        goto except;
    
    e = IAudioClient_Start(ctx->audio);
    if (e != S_OK)
        goto except;

except:
    if (wfx != NULL)
        free(wfx);

    if (e != S_OK) {
        FreeAudio(device_ctx);
    }

    if (imm_enumerator != NULL)
        IMMDeviceEnumerator_Release(imm_enumerator);
    
    CoUninitialize();
    return e;
}

void FreeAudio(struct audio_device **device_ctx) {
    struct audio_device *ctx = *device_ctx;

    if (ctx->capture) {
        IAudioCaptureClient_Release(ctx->capture);
        ctx->capture = NULL;
    }

    if (ctx->audio) {
        IAudioClient_Stop(ctx->audio);
        IAudioClient_Release(ctx->audio);
        ctx->audio = NULL;

    if (ctx->device) {
        IMMDevice_Release(ctx->device);
        ctx->device = NULL;
    }
    }
}

uint32_t IsAudioFloat(struct audio_device *device_ctx) {
    return device_ctx->is_format_float;
}

void GetAudioSize(struct audio_device *device_ctx, uint32_t *frames) {
    uint32_t e = IAudioCaptureClient_GetNextPacketSize(device_ctx->capture, frames);
    if (e != S_OK)
        printf("Bad Capture: 0x%x\n", e);
}

void GetAudioBuffer(struct audio_device *device_ctx, void *buffer, uint32_t *frames) {
    IAudioCaptureClient_GetNextPacketSize(device_ctx->capture, frames);

    void *buffer_tmp = NULL;
    if (frames) {
        DWORD flags = 0;
        IAudioCaptureClient_GetBuffer(device_ctx->capture, buffer_tmp, frames, &flags, NULL, NULL);

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
            frames = 0;
        
        if (buffer_tmp) {
            memcpy(buffer, buffer_tmp, *frames);
            IAudioCaptureClient_ReleaseBuffer(device_ctx->capture, *frames);
        }
    }
}