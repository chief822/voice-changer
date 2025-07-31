/*
Yin algorithmm used to calculate fundamental pitch from audio samples
input parameter: audio samples buffer(raw amplitude values)
output: TODO
*/
#define LAG_MIN 85
#define LAG_MAX 585

float* diffFunc(float *samples, int sampleSize, float* diffData) {
    /*
    diffdata length should be upto LAG_MAX-LAG_MIN

    diffData[lag - 1] corresponds to lag τ = lag
    because lag is fitted into index
    0 index means value for 1 lag and so on

    585 decided for standard as lowest human voice 44100/75
    85 as standard for highest human voice (children not female)
    */
   
    for (int lag = LAG_MIN; lag <= LAG_MAX; ++lag) {
        float sum = 0;
        for (int sample = 0; sample < sampleSize-lag; ++sample) {
            float delta = samples[sample] - samples[sample + lag];
            sum += delta * delta;
        }
        diffData[lag-LAG_MIN] = sum;
    }
}