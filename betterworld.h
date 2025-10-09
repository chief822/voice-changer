#include "world/dio.h"
#include "world/harvest.h"
#include "world/stonemask.h"
#include "world/cheaptrick.h"
#include "world/d4c.h"
#include "world/synthesis.h"
#include <string.h>	// for memcpy
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define FACTOR 2
// these are pre calculated and correct, if you are editing frame period or sample size then find f0 length using this:
/* use sample size not input size for this
 * int GetSamplesForDIO(int fs, int x_length, double frame_period) {
 *    return static_cast<int>(1000.0 * x_length / fs / frame_period) + 1;
 * }
 */
#define WORLD_FFT_SIZE 2048 // better if you dont edit anything that changes fft size as it is not easy to make it work without error

typedef struct {
    int low;
    int high;
    double frac;
} BinMap;

typedef struct {
    double *samples;
    double *previousSamples;
    DioOption dioOption;
    CheapTrickOption cheapTrickOption;
    D4COption d4cOption;
    double *temporalPositions;
    double *f0;
    double *refined_f0;
    double **spectrogram;
    double **aperiodicity;
    int sampleRate;
    int sampleCount;
    int f0_length;
    int framePeriod;
    BinMap map[WORLD_FFT_SIZE/2 + 1];
} WorldParameters;

void freeconfig(WorldParameters* config);

// Function to compute geometric mean using log trick
double geometric_mean(const double arr[], size_t n) {
    if (n == 0) {
        return NAN; // undefined for empty arrays
    }

    double log_sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        if (arr[i] <= 0.0) {
            return NAN; // geometric mean only defined for positive numbers
        }
        log_sum += log(arr[i]);
    }

    return exp(log_sum / n);
}

double warping_ratio_for_freq_m2f(double f, int sampleRate) {
    if (f < 500.0) return 1.08;
    if (f < 2500.0)
        return 1.08 + (f - 500.0) * (1.25 - 1.08) / (2500.0 - 500.0);
    return 1.25 - (f - 2500.0) * (0.20) / (sampleRate / 2.0 - 2500.0);
}

void build_bin_map_m2f(BinMap *map, int sampleRate) {
    for (int i = 0; i < WORLD_FFT_SIZE/2 + 1; i++) {
        double target_freq = (double)i * sampleRate / WORLD_FFT_SIZE;
        double src_freq = target_freq / warping_ratio_for_freq_m2f(target_freq, sampleRate);
        if (src_freq > sampleRate / 2.0) {
            map[i].low = map[i].high = -1;
            map[i].frac = 0.0;
        } else {
            double src_bin = src_freq * WORLD_FFT_SIZE / sampleRate;
            map[i].low = (int)src_bin;
            map[i].high = map[i].low + 1;
            map[i].frac = src_bin - map[i].low;
        }
    }
}

