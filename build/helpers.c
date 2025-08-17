#include <stdint.h>
#include <stddef.h>  // for size_t
#include <math.h>    // for round

#define HELPER_SIZE 32768

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

// void convert_normalized_to_int16(const double* input, int16_t* output, size_t length) {
//     for (size_t i = 0; i < length; ++i) {
//         double x = input[i];

//         // Clamp the input to [-1.0, 1.0] to avoid overflow
//         if (x > 1.0) x = 1.0;
//         if (x < -1.0) x = -1.0;

//         // Scale and convert to int16_t
//         if (x >= 0.0)
//             output[i] = (int16_t)round(x * 32767.0);
//         else
//             output[i] = (int16_t)round(x * 32768.0);  // negative side reaches -32768
//     }
// }

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