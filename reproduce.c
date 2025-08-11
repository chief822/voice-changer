#include <omp.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "world/cheaptrick.h"
#include "world/d4c.h"
//i have made a static library of world so you can download easily and link it here: 
// for windows: https://drive.google.com/file/d/1mJGzuM3pm3U3EWXiTjud_iEtf6Rnri4J/view?usp=drive_link
// for unix: https://drive.google.com/file/d/1rQ3JVQ6I4f9Vf-nZf2csZnUPC5fg-Veu/view?usp=drive_link

#define WORLD_SAMPLE_RATE 48000
#define WORLD_SAMPLE_SIZE 16384
#define WORLD_FRAME_PERIOD 5
#define WORLD_F0_LENGTH 69      // these are pre calculated and correct
#define WORLD_FFT_SIZE 2048

typedef struct {
    double refined_f0[WORLD_F0_LENGTH];
    double **spectrogram;
    double **aperiodicity;
    double previousSamples[WORLD_SAMPLE_SIZE/2]; // acoording to 50% hop length
} WorldParamters;

void fillDummyInput(double* dummysamples, double* dummyf0TemPos, double* refined_f0) {
    srand((unsigned int)time(NULL));

    // Fill samples with small amplitude noise or a simple synthetic waveform
    for (int i = 0; i < WORLD_SAMPLE_SIZE; ++i) {
        // White noise or sine wave + noise
        double t = (double)i / WORLD_SAMPLE_RATE;
        dummysamples[i] = 0.3 * sin(2.0 * 3.14 * 220.0 * t) + 0.05 * ((double)rand() / RAND_MAX - 0.5);
    }

    // Fill f0 temporal positions (in seconds) and f0 values
    for (int i = 0; i < WORLD_F0_LENGTH; ++i) {
        dummyf0TemPos[i] = i * WORLD_FRAME_PERIOD / 1000.0;  // convert ms to seconds

        // Valid F0 values: > 50 Hz and < 500 Hz
        double f0 = 100.0 + (rand() % 300);  // Random F0 between 100 and 400 Hz
        refined_f0[i] = f0;
    }
}


int setup(WorldParamters* config) {
    config->aperiodicity = malloc(WORLD_F0_LENGTH * sizeof(double *));
    double *aperiodicity_data = malloc(WORLD_F0_LENGTH * (WORLD_FFT_SIZE / 2 + 1) * sizeof(double));
    for (int i = 0; i < WORLD_F0_LENGTH; ++i) {
        config->aperiodicity[i] = aperiodicity_data + i * (WORLD_FFT_SIZE / 2 + 1);
    }
    config->spectrogram = malloc(WORLD_F0_LENGTH * sizeof(double *));
    double *spectrogram_data = malloc(WORLD_F0_LENGTH * (WORLD_FFT_SIZE / 2 + 1) * sizeof(double));
    for (int i = 0; i < WORLD_F0_LENGTH; ++i) {
        config->spectrogram[i] = spectrogram_data + i * (WORLD_FFT_SIZE / 2 + 1);
    }
    for (int i = 0; i < WORLD_SAMPLE_SIZE/2; ++i) {
        config->previousSamples[i] = 0;
    }
    return 0;
}

void reproduceThread (double* dummysamples, double* dummyf0andTemporalPositions, WorldParamters* config) {
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            CheapTrickOption cheapTrickOption;
            InitializeCheapTrickOption(WORLD_SAMPLE_RATE, &cheapTrickOption);
            CheapTrick(dummysamples, WORLD_SAMPLE_SIZE, WORLD_SAMPLE_RATE, dummyf0andTemporalPositions,
                    config->refined_f0, WORLD_F0_LENGTH, &cheapTrickOption, config->spectrogram);
        }

        #pragma omp section
        {
            D4COption option;
            InitializeD4COption(&option);
            D4C(dummysamples, WORLD_SAMPLE_SIZE, WORLD_SAMPLE_RATE, dummyf0andTemporalPositions,
                config->refined_f0, WORLD_F0_LENGTH, WORLD_FFT_SIZE, &option, config->aperiodicity);
        }
    }
}
void reproduceWithoutThread(double* dummysamples, double* dummyf0andTemporalPositions, WorldParamters* config) {
    CheapTrickOption cheapTrickOption;
    InitializeCheapTrickOption(WORLD_SAMPLE_RATE, &cheapTrickOption);
    CheapTrick(dummysamples, WORLD_SAMPLE_SIZE, WORLD_SAMPLE_RATE, dummyf0andTemporalPositions,
        config->refined_f0, WORLD_F0_LENGTH, &cheapTrickOption, config->spectrogram);

    D4COption option;
    InitializeD4COption(&option);
    D4C(dummysamples, WORLD_SAMPLE_SIZE, WORLD_SAMPLE_RATE, dummyf0andTemporalPositions,
        config->refined_f0, WORLD_F0_LENGTH, WORLD_FFT_SIZE, &option, config->aperiodicity);
}

int main() {
    double* dummyf0TemPos = malloc(WORLD_F0_LENGTH * sizeof(double));
double* dummysamples = malloc(WORLD_SAMPLE_SIZE * sizeof(double));
    if (!dummyf0TemPos || !dummysamples) {
        printf("Failed to allocate memory!\n");
        exit(1);
    }
    WorldParamters config;
    setup(&config);
    fillDummyInput(dummysamples, dummyf0TemPos, config.refined_f0);
    clock_t start = clock();
    for (int i = 0; i < 50; i++) {      // 50*16384 is about 17s of audio at 48kHz sample rate
        reproduceThread(dummysamples, dummyf0TemPos, &config);
    }
    clock_t end = clock();
    double elapsed_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("Elapsed time: %.2f ms\n", elapsed_ms);
    free(dummyf0TemPos);
    free(dummysamples);
    return 0;
}