#include <stdint.h>
#include <stddef.h>  // for size_t
#include <math.h>    // for round

void convertFloatArrayToDouble(const float *floatArray, double *doubleArray, int size) {
    for (int i = 0; i < size; i++) {
        doubleArray[i] = (double)floatArray[i]; // Explicitly cast float to double
    }
}
void convertDoubleArrayToFloat(const double *doubleArray, float *floatArray, int size) {
    for (int i = 0; i < size; i++) {
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