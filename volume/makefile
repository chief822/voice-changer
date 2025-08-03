CC = gcc
CFLAGS = -Wall -g
TARGET = myprogram
SOURCES = myprogram.c miniaudio.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o
