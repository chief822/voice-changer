#include <stdint.h> /* For standard interger types (int16_t) */
#include <stdlib.h> /* For call to malloc */
#include "yin.h"

/* ------------------------------------------------------------------------------------------
--------------------------------------------------------------------------- PRIVATE FUNCTIONS
-------------------------------------------------------------------------------------------*/

/**
 * Step 1: Calculates the squared difference of the signal with a shifted version of itself.
 * @param buffer Buffer of samples to process. 
 *
 * This is the Yin algorithms tweak on autocorellation. Read http://audition.ens.fr/adc/pdf/2002_JASA_YIN.pdf
 * for more details on what is in here and why it's done this way.
 */
void Yin_difference(Yin *yin, int16_t* buffer){
	int16_t i;
	int16_t tau;
	float delta;

	/* Calculate the difference for difference shift values (tau) for the half of the samples */
	for(tau = 0 ; tau < yin->halfBufferSize; tau++){

		/* Take the difference of the signal with a shifted version of itself, then square it.
		 * (This is the Yin algorithm's tweak on autocorellation) */ 
		for(i = 0; i < yin->halfBufferSize; i++){
			delta = buffer[i] - buffer[i + tau];
			yin->yinBuffer[tau] += delta * delta;
		}
	}
}


/**
 * Step 2: Calculate the cumulative mean on the normalised difference calculated in step 1
 * @param yin #Yin structure with information about the signal
 *
 * This goes through the Yin autocorellation values and finds out roughly where shift is which 
 * produced the smallest difference
 */
void Yin_cumulativeMeanNormalizedDifference(Yin *yin){
	int16_t tau;
	float runningSum = 0;
	yin->yinBuffer[0] = 1;

	/* Sum all the values in the autocorellation buffer and nomalise the result, replacing
	 * the value in the autocorellation buffer with a cumulative mean of the normalised difference */
	for (tau = 1; tau < yin->halfBufferSize; tau++) {
		runningSum += yin->yinBuffer[tau];
		yin->yinBuffer[tau] *= tau / runningSum;
	}
}

/**
 * Step 3: Search through the normalised cumulative mean array and find values that are over the threshold
 * @return Shift (tau) which caused the best approximate autocorellation. -1 if no suitable value is found over the threshold.
 */
int16_t Yin_absoluteThreshold(Yin *yin){
	int16_t tau;

	/* Search through the array of cumulative mean values, and look for ones that are over the threshold 
	 * The first two positions in yinBuffer are always so start at the third (index 2) */
	for (tau = 50; tau < yin->halfBufferSize ; tau++) {
		if (yin->yinBuffer[tau] < yin->threshold) {
			while (tau + 1 < yin->halfBufferSize && yin->yinBuffer[tau + 1] < yin->yinBuffer[tau]) {
				tau++;
			}
			/* found tau, exit loop and return
			 * store the probability
			 * From the YIN paper: The yin->threshold determines the list of
			 * candidates admitted to the set, and can be interpreted as the
			 * proportion of aperiodic power tolerated
			 * within a periodic signal.
			 *
			 * Since we want the periodicity and and not aperiodicity:
			 * periodicity = 1 - aperiodicity */
			yin->probability = 1 - yin->yinBuffer[tau];
			break;
		}
	}
	if (tau == yin->halfBufferSize || yin->yinBuffer[tau] >= yin->threshold) {
		tau = -1;
		return tau;
	}
	return tau;
}

/**
 * Step 5: Interpolate the shift value (tau) to improve the pitch estimate.
 * @param  yin         [description]
 * @param  tauEstimate [description]
 * @return             [description]
 *
 * The 'best' shift value for autocorellation is most likely not an interger shift of the signal.
 * As we only autocorellated using integer shifts we should check that there isn't a better fractional 
 * shift value.
 */
