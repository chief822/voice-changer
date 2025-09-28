#include "build/dr_wav.h"

#include "worldforfile.h"
#include "build/forfilehelpers.h"

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s input.wav output.wav\n", argv[0]);
        return 1;
    }

    unsigned int channels = 0;
    unsigned int sampleRate = 0;
    drwav_uint64 totalFrameCount = 0;

    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        argv[1], &channels, &sampleRate, &totalFrameCount, NULL
    );

    if (pSampleData == NULL) {
        fprintf(stderr, "Failed to open/read WAV: %s\n", argv[1]);
        return -1;
    }
    if (channels == 0 || sampleRate == 0 || totalFrameCount == 0) {
        fprintf(stderr, "Invalid WAV file metadata (channels=%u, sampleRate=%u, frames=%llu)\n",
                channels, sampleRate, (unsigned long long)totalFrameCount);
        drwav_free(pSampleData, NULL);
        return -1;
    }
    // total samples = frames * channels
    // Check overflow before multiplying
    const drwav_uint64 totalSampleCount64 = totalFrameCount * (drwav_uint64)channels;
    if (totalSampleCount64 > (drwav_uint64)SIZE_MAX) {
        fprintf(stderr, "File too large: total sample count %" PRIu64 " exceeds addressable memory\n", (uint64_t)totalSampleCount64);
        drwav_free(pSampleData, NULL);
        return -1;
    }

    size_t totalSampleCount = (size_t)totalSampleCount64;

    // Prepare output WAV format (float)
    drwav_data_format format;
    format.container     = drwav_container_riff;
    format.format        = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels      = channels;
    format.sampleRate    = sampleRate;
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write(&wav, argv[2], &format, NULL)) {
        fprintf(stderr, "Failed to open output WAV: %s\n", argv[2]);
        drwav_free(pSampleData, NULL);
        return -2;
    }

    int female = 1;

    // NOTE: decide whether your helpers expect samples or frames.
    // You used totalSampleCount previously. If your helpers are per-frame (i.e.
    // they assume sampleCount == totalFrameCount), change the next line to pass (size_t)totalFrameCount.
    WorldParameters config;
    setup(&config, totalSampleCount, female);

    // allocate double buffer safely (check overflow again for bytes)
    size_t bytesNeeded = totalSampleCount;
    if (bytesNeeded > SIZE_MAX / sizeof(double)) {
        fprintf(stderr, "Allocation size overflow\n");
        drwav_uninit(&wav);
        drwav_free(pSampleData, NULL);
        return -3;
    }
    bytesNeeded *= sizeof(double);

    double *buffer = malloc(bytesNeeded);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate %zu bytes for buffer\n", bytesNeeded);
        freeconfig(&config);
        drwav_uninit(&wav);
        drwav_free(pSampleData, NULL);
        return -4;
    }

    clock_t start = clock();

    // Convert, process, convert back
    convertFloatArrayToDouble(pSampleData, buffer, totalSampleCount);
    // If pitchshift expects frame count, call pitchshift(buffer, totalFrameCount, &config);
    pitchshift(buffer, totalSampleCount, &config);
    convertDoubleArrayToFloat(buffer, pSampleData, totalSampleCount);

    // Write frames - drwav_write_pcm_frames expects frame count
    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, totalFrameCount, pSampleData);
    if (framesWritten != totalFrameCount) {
        fprintf(stderr, "Warning: wrote %" PRIu64 " frames but expected %" PRIu64 "\n",
                framesWritten, totalFrameCount);
        // not necessarily fatal, but it's suspicious
    }

    clock_t end = clock();
    double elapsed_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("Elapsed time: %.2f ms\n", elapsed_ms);

    freeconfig(&config);
    drwav_uninit(&wav);
    drwav_free(pSampleData, NULL);
    free(buffer);

    return 0;
}
