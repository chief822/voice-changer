#define DR_WAV_IMPLEMENTATION // too lazy to do it in separate file that's why
#include "build/dr_wav.h"

#include "world.h"

#include "build/helpers.h"
#include <stdint.h>
#include <stddef.h>
#include <math.h> 
#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#define BLOCK_SIZE 32768
 
int main(int argc, char *argv[])
{
    // Check command-line arguments
    if (argc != 3)
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

    int female = 1; 

    float buffer[BLOCK_SIZE]; 
    double samples[BLOCK_SIZE];
    double processed[BLOCK_SIZE];
    float removed[BLOCK_SIZE];
    int16_t output[BLOCK_SIZE];
    WorldParameters config;
    setup(&config, female);
    clock_t start = clock();
    while (1) {
        drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, BLOCK_SIZE, buffer);
	if (framesRead == 0)
            break;
        convertFloatArrayToDouble(buffer, samples);
        removeDC(samples, BLOCK_SIZE);
        process(samples, processed, &config); 
        double_to_float_array(processed, buffer);
        drwav_f32_to_s16(output, buffer, BLOCK_SIZE);
        drwav_write_pcm_frames(&out, BLOCK_SIZE, output);
    }
    clock_t end = clock();
    double elapsed_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;printf("Elapsed time: %.2f ms\n", elapsed_ms);
    freeconfig(&config);
    // Close files
    drwav_uninit(&wav);
    drwav_uninit(&out);

}
