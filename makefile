CC = gcc
CFLAGS = -Wall -g
KISSFFT_DIR = kissfft-master

TARGET := $(MAKECMDGOALS)
OBJS := $(TARGET).o $(KISSFFT_DIR)/kiss_fft.o $(KISSFFT_DIR)/kiss_fftr.o

.PHONY: clean

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KISSFFT_DIR)/%.o: $(KISSFFT_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(KISSFFT_DIR)/*.o $(TARGET)
