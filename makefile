# Use the command-line argument as the target name
TARGET := $(MAKECMDGOALS)

# Default rule: build the specified target
.PHONY: all
all: $(TARGET)

%: %.c
	gcc -I. -Ibuild -fopenmp -O3 -ftree-vectorize -fopt-info-vec-optimized -funroll-loops -g -c $@.c
	g++ $@.o build/miniaudio.o build/libmoreopworld.lib -fopenmp -O3 -g -o $@

# static libray build command
# g++ -c -I. -O3 -ftree-vectorize -fopt-info-vec-optimized -funroll-loops -g *.cpp
# ar rcs libmoreopworld.lib *.o