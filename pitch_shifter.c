#include "miniaudio.c"

#define KISS_FFT_USE_ALLOCA
#include "kissfft-master/kiss_fft.h"
#include "kissfft-master/kiss_fftr.h"

#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024
#define PITCH_SHIFT 2
#define SAMPLE_RATE 48000

typedef struct {
    kiss_fftr_cfg fwd_config;
    kiss_fftr_cfg inv_config;
    kiss_fft_cpx* freqDomain;
    kiss_fft_cpx* shiftedFreqDomain;
} FFTContext;

#ifdef __EMSCRIPTEN__
void main_loop__em()
{
}
#endif

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    /* This example assumes the playback and capture sides use the same format and channel count. */
    if (pDevice->capture.format != pDevice->playback.format || pDevice->capture.channels != pDevice->playback.channels)
    {
        return;
    }
    if (frameCount != BUFFER_SIZE || pInput == NULL) {
        // Optional safety check
        return;
    }
    MA_COPY_MEMORY(pOutput, pInput, frameCount * ma_get_bytes_per_frame(pDevice->capture.format, pDevice->capture.channels));
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
    deviceConfig = ma_device_config_init(ma_device_type_duplex);
    deviceConfig.capture.pDeviceID = NULL;
    deviceConfig.playback.pDeviceID = &pPlaybackInfos[id].id;
    deviceConfig.sampleRate           = SAMPLE_RATE;  // or match your system's setting
    deviceConfig.wasapi.noAutoConvertSRC = MA_TRUE;  // If using WASAPI
    deviceConfig.wasapi.noDefaultQualitySRC = MA_TRUE;

    
    deviceConfig.capture.format       = ma_format_f32;
    deviceConfig.capture.channels     = 1;
    deviceConfig.capture.shareMode    = ma_share_mode_exclusive;

    deviceConfig.playback.format      = ma_format_f32;
    deviceConfig.playback.channels    = 1;
    deviceConfig.playback.shareMode = ma_share_mode_exclusive;

    deviceConfig.periodSizeInFrames = BUFFER_SIZE;
    deviceConfig.dataCallback = data_callback;
    FFTContext setup;
    setup.freqDomain = malloc(sizeof(kiss_fft_cpx) * (BUFFER_SIZE / 2 + 1));
    setup.shiftedFreqDomain = malloc(sizeof(kiss_fft_cpx) * (BUFFER_SIZE / 2 + 1));
    setup.fwd_config = kiss_fftr_alloc(BUFFER_SIZE, 0, NULL, NULL); // forward
    setup.inv_config = kiss_fftr_alloc(BUFFER_SIZE, 1, NULL, NULL); // inverse
    if (!setup.freqDomain || !setup.shiftedFreqDomain || !setup.fwd_config || !setup.inv_config) {
        printf("Memory allocation failed.\n");
        return -1;
    }
    deviceConfig.pUserData = &setup;
    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS)
    {
        ma_context_uninit(&context);
        return result;
    }

#ifdef __EMSCRIPTEN__
    getchar();
#endif

    ma_device_start(&device);
    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start device.\n");
        ma_device_uninit(&device);
        ma_context_uninit(&context);
        return -1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop__em, 0, 1);
#else
    printf("\nUsing playback device: %s\n", pPlaybackInfos[id].name);
    printf("Using microphone device: %s\n", device.capture.name);
    printf("Press Enter to quit...\n");
    getchar();
#endif
    
    ma_device_uninit(&device);
    free(setup.freqDomain);
    free(setup.shiftedFreqDomain);
    ma_context_uninit(&context);

    (void)argc;
    (void)argv;
    return 0;
}