#include "build/yin.c"
#include "build/webrtc_vad.h"
#define DR_WAV_IMPLEMENTATION
#include "build/dr_wav.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define yin_SAMPLE_SIZE 2048

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./pitch input.wav\n");
        return 1;
    }   
 
    drwav wav;
    if (!drwav_init_file(&wav, argv[1], NULL)) {
        printf("Failed to open WAV file.\n");
        return 1;
    }


    int16_t buffer[yin_SAMPLE_SIZE];
    float pitchs[2000];
    int i = 0;
    Yin yin;
    Yin_init(&yin, yin_SAMPLE_SIZE, 0.2f);

    VadInst* handle = WebRtcVad_Create(); 
    if (WebRtcVad_Init(handle) != 0) {
        printf("Webrtc VAD failed/\n");
    }
    if (WebRtcVad_set_mode(handle, 1) != 0) {
        printf("Webrtc VAD failed/\n");
    }
    int not_found = 0;
    clock_t start = clock();
    while (1) {
        drwav_uint64 framesRead = drwav_read_pcm_frames_s16(&wav, yin_SAMPLE_SIZE, buffer);
        if (framesRead == 0)
            break;
        // to deal with limited support of webrtc vad, middle of buffer is passed to it
        if (WebRtcVad_Process(handle, 48000, buffer+304, 1440) != 1) {
            printf("i: %d\n", i);
            continue;
        } 
        removeDC(buffer, yin_SAMPLE_SIZE);
		pitchs[i] = Yin_getPeriod(&yin, buffer);
        printf("pitch: %f, i: %d\n", pitchs[i], i);
        if (pitchs[i] == -1) {
            not_found++;
            continue;
        }
        ++i;
    }
    clock_t end = clock();
    double elapsed_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;printf("Elapsed time: %.2f ms\n", elapsed_ms);
    printf("\npitchs: %d\nnot found: %d\nMean: %lf\nMedian: %lf\n", i, not_found, calculateMean(pitchs, i), calculateMedian(pitchs, i));
    WebRtcVad_Free(handle);
    drwav_uninit(&wav);
    return 0;
}
