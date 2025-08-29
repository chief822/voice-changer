#include "world.h"
#include "build/helpers.h"
#include "miniaudio.h"

#include <stdio.h>
#include <stdlib.h> // imported for dont know yet
#include <string.h> //for memset
#include <stdint.h>

#define BUFFER_SIZE 32768
#define SAMPLE_RATE 48000

typedef struct {
    WorldParameters config;
    double samples[BUFFER_SIZE];
    double processed[BUFFER_SIZE];
} config;

#ifdef __EMSCRIPTEN__
void main_loop__em()
{
}
#endif

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    printf("%d ", frameCount);
    config* data = pDevice->pUserData;
    convertDoubleArrayToFloat(data->processed, pOutput);
    // move this to a worker thread todo
    convertFloatArrayToDouble(pInput, data->samples);
    removeDC(data->samples, BUFFER_SIZE);
    memcpy(data->processed, data->samples, BUFFER_SIZE * sizeof(double));
    process(data->samples, data->processed, &(data->config));
}

int main(int argc, char **argv)
{
    ma_result result;
    ma_device_config deviceConfig;
    ma_device device;
    ma_context context;
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS)
    {
        printf("Failed to initialize context.\n");
    }

    ma_device_info *pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info *pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS)
    {
        printf("Failed to detect devices.\n");
        ma_context_uninit(&context);
        return -1;
    }

    // Loop over each device info and do something with it. You may want
    // to give the user the opportunity to choose which device they'd prefer
    // Find "CABLE Input" device
    const ma_device_id* cableInputID = NULL;
    for (ma_uint32 i = 0; i < playbackCount; ++i) {
        if (strstr(pPlaybackInfos[i].name, "CABLE Input") != NULL) {
            cableInputID = &pPlaybackInfos[i].id;
            // printf("Using device: %s\n", pPlaybackInfos[i].name);
            break;
        }
    }

    if (cableInputID == NULL) {
        printf("CABLE Input device could not be found automatically.\n");
        printf("Please enter device id (left-column) of VB-Audio Virtual Cable:(if not found try restarting your computer)\n");
        for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1)
        {
            printf("%d - %s\n", iDevice, pPlaybackInfos[iDevice].name);
        }
        printf("\n");
        ma_uint32 id;
        scanf("%d", &id);
        while (getchar() != '\n');  // flush leftover newline
        if (!(0 <= id && id < playbackCount)) {
            ma_context_uninit(&context);
            printf("Invalid device ID.\n");
            return -1;
        }
        cableInputID = &pPlaybackInfos[id].id;
    }
    
    deviceConfig = ma_device_config_init(ma_device_type_duplex);
    deviceConfig.capture.pDeviceID = NULL;
    deviceConfig.playback.pDeviceID = cableInputID;
    deviceConfig.sampleRate           = SAMPLE_RATE;  // or match your system's setting
    deviceConfig.wasapi.noAutoConvertSRC = MA_TRUE;  // If using WASAPI
    deviceConfig.wasapi.noDefaultQualitySRC = MA_TRUE;

    
    deviceConfig.capture.format       = ma_format_f32;
    deviceConfig.capture.channels     = 1;
    deviceConfig.capture.shareMode    = ma_share_mode_shared;   
    // exclusive mode is not used because it is causing problems specifically delivering samples in weird but periodic way that weird is causing errors
    deviceConfig.playback.format      = ma_format_f32;
    deviceConfig.playback.channels    = 1;
    deviceConfig.playback.shareMode   = ma_share_mode_shared;

    deviceConfig.periodSizeInFrames = BUFFER_SIZE;
    deviceConfig.dataCallback = data_callback;

    const int tofemale = 1;
    config data;
    setup(&data.config, tofemale);
    memset(data.processed, 0, sizeof(data.processed));
    deviceConfig.pUserData = &data;

    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS)
    {
        ma_context_uninit(&context);
        return result;
    }

#ifdef __EMSCRIPTEN__
    getchar();
#endif

    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start device.\n");
        ma_device_uninit(&device);
        ma_context_uninit(&context);
        return -1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop__em, 0, 1);
#else
    printf("Using output device: %s\n", device.playback.name);
    printf("Using microphone device: %s\n", device.capture.name);
    printf("Press Enter to quit...\n");
    getchar();
#endif
    ma_device_uninit(&device);
    ma_context_uninit(&context);

    (void)argc;
    (void)argv;
    return 0;
}