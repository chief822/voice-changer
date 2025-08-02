#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    int16_t* out = (int16_t*)pOutput;
    static double phase = 0.0;
    double freq = 110.0;
    double sampleRate = (double)pDevice->sampleRate;

    for (ma_uint32 i = 0; i < frameCount; i++) {
        double value = sin(2.0 * 3.14159265 * phase);
        phase += freq / sampleRate;
        if (phase >= 1.0) phase -= 1.0;

        // Scale float (-1.0 to 1.0) to int16 range
        int16_t sample = (int16_t)(value * 32767.0);

        // Stereo: same sample to both left and right
        out[i * 2 + 0] = sample;
        out[i * 2 + 1] = sample;
    }
}

int main(void) {
    ma_result result;
    ma_context context;
    ma_device_info* pPlaybackDevices;
    ma_uint32 playbackDeviceCount;

    result = ma_context_init(NULL, 0, NULL, &context);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize miniaudio context.\n");
        return -1;
    }

    // Get playback devices
    result = ma_context_get_devices(&context, &pPlaybackDevices, &playbackDeviceCount, NULL, NULL);
    if (result != MA_SUCCESS) {
        printf("Failed to get playback devices.\n");
        ma_context_uninit(&context);
        return -1;
    }

    // Find "CABLE Input" device
    const ma_device_id* cableInputID = NULL;
    for (ma_uint32 i = 0; i < playbackDeviceCount; ++i) {
        printf("Playback Device %d: %s\n", i, pPlaybackDevices[i].name);
        if (strstr(pPlaybackDevices[i].name, "CABLE Input") != NULL) {
            cableInputID = &pPlaybackDevices[i].id;
            printf("✅ Using device: %s\n", pPlaybackDevices[i].name);
            break;
        }
    }

    if (cableInputID == NULL) {
        printf("❌ 'CABLE Input' device not found.\n");
        ma_context_uninit(&context);
        return -1;
    }

    // Configure and start playback device
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.sampleRate = 44100;
    config.playback.format = ma_format_s16;     // 16-bit signed integer
    config.playback.channels = 2;               // stereo
    config.playback.pDeviceID = cableInputID;
    config.dataCallback = data_callback;
    config.pUserData = NULL;

    ma_device device;
    result = ma_device_init(&context, &config, &device);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize playback device.\n");
        ma_context_uninit(&context);
        return -1;
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        printf("Failed to start playback device.\n");
        ma_device_uninit(&device);
        ma_context_uninit(&context);
        return -1;
    }

    printf(" Sending sine wave to VB-Audio Cable (virtual mic)...\n");
    printf(" Use 'CABLE Output' as your mic in apps (OBS, Discord, etc.)\n");
    printf(" Press Enter to stop...\n");
    getchar();

    ma_device_uninit(&device);
    ma_context_uninit(&context);
    return 0;
}
