### PRESQUITES
You need to have a c compiler installed to build it.

### Compilation:
To compile this code there are some dependencies you need to get before

#### Dependencies:
- To use real time voice changer install, vb audio cable, available at https://vb-audio.com/Cable/ , this is required only if you are using my real time voice_changer.c
- make

Libs used are already in build/
To compile run in root directory: `make worldmain` or `make voice_changer`
worldmain is the cli tool for applying voice effect on audio files and voice changer for real time

##### CLI tool
you can either add worldmain to path or copy it in the folder where the audio file is located. run `./worldmain name.wav output.wav` replace name with audio file name and output with your desired name

##### Real time voice changer
there are 2 effects one robotic and other feminine. you can comment/uncomment the section to choose your effect in world.h before compiling.
run voice_changer and it will select vb audio since its available only on windows it won't and for other os you can pick other virtual microphone to use it. then go to app in which you want to change your voice and go to its settings and change microphone to the virtual microphone you choosed when running voicechanger.

If you want to use the voice_changer effect somewhere else that is available in my file world.h. You can see how to use it in worldmain.c

#### Code Structure
If reviewing my code, voice_changer.c contains code for real time use, worldmain.c for using it on audio files. The actual voice changing effect is in world.h.


