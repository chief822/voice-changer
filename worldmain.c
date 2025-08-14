// Modifies the volume of an audio file
#define DR_WAV_IMPLEMENTATION
#include "build/dr_wav.h"

#include "world.c"

#include "build/helpers.c"
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define BLOCK_SIZE 16384

int main(int argc, char *argv[])
{
    // Check command-line arguments
    if (argc != 4)
    {
        printf("Usage: ./volume input.wav output.wav factor\n");
        return 1;
    }

    // Open files and determine scaling factor
    drwav wav;
    if (!drwav_init_file(&wav, argv[1], NULL)) {
        printf("Failed to open WAV file.\n");
        return 1;
    }

    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = wav.channels;
    format.sampleRate = wav.sampleRate;
    format.bitsPerSample = wav.bitsPerSample;

    drwav out;
    if (!drwav_init_file_write(&out, argv[2], &format, NULL)) {
        printf("Failed to create output.wav\n");
        return 1;
    } 


    float factor = atof(argv[3]);
    int female = 1; 

    float buffer[BLOCK_SIZE]; 
    double samples[BLOCK_SIZE];
    double processed[BLOCK_SIZE];
    int16_t output[BLOCK_SIZE];
    WorldParamters config1;
    setup(&config1, female); 
    WorldParamters config2;
    setup(&config2, female); 
    clock_t start = clock();
    while (1) {
        drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, BLOCK_SIZE, buffer);
        if (framesRead == 0)
            break; 
        convertFloatArrayToDouble(buffer, samples, BLOCK_SIZE);
        // removeDC(samples, BLOCK_SIZE);
        process(samples, processed, factor, &config1, &config2);
        drwav_f64_to_s16(output, processed, BLOCK_SIZE);
        drwav_write_pcm_frames(&out, framesRead, output);
    }
    clock_t end = clock();
    double elapsed_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;printf("Elapsed time: %.2f ms\n", elapsed_ms);
    freeconfig(&config1);
    freeconfig(&config2);
    // Close files
    drwav_uninit(&wav);
    drwav_uninit(&out);

}
