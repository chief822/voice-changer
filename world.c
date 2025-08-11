#include "world/dio.h"
#include "world/stonemask.h"
#include "world/cheaptrick.h"
#include "world/d4c.h"
#include "world/synthesis.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <omp.h>

#define WORLD_SAMPLE_RATE 48000
#define WORLD_SAMPLE_SIZE 16384
#define WORLD_FRAME_PERIOD 5
#define WORLD_F0_LENGTH 69      // these are pre calculated and correct
#define WORLD_FFT_SIZE 2048

typedef struct {
    int low;
    int high;
    double frac;
} BinMap;

typedef struct {
    CheapTrickOption cheapTrickOption;
    D4COption d4cOption;
    double refined_f0[WORLD_F0_LENGTH];
    double **spectrogram;
    double **aperiodicity;
    double previousSamples[WORLD_SAMPLE_SIZE/2]; // acoording to 50% hop length
    BinMap map[WORLD_FFT_SIZE/2 + 1];
} WorldParamters;

double warping_ratio_for_freq(double f) {
    if (f < 500.0) return 1.08;
    if (f < 2500.0)
        return 1.08 + (f - 500.0) * (1.25 - 1.08) / (2500.0 - 500.0);
    return 1.25 - (f - 2500.0) * (0.20) / (WORLD_SAMPLE_RATE / 2.0 - 2500.0);
}

void build_bin_map(BinMap *map, int bins, int fft_size, int fs) {
    for (int i = 0; i < bins; i++) {
        double target_freq = (double)i * fs / fft_size;
        double src_freq = target_freq / warping_ratio_for_freq(target_freq);
        if (src_freq > fs / 2.0) {
            map[i].low = map[i].high = -1;
            map[i].frac = 0.0;
        } else {
            double src_bin = src_freq * fft_size / fs;
            map[i].low = (int)src_bin;
            map[i].high = map[i].low + 1;
            map[i].frac = src_bin - map[i].low;
        }
    }
}

void shift_formants(double **spectrogram, int f0_length, int bins, BinMap *map) {
    double *temp = malloc(sizeof(double) * bins);

    for (int p = 0; p < f0_length; p++) {
        for (int i = 0; i < bins; i++) {
            if (map[i].low < 0) {
                temp[i] = 0.0;
            } else if (map[i].high < bins) {
                temp[i] = spectrogram[p][map[i].low] * (1.0 - map[i].frac) +
                          spectrogram[p][map[i].high] * map[i].frac;
            } else {
                temp[i] = spectrogram[p][map[i].low];
            }
        }
        memcpy(spectrogram[p], temp, bins * sizeof(double));
    }

    free(temp);
}

void pitchshift(double *samples, const int factor, WorldParamters* config) {
    DioOption dioOption;
    InitializeDioOption(&dioOption);
    dioOption.frame_period = WORLD_FRAME_PERIOD;
    double temporalPositions[WORLD_F0_LENGTH];
    double f0[WORLD_F0_LENGTH];
    double *refined_f0 = config->refined_f0;

    Dio(samples, WORLD_SAMPLE_SIZE, WORLD_SAMPLE_RATE, &dioOption,
            temporalPositions, f0);
    StoneMask(samples, WORLD_SAMPLE_SIZE, WORLD_SAMPLE_RATE, temporalPositions,
              f0, WORLD_F0_LENGTH, refined_f0);

    
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            CheapTrick(samples, WORLD_SAMPLE_SIZE, WORLD_SAMPLE_RATE, temporalPositions,
                    refined_f0, WORLD_F0_LENGTH, &config->cheapTrickOption, config->spectrogram);
        }

        #pragma omp section
        {
            D4C(samples, WORLD_SAMPLE_SIZE, WORLD_SAMPLE_RATE, temporalPositions,
                refined_f0, WORLD_F0_LENGTH, WORLD_FFT_SIZE, &config->d4cOption, config->aperiodicity);
        }
    }  // waits for both sections to finish

    // F0 pitch shift
    for (int i = 0; i < WORLD_F0_LENGTH; ++i) {
        refined_f0[i] *= factor;
    }

    // Formant shift
    shift_formants(config->spectrogram, WORLD_F0_LENGTH, WORLD_FFT_SIZE/2 + 1, config->map);

    Synthesis(config->refined_f0, WORLD_F0_LENGTH, (const double * const *)config->spectrogram, 
    (const double * const *)config->aperiodicity, WORLD_FFT_SIZE, WORLD_FRAME_PERIOD, 
    WORLD_SAMPLE_RATE, WORLD_SAMPLE_SIZE, samples);
}

void process(double *samples, double* output, const int factor, WorldParamters* config1, WorldParamters* config2) {
    double current_samples1[WORLD_SAMPLE_SIZE];
    double current_samples2[WORLD_SAMPLE_SIZE];

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            // Thread 1 work
            memcpy(current_samples1, config1->previousSamples, WORLD_SAMPLE_SIZE/2 * sizeof(double));
            memcpy(current_samples1 + WORLD_SAMPLE_SIZE/2, samples, WORLD_SAMPLE_SIZE/2 * sizeof(double));
            pitchshift(current_samples1, factor, config1);
        }

        #pragma omp section
        {
            // Thread 2 work
            memcpy(current_samples2, samples, WORLD_SAMPLE_SIZE * sizeof(double));
            pitchshift(current_samples2, factor, config2);
        }
    }  // waits for both threads here automatically

    // Combine output after both threads finish
    memcpy(output, current_samples1 + WORLD_SAMPLE_SIZE/4, WORLD_SAMPLE_SIZE/2 * sizeof(double));
    memcpy(output + WORLD_SAMPLE_SIZE/2, current_samples2 + WORLD_SAMPLE_SIZE/4, WORLD_SAMPLE_SIZE/2 * sizeof(double));

    // Save last half for next call
    memcpy(config1->previousSamples, samples + WORLD_SAMPLE_SIZE/2, WORLD_SAMPLE_SIZE/2 * sizeof(double));
    memcpy(config2->previousSamples, samples + WORLD_SAMPLE_SIZE/2, WORLD_SAMPLE_SIZE/2 * sizeof(double));
}

/*
const double kFloorF0 = 71.0;
const double kCeilF0 = 800.0;
const double kLog2 = 0.69314718055994529;
*/

int setup(WorldParamters* config) {
    InitializeCheapTrickOption(WORLD_SAMPLE_RATE, &config->cheapTrickOption);
    InitializeD4COption(&config->d4cOption);
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
    build_bin_map(config->map, WORLD_FFT_SIZE/2 +1, WORLD_FFT_SIZE, WORLD_SAMPLE_RATE);
    return 0;
}

void freeconfig(WorldParamters* config) {
    if (config->spectrogram) {
        free(config->spectrogram[0]);  // Only if WORLD_F0_LENGTH > 0 and setup was successful
        free(config->spectrogram);
    }
    if (config->aperiodicity) {
        free(config->aperiodicity[0]);
        free(config->aperiodicity);
    }
}