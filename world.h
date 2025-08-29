#include "world/dio.h"
#include "world/harvest.h"
#include "world/stonemask.h"
#include "world/cheaptrick.h"
#include "world/d4c.h"
#include "world/synthesis.h"
#include <string.h>	// for memcpy
#include <math.h>   // for pow
#include <stdlib.h>	// dont know why for these
#include <stdbool.h>
#include <omp.h>	// for parallelism in pitchshift

#define FACTOR 2
#define WORLD_SAMPLE_RATE 48000
#define WORLD_SAMPLE_SIZE 49152
#define WORLD_FRAME_PERIOD 5
#define WORLD_F0_LENGTH 205    // these are pre calculated and correct
#define WORLD_FFT_SIZE 2048
#define WORLD_INPUT_SIZE 32768

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
    double previousSamples[WORLD_INPUT_SIZE/2]; // acoording to 50% hop length
    BinMap map[WORLD_FFT_SIZE/2 + 1];
} WorldParameters;

double warping_ratio_for_freq_m2f(double f) {
    if (f < 500.0) return 1.08;
    if (f < 2500.0)
        return 1.08 + (f - 500.0) * (1.25 - 1.08) / (2500.0 - 500.0);
    return 1.25 - (f - 2500.0) * (0.20) / (WORLD_SAMPLE_RATE / 2.0 - 2500.0);
}

double warping_ratio_for_freq_f2m(double f) {
    // First formant: ~0–800 Hz
    if (f < 800.0) return 0.65; // 35% lower

    // Second formant: ~800–2500 Hz
    if (f < 2500.0)
        return 0.65 - (f - 800.0) * (0.10) / (2500.0 - 800.0); // 0.65 → 0.55

    // Highs: restore some ratio for consonant clarity
    return 0.55 + (f - 2500.0) * 0.25 / (WORLD_SAMPLE_RATE / 2.0 - 2500.0);
}


void build_bin_map_m2f(BinMap *map) {
    for (int i = 0; i < WORLD_FFT_SIZE/2 + 1; i++) {
        double target_freq = (double)i * WORLD_SAMPLE_RATE / WORLD_FFT_SIZE;
        double src_freq = target_freq / warping_ratio_for_freq_m2f(target_freq);
        if (src_freq > WORLD_SAMPLE_RATE / 2.0) {
            map[i].low = map[i].high = -1;
            map[i].frac = 0.0;
        } else {
            double src_bin = src_freq * WORLD_FFT_SIZE / WORLD_SAMPLE_RATE;
            map[i].low = (int)src_bin;
            map[i].high = map[i].low + 1;
            map[i].frac = src_bin - map[i].low;
        }
    }
}

void build_bin_map_f2m(BinMap *map) {
    for (int i = 0; i < WORLD_FFT_SIZE/2 + 1; i++) {
        double target_freq = (double)i * WORLD_SAMPLE_RATE / WORLD_FFT_SIZE;

        // Multiply to push bins lower in frequency
        double src_freq = target_freq * warping_ratio_for_freq_f2m(target_freq);

        if (src_freq > WORLD_SAMPLE_RATE / 2.0 || src_freq < 0.0) {
            map[i].low = map[i].high = -1;
            map[i].frac = 0.0;
        } else {
            double src_bin = src_freq * WORLD_FFT_SIZE / WORLD_SAMPLE_RATE;
            map[i].low = (int)src_bin;
            map[i].high = map[i].low + 1;
            map[i].frac = src_bin - map[i].low;
        }
    }
}

void shift_formants(double **spectrogram, BinMap *map) {
    double *temp = malloc(sizeof(double) * (WORLD_FFT_SIZE/2 + 1));

    for (int p = 0; p < WORLD_F0_LENGTH; p++) {
        for (int i = 0; i < WORLD_FFT_SIZE/2 + 1; i++) {
            if (map[i].low < 0) {
                temp[i] = 0.0;
            } else if (map[i].high < WORLD_FFT_SIZE/2 + 1) {
                temp[i] = spectrogram[p][map[i].low] * (1.0 - map[i].frac) +
                          spectrogram[p][map[i].high] * map[i].frac;
            } else {
                temp[i] = spectrogram[p][map[i].low];
            }
        }
        memcpy(spectrogram[p], temp, (WORLD_FFT_SIZE/2 + 1) * sizeof(double));
    }

    free(temp);
}

void pitchshift(double *samples, WorldParameters* config) {
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
    }
    // F0 pitch shift
    for (int i = 0; i < WORLD_F0_LENGTH; ++i) {
        refined_f0[i] *= FACTOR;
    }

    // Formant shift
    shift_formants(config->spectrogram, config->map);
    Synthesis(config->refined_f0, WORLD_F0_LENGTH, (const double * const *)config->spectrogram, 
    (const double * const *)config->aperiodicity, WORLD_FFT_SIZE, WORLD_FRAME_PERIOD, 
    WORLD_SAMPLE_RATE, WORLD_SAMPLE_SIZE, samples);
}

void process(double *samples, double* output, WorldParameters* config) {
    double current_samples[WORLD_SAMPLE_SIZE];
    memcpy(current_samples, config->previousSamples, WORLD_INPUT_SIZE/2  * sizeof(double));
    memcpy(config->previousSamples, samples + WORLD_INPUT_SIZE/2, WORLD_INPUT_SIZE/2  * sizeof(double));
    memcpy(current_samples + WORLD_INPUT_SIZE/2, samples, WORLD_INPUT_SIZE * sizeof(double));
    pitchshift(current_samples, config);
    memcpy(output, current_samples + WORLD_INPUT_SIZE/4, WORLD_INPUT_SIZE * sizeof(double));
}

/*
const double kFloorF0 = 71.0;
const double kCeilF0 = 800.0;
const double kLog2 = 0.69314718055994529;
*/

int setup(WorldParameters* config, bool female) {
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
    for (int i = 0; i < WORLD_INPUT_SIZE/2; ++i) {
        config->previousSamples[i] = 0;
    }
    if (female) {
        build_bin_map_m2f(config->map);
    }
    else {
        build_bin_map_f2m(config->map);
    }
    return 0;
}

void freeconfig(WorldParameters* config) {
    if (config->spectrogram) {
        free(config->spectrogram[0]);  // Only if WORLD_F0_LENGTH > 0 and setup was successful
        free(config->spectrogram);
    }
    if (config->aperiodicity) {
        free(config->aperiodicity[0]);
        free(config->aperiodicity);
    }
}