float Yin_parabolicInterpolation(Yin *yin, int16_t tauEstimate) {
	float betterTau;
	int16_t x0;
	int16_t x2;
	
	/* Calculate the first polynomial coeffcient based on the current estimate of tau */
	if (tauEstimate < 1) {
		x0 = tauEstimate;
	} 
	else {
		x0 = tauEstimate - 1;
	}

	/* Calculate the second polynomial coeffcient based on the current estimate of tau */
	if (tauEstimate + 1 < yin->halfBufferSize) {
		x2 = tauEstimate + 1;
	} 
	else {
		x2 = tauEstimate;
	}

	/* Algorithm to parabolically interpolate the shift value tau to find a better estimate */
	if (x0 == tauEstimate) {
		if (yin->yinBuffer[tauEstimate] <= yin->yinBuffer[x2]) {
			betterTau = tauEstimate;
		} 
		else {
			betterTau = x2;
		}
	} 
	else if (x2 == tauEstimate) {
		if (yin->yinBuffer[tauEstimate] <= yin->yinBuffer[x0]) {
			betterTau = tauEstimate;
		} 
		else {
			betterTau = x0;
		}
	} 
	else {
		float s0, s1, s2;
		s0 = yin->yinBuffer[x0];
		s1 = yin->yinBuffer[tauEstimate];
		s2 = yin->yinBuffer[x2];
		// fixed AUBIO implementation, thanks to Karl Helgason:
		// (2.0f * s1 - s2 - s0) was incorrectly multiplied with -1
		betterTau = tauEstimate + (s2 - s0) / (2 * (2 * s1 - s2 - s0));
	}


	return betterTau;
}





/* ------------------------------------------------------------------------------------------
---------------------------------------------------------------------------- PUBLIC FUNCTIONS
-------------------------------------------------------------------------------------------*/



/**
 * Initialise the Yin pitch detection object
 * @param yin        Yin pitch detection object to initialise
 * @param bufferSize Length of the audio buffer to analyse
 * @param threshold  Allowed uncertainty (e.g 0.05 will return a pitch with ~95% probability)
 */
void Yin_init(Yin *yin, int16_t bufferSize, float threshold){
	/* Initialise the fields of the Yin structure passed in */
	yin->bufferSize = bufferSize;
	yin->halfBufferSize = bufferSize / 2;
	yin->probability = 0.0;
	yin->threshold = threshold;

	/* Allocate the autocorellation buffer and initialise it to zero */
	yin->yinBuffer = (float *) malloc(sizeof(float)* yin->halfBufferSize);

	int16_t i;
	for(i = 0; i < yin->halfBufferSize; i++){
		yin->yinBuffer[i] = 0;
	}
}

/**
 * Runs the Yin pitch detection algortihm
 * @param  yin    Initialised Yin object
 * @param  buffer Buffer of samples to analyse
 * @return        Fundamental frequency of the signal in Hz. Returns -1 if pitch can't be found
 */
float Yin_getPeriod(Yin *yin, int16_t* buffer){
	int16_t tauEstimate = -1;
	float pitchInHertz = -1;
	
	/* Step 1: Calculates the squared difference of the signal with a shifted version of itself. */
	Yin_difference(yin, buffer);
	
	/* Step 2: Calculate the cumulative mean on the normalised difference calculated in step 1 */
	Yin_cumulativeMeanNormalizedDifference(yin);
	
	/* Step 3: Search through the normalised cumulative mean array and find values that are over the threshold */
	tauEstimate = Yin_absoluteThreshold(yin);
	
	/* Step 5: Interpolate the shift value (tau) to improve the pitch estimate. */
	if(tauEstimate != -1){
		pitchInHertz = YIN_SAMPLING_RATE / Yin_parabolicInterpolation(yin, tauEstimate);
	}
	
	return pitchInHertz;
}

/**
 * Certainty of the pitch found 
 * @param  yin Yin object that has been run over a buffer
 * @return     Returns the certainty of the note found as a decimal (i.e 0.3 is 30%)
 */
float Yin_getProbability(Yin *yin){
	return yin->probability;
}











void removeDC(int16_t* buffer, int size) {
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

// void int16_to_float_normalized(const int16_t* input, float* output, size_t length) {
//     const float scale = 1.0f / 32768.0f;
//     for (size_t i = 0; i < length; ++i) {
//         output[i] = input[i] * scale;
//     }
// }