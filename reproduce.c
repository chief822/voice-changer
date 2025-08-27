#include "miniaudio.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    printf("%d", frameCount);
    float* out = (float*)pOutput;
    static float phase = 0.0f;
    float freq = 2000.0f; // A4
    float sampleRate = (float)pDevice->sampleRate;

    for (ma_uint32 i = 0; i < frameCount; i++) {
        float sample = (float)sin(2.0f * 3.14159265f * phase);
        phase += freq / sampleRate;
        if (phase >= 1.0f) phase -= 1.0f;

        out[i + 0] = sample; // left
        out[i+ 1] = sample; // right
    }
}

int main() {
    ma_result result;
    ma_context context;
    ma_device_info* pPlaybackDevices;
    ma_uint32 playbackDeviceCount;

    ma_device_config config;
    ma_device device;

    result = ma_context_init(NULL, 0, NULL, &context);
    if (result != MA_SUCCESS) {
        printf("Failed to init context.\n");
        return -1;
    }

    result = ma_context_get_devices(&context, &pPlaybackDevices, &playbackDeviceCount, NULL, NULL);
    if (result != MA_SUCCESS) {
        printf("Failed to get devices.\n");
        return -1;
    }
    const ma_device_id* cableInputID = NULL;
    for (ma_uint32 i = 0; i < playbackDeviceCount; ++i) {
        if (strstr(pPlaybackDevices[i].name, "CABLE Input") != NULL) {
            cableInputID = &pPlaybackDevices[i].id;
            // printf("Using device: %s\n", pPlaybackInfos[i].name);
            break;
        }
    }

    config = ma_device_config_init(ma_device_type_duplex);
    config.sampleRate = 44100;
    config.capture.pDeviceID = NULL;

    config.capture.format       = ma_format_f32;
    config.capture.channels     = 1;

    config.playback.format = ma_format_f32;
    config.playback.channels = 1;
    config.dataCallback = data_callback;
    config.pUserData = NULL;
    config.playback.pDeviceID = cableInputID;
    config.periodSizeInFrames = 32768;
    config.playback.shareMode   = ma_share_mode_shared;
    
    config.capture.shareMode    = ma_share_mode_shared;



    result = ma_device_init(&context, &config, &device);
    if (result != MA_SUCCESS) {
        printf("Failed to init device.\n");
        return -1;
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        printf("Failed to start device.\n");
        ma_device_uninit(&device);
        return -1;
    }

    printf("Now sending audio to virtual microphone (CABLE Output)...\n");
    getchar(); // Wait until user presses Enter

    ma_device_uninit(&device);
    ma_context_uninit(&context);

    return 0;
}
