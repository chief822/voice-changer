### USAGE:
To compile this code there are some dependencies you need to get before
#### Dependencies:
My build folder has precompiled libs which you may try to compile using my makefile but their ABI may not follow yours so better to compile them for yourself. Following are are the dependencies required:
- miniaudio available at https://miniaud.io/
- vb audio cable https://vb-audio.com/Cable/ , these are required only if you are compiilng and using my voice_changer.c
- world vocoder available at https://github.com/mmorise/World

I already have them in my build folder and you can compile them there directly and link with voice_changer.c or worldmain.c or with your own app, you can use my makefile if you follow the same structure when compiling.
If you just want the voice_changer effect that is available in my file world.h. You can use this function to do it:

```
process(
double *samples,    // input samples buffer, default size is 32768 to change it you have to change the macros world.h and do some more modification, also dont use less than 16384 sample size otherwise the effect quality will decrease
double* output,     // output buffer
WorldParameters* config);    // config buffer
// to get the config variable do this
WorldParameters config;
// then initialize it to make it work using this
setup(WorldParameters* config, bool female);
// you can pass 1 or true in place of female always as male version is not currently supported well, then finally call the process function
```

#### Voice Changer
To use my voice_changer either compile or use the precompiled one and just execute it, it will first try to automatically find vb audio cable if it failed then it would ask user to input its id and then you can use it
#### To do it on a audio file
Similarly get compiled worldmain you have to execute this on the command line it expects wav file as input and wav file as output, use it like this:
`./worldmain inputAudio.wav OutputAudioName.wav`
you can also test on audio files i provided in tests folder like this:
`./worldmain tests/raven.wav output.wav`
