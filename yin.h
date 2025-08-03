/*
Yin algorithmm used to calculate fundamental pitch from audio samples
input parameter: audio samples buffer(raw amplitude values)
output: frequency

600decided for standard as lowest human voice 44100/75
55 as standard for highest human voice (children not female)
*/

#ifndef YIN_H
#define YIN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// === Constants ===
#define yin_LAG_MIN 55
#define yin_LAG_MAX 600
#define yin_THRESHOLD 0.7f
#define MAX_DIPS 16
#define yin_SAMPLE_SIZE 1024
#define yin_SAMPLE_RATE 48000

// === Function Declarations ===
void yin_DF(float *samples, float* DFData);
void yin_CMNDF(float* DFData, float* CMNDFData);
int threshold(float* CMNDFData, int* periodCandidates);
float interpolate(float* DFData, int* periodCandidates, int numCandidates);
float yin_getFreq(float* samples);
void removeDC(float* buffer, int size);
double calculateMean(float arr[], int size);
double calculateMedian(float arr[], int n);
void int16_to_float_normalized(const int16_t* input, float* output, size_t length);

#ifdef __cplusplus
}
#endif

#endif // YIN_H


// === IMPLEMENTATION SECTION ===
#ifdef YIN_IMPLEMENTATION

#include <float.h>
#include <stdlib.h>

// --- Internal helper ---
static int findIndexOfMinValue(float *arr, int size) {
    int minIndex = 0;
    float minValue = arr[0];
    for (int i = 1; i < size; ++i) {
        if (arr[i] < minValue) {
            minValue = arr[i];
            minIndex = i;
        }
    }
    return minIndex;
}

void yin_DF(float *samples, float* DFData) {
    /*
    df data length should be upto lagmax
    diffData[lag - 1] corresponds to lag τ = lag
    because lag is fitted into index
    0 index means value for 1 lag and so on
    */

    for (int lag = 1; lag <= yin_LAG_MAX; ++lag) {
        float sum = 0.0f;
        for (int sample = 0; sample < yin_SAMPLE_SIZE-lag; ++sample) {
            float delta = samples[sample] - samples[sample + lag];
            sum += delta * delta;
        }
        DFData[lag-1] = sum;
    }
}

void yin_CMNDF(float* DFData, float* CMNDFData) {
    // cmndf data length should be upto lagmax-lagmin
    // samething for index but except its 0 + lagmin
    float sum = 0.0f;
    for (int i = 0; i < yin_LAG_MIN-1; ++i) {
        sum += DFData[i];
    }
    for (int lag = yin_LAG_MIN; lag < yin_LAG_MAX; ++lag) {
        sum += DFData[lag];
        CMNDFData[lag-yin_LAG_MIN] = lag * DFData[lag] / sum;
    }
}

// todo fix todo
int threshold(float* CMNDFData, int* periodCandidates) {
    int dip = 0;
    for (int sample = 1; sample < yin_LAG_MAX - yin_LAG_MIN - 1; ++sample) {
        if (CMNDFData[sample] < yin_THRESHOLD &&
            CMNDFData[sample] < CMNDFData[sample-1] &&
            CMNDFData[sample] < CMNDFData[sample+1]) {
            periodCandidates[dip] = sample + yin_LAG_MIN;
            ++dip;
            return dip;
        }
        if (dip == MAX_DIPS) {
            return dip;
        }
    }
    if (dip == 0) {
        periodCandidates[dip] = findIndexOfMinValue(CMNDFData, yin_LAG_MAX - yin_LAG_MIN) + yin_LAG_MIN;
        ++dip;
    }
    return dip;
}

float interpolate(float* DFData, int* periodCandidates, int numCandidates) {
    float period = 0;
    float smallest = FLT_MAX;
    for (int i = 0; i < numCandidates; ++i) {
        if (periodCandidates[i] == yin_LAG_MAX-1) {
            if (DFData[periodCandidates[i]-1] < smallest) {
                smallest = DFData[periodCandidates[i]];
                period = periodCandidates[i];
            }
            continue;
        }
        float y1 = DFData[periodCandidates[i]-1 - 1];
        float y2 = DFData[periodCandidates[i]-1];
        float y3 = DFData[periodCandidates[i]-1 + 1];
        float denomLag = 2 * (y1 - 2 * y2 + y3);
        if (denomLag == 0.0f) {
            if (DFData[periodCandidates[i]] < smallest) {
                smallest = DFData[periodCandidates[i]];
                period = periodCandidates[i];
            }
            continue;
        }
        float testPeriod = (float)periodCandidates[i] + ((y3 - y1) / denomLag);
        float denomDF = 8 * (2 * DFData[periodCandidates[i]-1] - DFData[periodCandidates[i]-1 - 1] - DFData[periodCandidates[i]-1 + 1]);
        if (denomDF == 0.0f) {
            if (DFData[periodCandidates[i]] < smallest) {
                smallest = DFData[periodCandidates[i]];
                period = periodCandidates[i];
            }
            continue;
        }
        float testDFValue = y2 - (y3 - y1) * (y3 - y1) / denomDF;
        if (testDFValue < smallest) {
            smallest = testDFValue;
            period = testPeriod;
        }
    }
    return period;
}
/*
the audio buffer needs to have dc offset removed before giving it
normalizing is not required and it shouldnt be done also for real time
format type should be float -1 to 1
*/
float yin_getFreq(float* samples) {
    float DFData[yin_LAG_MAX];
    float CMNDFData[yin_LAG_MAX - yin_LAG_MIN];
    int periodCandidates[MAX_DIPS];
    yin_DF(samples, DFData);
    yin_CMNDF(DFData, CMNDFData);
    int numCandidates = threshold(CMNDFData, periodCandidates);
    float truePeriod = interpolate(DFData, periodCandidates, numCandidates);
    return yin_SAMPLE_RATE / truePeriod;
}

void removeDC(float* buffer, int size) {
    float mean = 0.0f;
    for (int i = 0; i < size; ++i) {
        mean += buffer[i];
    }
    mean /= size;
    for (int i = 0; i < size; ++i) {
        buffer[i] -= mean;
    }
}

// Function to calculate the mean of an array
double calculateMean(float arr[], int size) {
    if (size == 0) return 0.0;  // or return NAN
    float sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    // Type casting sum to double to ensure floating-point division
    return (double)sum / size; 
}


// Comparison function for qsort (for integers)
int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// Function to calculate the median of an integer array
double calculateMedian(float arr[], int n) {
    // Sort the array
    qsort(arr, n, sizeof(float), compare);

    // Check if the number of elements is odd or even
    if (n % 2 == 1) {
        // Odd number of elements: median is the middle element
        return (double)arr[n / 2];
    } else {
        // Even number of elements: median is the average of the two middle elements
        return (double)(arr[n / 2 - 1] + arr[n / 2]) / 2.0;
    }
}

void int16_to_float_normalized(const int16_t* input, float* output, size_t length) {
    const float scale = 1.0f / 32768.0f;
    for (size_t i = 0; i < length; ++i) {
        output[i] = input[i] * scale;
    }
}

#endif // YIN_IMPLEMENTATION
