#include <stdint.h>
#include <stdio.h>

/*
==============
	RENDER
==============
*/
#include <wtypes.h>
#include <WinUser.h>

void SetFlags(void *handle) {
	HWND hwnd = (HWND) handle;
	
	LONG exstyle = WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW;
	LONG style = WS_CHILD | WS_VISIBLE;
	SetWindowLong(hwnd, GWL_STYLE, style);
	SetWindowLong(hwnd, GWL_EXSTYLE, exstyle);

	EnableWindow(hwnd, 0);
}

/*
=============
	AUDIO
=============
*/

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

void audio_free_client(IAudioClient **audio_client)
{
	if (*audio_client) {
		IAudioClient_Release(*audio_client);
		*audio_client = NULL;
	}
}

uint32_t audio_get_default_client(IAudioClient **audio_client_out)
{
	uint32_t e = CoInitialize(NULL);
	if (e != S_OK)
		return e;

	IMMDeviceEnumerator *imm_enumerator = NULL;
	IMMDevice *imm_device = NULL;

	e = CoCreateInstance(&OWN_IID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &OWN_IID_IMMDeviceEnumerator, &imm_enumerator);
	if (e != S_OK)
		goto except;
	
	e = IMMDeviceEnumerator_GetDefaultAudioEndpoint(imm_enumerator, eRender, eConsole, &imm_device);
	if (e != S_OK)
		goto except;

	e = IMMDevice_Activate(imm_device, &OWN_IID_IAudioClient, CLSCTX_ALL, NULL, audio_client_out);
	if (e != S_OK)
		goto except;
	
except:
	if (e != S_OK && *audio_client_out)
		audio_free_client(audio_client_out);

	if (imm_device)
		IMMDevice_Release(imm_device);
	
	if (imm_enumerator)
		IMMDeviceEnumerator_Release(imm_enumerator);
	
	CoUninitialize();
	return e;
}

void audio_free_format(WAVEFORMATEX **wfx)
{
	free(*wfx);
	*wfx = NULL;
}

uint32_t audio_get_format(IAudioClient *audio_client, WAVEFORMATEX **wfx_out)
{
	WAVEFORMATEX *wfx = *wfx_out = calloc(1, sizeof(WAVEFORMATEX));
	wfx->cbSize = sizeof(WAVEFORMATEX);

	uint32_t e = IAudioClient_GetMixFormat(audio_client, wfx_out);
	if (e != S_OK)
		audio_free_format(wfx_out);

	return e;
}

void audio_stop_capture(IAudioClient *audio_client, IAudioCaptureClient **capture_client)
{
	IAudioClient_Stop(audio_client);
	if (*capture_client) {
		IAudioCaptureClient_Release(*capture_client);
		*capture_client = NULL;
	}
}

uint32_t audio_start_capture(IAudioClient *audio_client, WAVEFORMATEX *wfx, IAudioCaptureClient **capture_client)
{
	uint32_t e = IAudioClient_Initialize(audio_client, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 100000, 0, wfx, NULL);
	if (e != S_OK)
		goto except;
	
	e = IAudioClient_GetService(audio_client, &OWN_IID_IAudioCaptureClient, capture_client);
	if (e != S_OK)
		goto except;
	
	e = IAudioClient_Start(audio_client);
	if (e != S_OK)
		goto except;

except:
	if (e != S_OK)
		audio_stop_capture(audio_client, capture_client);
	return e;
}

uint32_t audio_get_next_size(IAudioCaptureClient *capture_client, uint32_t *frames)
{
	return IAudioCaptureClient_GetNextPacketSize(capture_client, frames);
}

uint32_t audio_get_next_buffer(IAudioCaptureClient *capture_client, void **buffer, uint32_t *frames, DWORD *flags)
{
	return IAudioCaptureClient_GetBuffer(capture_client, (BYTE **) buffer, frames, flags, NULL, NULL);
}

void audio_free_buffer(IAudioCaptureClient *capture_client, uint32_t frames)
{
	IAudioCaptureClient_ReleaseBuffer(capture_client, frames);
}
