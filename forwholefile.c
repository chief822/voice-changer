#define DR_WAV_IMPLEMENTATION // too lazy to do it in separate file that's why
#include "build/dr_wav.h"

#include "worldforfile.h"

#include "build/forfilehelpers.h"
#include <stdint.h>
#include <stddef.h>
#include <math.h> 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
 
int main(int argc, char *argv[])
{
    // Check command-line arguments
    if (argc != 3)
    {
        printf("Usage: ./volume input.wav output.wav factor\n");
        return 1;
    }

    // Open files and determine scaling factor
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalFrameCount;

    // Load into 32-bit floats (recommended).
    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        argv[1], &channels, &sampleRate, &totalFrameCount, NULL
    );

    if (pSampleData == NULL) {
        return -1; // Failed to read
    }

    drwav_uint64 totalSampleCount = totalFrameCount * channels;

    // ---- Write to new WAV file
    drwav_data_format format;
    format.container     = drwav_container_riff; // standard WAV
    format.format        = DR_WAVE_FORMAT_IEEE_FLOAT; // since we used float
    format.channels      = channels;
    format.sampleRate    = sampleRate;
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write(&wav, argv[2], &format, NULL)) {
        drwav_free(pSampleData, NULL);
        return -2; // Failed to open output file
    }

    int female = 1;

    WorldParameters config;
    setup(&config, totalSampleCount, female);
    double *buffer = malloc(totalSampleCount * sizeof(double));
    clock_t start = clock();

    convertFloatArrayToDouble(pSampleData, buffer, totalSampleCount);
    pitchshift(buffer, totalSampleCount, &config);
    convertDoubleArrayToFloat(buffer, pSampleData, totalSampleCount);
    drwav_write_pcm_frames(&wav, totalFrameCount, pSampleData);

    clock_t end = clock();
    double elapsed_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;printf("Elapsed time: %.2f ms\n", elapsed_ms);
    freeconfig(&config);
    // Close files
    drwav_uninit(&wav);
    drwav_free(pSampleData, NULL);
    free(buffer);
    return 0;
}