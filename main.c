#define YIN_IMPLEMENTATION
#include "yin.h"
#include <stdio.h>
#include <stdint.h>

#define HEADER_SIZE 44

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./pitch input.wav\n");
        return 1;
    }

    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        printf("Could not open file.\n");
        return 1;
    }

    uint8_t header[HEADER_SIZE];
    fread(&header, sizeof(header), 1, input);

    int16_t buffer[yin_SAMPLE_SIZE];
    float norBuffer[yin_SAMPLE_SIZE];
    float pitchs[2000];
    int i = 0;

    while (fread(buffer, sizeof(buffer), 1, input)) {
        int16_to_float_normalized(buffer, norBuffer, yin_SAMPLE_SIZE);
        removeDC(norBuffer, yin_SAMPLE_SIZE);
        pitchs[i] = yin_getFreq(norBuffer);
        // printf("%f\t", pitchs[i]);
        ++i;
    }

    printf("\npitchs: %d\nMean: %lf\nMedian: %lf\n", i, calculateMean(pitchs, i), calculateMedian(pitchs, i));
    fclose(input);
    return 0;
}