void shift_formants(double **spectrogram, int f0_length, BinMap *map) {
    double *temp = malloc(sizeof(double) * (WORLD_FFT_SIZE/2 + 1));

    for (int p = 0; p < f0_length; p++) {
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
    int sampleCount = config->sampleCount;
    int sampleRate = config->sampleRate;
    int f0_length = config->f0_length;

    Dio(samples, sampleCount, sampleRate, &config->dioOption,
        config->temporalPositions, config->f0);
    StoneMask(samples, sampleCount, sampleRate, config->temporalPositions,
              config->f0, f0_length, config->refined_f0);

    CheapTrick(samples, sampleCount, sampleRate, config->temporalPositions,
        config->refined_f0, f0_length, &config->cheapTrickOption, config->spectrogram);
    D4C(samples, sampleCount, sampleRate, config->temporalPositions,
        config->refined_f0, f0_length, WORLD_FFT_SIZE, &config->d4cOption, config->aperiodicity);
    // int pitch = 170.0;
    for (int i = 0; i < f0_length; ++i) {
        if (config->refined_f0[i] > 50.0) {
            config->refined_f0[i] *= 2;
        }
    }
    shift_formants(config->spectrogram, f0_length, config->map);
    Synthesis(config->refined_f0, f0_length, (const double * const *)config->spectrogram,
              (const double * const *)config->aperiodicity, WORLD_FFT_SIZE, config->framePeriod,
              sampleRate, sampleCount, samples);
}

void process(double *samples, double* output, WorldParameters* config) {
    int sampleCountPerCall = (int)(config->sampleCount / 1.5);
    double* currentSamples = config->samples;
    double* previousSamples = config->previousSamples;
    memcpy(currentSamples, previousSamples, sampleCountPerCall/2  * sizeof(double));
    memcpy(previousSamples, samples + sampleCountPerCall/2, sampleCountPerCall/2  * sizeof(double));
    memcpy(currentSamples + sampleCountPerCall/2, samples, sampleCountPerCall * sizeof(double));
    pitchshift(currentSamples, config);
    memcpy(output, currentSamples + sampleCountPerCall/4, sampleCountPerCall * sizeof(double));
}

int setup(WorldParameters* config, int sampleCountPerCall, int sampleRate, int framePeriod, bool female) {
    int sampleCount = (int)(sampleCountPerCall * 1.5);     // actual samples count used in processing
    config->sampleCount = sampleCount;
    config->sampleRate = sampleRate;
    config->framePeriod = framePeriod;

    config->samples = malloc(sampleCount * sizeof(double));
    config->previousSamples = malloc(sampleCountPerCall/2 * sizeof(double));
    if(!config->samples || !config->previousSamples) {
        return -1;
    }

    InitializeDioOption(&config->dioOption);
    config->dioOption.frame_period = framePeriod;
    InitializeCheapTrickOption(sampleRate, &config->cheapTrickOption);
    InitializeD4COption(&config->d4cOption);

    config->f0_length = GetSamplesForDIO(sampleRate, sampleCount, framePeriod);
    int f0_length = config->f0_length;

    // Allocate aperiodicity
    config->aperiodicity = malloc(f0_length * sizeof(double *));
    double *aperiodicity_data = malloc(f0_length * (WORLD_FFT_SIZE / 2 + 1) * sizeof(double));
    if (config->aperiodicity == NULL || aperiodicity_data == NULL) {
        free(config->aperiodicity);
        free(aperiodicity_data);
        return -1;
    }
    // for (int i = 0; i < f0_length * (WORLD_FFT_SIZE / 2 + 1); ++i) {
    //     aperiodicity_data[i] = 0.0;
    // }
    for (int i = 0; i < f0_length; ++i)
        config->aperiodicity[i] = aperiodicity_data + i * (WORLD_FFT_SIZE / 2 + 1);

    // Allocate spectrogram
    config->spectrogram = malloc(f0_length * sizeof(double *));
    double *spectrogram_data = malloc(f0_length * (WORLD_FFT_SIZE / 2 + 1) * sizeof(double));
    if (config->spectrogram == NULL || spectrogram_data == NULL) {
        free(aperiodicity_data);
        free(config->aperiodicity);
        free(config->spectrogram);
        free(spectrogram_data);
        return -1;
    }
    for (int i = 0; i < f0_length; ++i)
        config->spectrogram[i] = spectrogram_data + i * (WORLD_FFT_SIZE / 2 + 1);

    build_bin_map_m2f(config->map, sampleRate);

    config->temporalPositions = malloc(f0_length * sizeof(double));
    config->f0 = malloc(f0_length * sizeof(double));
    config->refined_f0 = malloc(f0_length * sizeof(double));
    if (config->f0 == NULL || config->refined_f0 == NULL || config->temporalPositions == NULL) {
        freeconfig(config);
        return -1;
    }

    return 0;
}

void freeconfig(WorldParameters* config) {
    if (!config) return;
    free(config->f0);
    free(config->refined_f0);
    free(config->temporalPositions);
    if (config->spectrogram) {
        free(config->spectrogram[0]);
        free(config->spectrogram);
    }
    if (config->aperiodicity) {
        free(config->aperiodicity[0]);
        free(config->aperiodicity);
    }
    free(config->samples);
    free(config->previousSamples);
}
