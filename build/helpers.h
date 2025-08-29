#include <stdint.h>
#include <stddef.h>  // for size_t
#include <math.h>    // for round

#include "filter.h"

#define HELPER_SIZE 32768

// Filters an input array with a Butterworth high-pass filter
// input  - pointer to input samples
// output - pointer to pre-allocated output array
// length - number of samples
// order  - filter order
// fs     - sampling frequency (Hz)
// fc     - cutoff (-3 dB) frequency (Hz)
void butterworth_highpass_filter_array(
    const float *input,
    float *output,
    int length,
    int order,
    float fs,
    float fc
) {
    // Create filter
    BWHighPass* hp = create_bw_high_pass_filter(order, fs, fc);

    // Process array
    for (int i = 0; i < length; i++) {
        output[i] = bw_high_pass(hp, input[i]);
    }

    // Free filter
    free_bw_high_pass(hp);
}

void double_to_float_array(double *double_arr, float *float_arr) {
    for (int i = 0; i < HELPER_SIZE; i++) {
        float_arr[i] = (float)double_arr[i]; // Cast double to float
    }
}
void convertFloatArrayToDouble(const float *floatArray, double *doubleArray) {
    for (int i = 0; i < HELPER_SIZE; i++) {
        doubleArray[i] = (double)floatArray[i]; // Explicitly cast float to double
    }
}
void convertDoubleArrayToFloat(const double *doubleArray, float *floatArray) {
    for (int i = 0; i < HELPER_SIZE; i++) {
        floatArray[i] = (float)doubleArray[i];  // Explicitly cast double to float
    }
}

void removeDC(double* buffer, int size) {
    float mean = 0.0f;
    for (int i = 0; i < size; ++i) {
        mean += buffer[i];
    }
    mean /= size;
    for (int i = 0; i < size; ++i) {
        buffer[i] -= mean;
    }
}