# Detect OS
ifeq ($(OS),Windows_NT)
    LIB_EXT = lib
else
    LIB_EXT = a
endif

SRC_DIR = World-master/src
BUILD_DIR = build

CXX = g++
CC = gcc
CXXFLAGS = -c -IWorld-master/src/ -O3 -ftree-vectorize -fopt-info-vec-optimized -funroll-loops -g
CFLAGS = -c -I. -O3 -ftree-vectorize -fopt-info-vec-optimized -funroll-loops -g
LDFLAGS = -fopenmp -O3 -g

LIB_NAME = libworld.$(LIB_EXT)
LIB_PATH = $(BUILD_DIR)/$(LIB_NAME)
MINIAUDIO_OBJ = $(BUILD_DIR)/miniaudio.o
DRWAV_OBJ = $(BUILD_DIR)/dr_wav.o

.PHONY: all build clean

# Default rule
all: build

# Build libworld and helper objects
build: $(LIB_PATH) $(MINIAUDIO_OBJ) $(DRWAV_OBJ)

# Create the library from all .cpp files in SRC_DIR
$(LIB_PATH): $(wildcard $(SRC_DIR)/*.cpp)
	@mkdir -p $(BUILD_DIR)
	@for file in $^; do \
		$(CXX) $(CXXFLAGS) $$file -o $(BUILD_DIR)/$$(basename $$file .cpp).o; \
	done
	ar rcs $@ $(BUILD_DIR)/*.o
	rm -f $(BUILD_DIR)/*.o

# Compile miniaudio.c into object file
$(MINIAUDIO_OBJ): $(BUILD_DIR)/miniaudio.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Compile dr_wav.c into object file
$(DRWAV_OBJ): $(BUILD_DIR)/dr_wav.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# Rule to compile any executable from a .c file
%: %.c $(LIB_PATH) $(MINIAUDIO_OBJ) $(DRWAV_OBJ)
	$(CC) -I. -I$(BUILD_DIR) -fopenmp -O3 -ftree-vectorize -fopt-info-vec-optimized -funroll-loops -g -c $< -o $@.o
	$(CXX) $@.o $(MINIAUDIO_OBJ) $(DRWAV_OBJ) $(LIB_PATH) $(LDFLAGS) -o $@
	rm -f $@.o

# Clean build artifacts
clean:
	rm -f $(SRC_DIR)/*.o
	rm -f $(BUILD_DIR)/*.$(LIB_EXT) $(MINIAUDIO_OBJ) $(DRWAV_OBJ)
	rm -f *.o
	rm -f myprog
