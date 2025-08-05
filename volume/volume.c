// Modifies the volume of an audio file
#define DR_WAV_IMPLEMENTATION
#include "build/dr_wav.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define BLOCK_SIZE 2048

// Number of bytes in .wav header
const int HEADER_SIZE = 44;

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
    if (!drwav_init_file_write(&out, "output.wav", &format, NULL)) {
        printf("Failed to create output.wav\n");
        return 1;
    }


    float factor = atof(argv[3]);

    // TODO: Read samples from input file and write updated data to output file
    int16_t buffer[BLOCK_SIZE];
    while (1) {
    drwav_uint64 framesRead = drwav_read_pcm_frames_s16(&wav, BLOCK_SIZE, buffer);
    if (framesRead == 0)
        break;

    // Optionally apply processing here, e.g., volume *= 2
    for (int i = 0; i < framesRead; i++) {
        int32_t temp = buffer[i] * 2;  // example volume change
        if (temp > 32767) temp = 32767;
        if (temp < -32768) temp = -32768;
        buffer[i] = temp;
    }

    drwav_write_pcm_frames(&out, framesRead, buffer);
}


    // Close files
    drwav_uninit(&wav);
    drwav_uninit(&out);

}