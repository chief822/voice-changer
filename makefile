# Use the command-line argument as the target name
TARGET := $(MAKECMDGOALS)

# Default rule: build the specified target
.PHONY: all
all: $(TARGET)

%: %.c
	gcc -I. -Ibuild -fopenmp -O3 -ftree-vectorize -fopt-info-vec-optimized -march=native -g -c $@.c
	g++ $@.o build/libopworld.lib -fopenmp -O3 -g -o $@

# static libray build command
# g++ -c -I. -O3 -ftree-vectorize -fopt-info-vec-optimized -march=native -g *.cpp
# ar rcs libname.lib *.o