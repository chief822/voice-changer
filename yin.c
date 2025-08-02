/*
Yin algorithmm used to calculate fundamental pitch from audio samples
input parameter: audio samples buffer(raw amplitude values)
output: frequency

600decided for standard as lowest human voice 44100/75
55 as standard for highest human voice (children not female)
*/

#include<float.h>

#define LAG_MIN 55
#define LAG_MAX 600
#define THRESHOLD 1.8f
#define MAX_DIPS 16
#define SAMPLE_SIZE 1024
#define SAMPLE_RATE 44100

void yin_DF(float *samples, float* DFData) {
    /*
    df data length should be upto lagmax
    diffData[lag - 1] corresponds to lag τ = lag
    because lag is fitted into index
    0 index means value for 1 lag and so on
    */

    for (int lag = 1; lag <= LAG_MAX; ++lag) {
        float sum = 0.0f;
        for (int sample = 0; sample < SAMPLE_SIZE-lag; ++sample) {
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
    for (int i = 0; i < LAG_MIN-1; ++i) {
        sum += DFData[i];
    }
    for (int lag = LAG_MIN; lag < LAG_MAX; ++lag) {
        sum += DFData[lag];
        CMNDFData[lag-LAG_MIN] = lag * DFData[lag] / sum;
    }
}

int findIndexOfMinValue(float *arr, int size);
int threshold(float* CMNDFData, int* periodCandidates) {
    int dip = 0;
    for (int sample = 1; sample < LAG_MAX-LAG_MIN-1; ++sample) {
        if (CMNDFData[sample] < THRESHOLD && (CMNDFData[sample] < CMNDFData[sample-1] &&
        (CMNDFData[sample] < CMNDFData[sample+1]))) {
            periodCandidates[dip] = sample + LAG_MIN;
            ++dip;
        }
        if (dip == MAX_DIPS) {
            return dip;
        }
    }
    if (dip == 0) {
        periodCandidates[dip] = findIndexOfMinValue(CMNDFData, LAG_MAX - LAG_MIN) + LAG_MIN;
        ++dip;
    }
    return dip;
}


float interpolate(float* DFData, int* periodCandidates, int numCandidates) {
    float period = 0;
    float smallest = FLT_MAX;
    for (int i = 0; i < numCandidates; ++i) {
        if (periodCandidates[i] == LAG_MAX-1) {
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

// function copied from chatgpt, trusting on it that it works
int findIndexOfMinValue(float *arr, int size) {
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
/*
the audio buffer needs to have dc offset removed before giving it
normalizing is not required and it shouldnt be done also for real time
format type should be float -1 to 1
*/
float yin_getFreq(float* samples) {
    float DFData[LAG_MAX];
    float CMNDFData[LAG_MAX-LAG_MIN];
    int periodCandidates[MAX_DIPS];
    yin_DF(samples, DFData);
    yin_CMNDF(DFData, CMNDFData);       // performance can be increased by combining threshold
    int numCandidates = threshold(CMNDFData, periodCandidates);  // into CMNDF function through early exit. TODO
    float truePeriod = interpolate(DFData, periodCandidates, numCandidates);

    // frequency:-
    return SAMPLE_RATE / truePeriod;
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
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    // Type casting sum to double to ensure floating-point division
    return (double)sum / size; 
}

#include <stdint.h>
#include <stddef.h>

void int16_to_float_normalized(const int16_t* input, float* output, size_t length) {
    const float scale = 1.0f / 32768.0f;  // scale for converting int16 to float in [-1.0, 1.0)

    for (size_t i = 0; i < length; ++i) {
        output[i] = input[i] * scale;
    }
}

#include <stdio.h>
#include <stdlib.h>

// Comparison function for qsort (for integers)
int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// Function to calculate the median of an integer array
double calculateMedian(float arr[], int n) {
    // Sort the array
    qsort(arr, n, sizeof(int), compare);

    // Check if the number of elements is odd or even
    if (n % 2 == 1) {
        // Odd number of elements: median is the middle element
        return (double)arr[n / 2];
    } else {
        // Even number of elements: median is the average of the two middle elements
        return (double)(arr[n / 2 - 1] + arr[n / 2]) / 2.0;
    }
}

// Number of bytes in .wav header
const int HEADER_SIZE = 44;

int main(int argc, char *argv[])
{
    // Check command-line arguments
    if (argc != 2)
    {
        printf("Usage: ./volume input.wav\n");
        return 1;
    }

    // Open files and determine scaling factor
    FILE *input = fopen(argv[1], "rb");
    if (input == NULL)
    {
        printf("Could not open file.\n");
        return 1;
    }

    // TODO: Copy THE FIRST 44 BYTES FROM from header input file to header output file
    uint8_t header[HEADER_SIZE];

    fread(&header, sizeof(header), 1, input);

    // TODO: Read samples from input file and write updated data to output file

    int16_t buffer[SAMPLE_SIZE];
    float norBuffer[SAMPLE_SIZE];
    float pitchs[2000];
    int i = 0;
    while (fread(buffer, sizeof(buffer), 1, input))
    {
        int16_to_float_normalized(buffer, norBuffer, SAMPLE_SIZE);
        removeDC(norBuffer, SAMPLE_SIZE);
        pitchs[i] = yin_getFreq(norBuffer);
        ++i;
    }
    printf(" Mean: %lf\n Median: %lf", calculateMean(pitchs, i), calculateMedian(pitchs, i));
    // Close files
    fclose(input);
}