TARGET = $(MAKECMDGOALS)

$(TARGET): $(TARGET).o build/libwebrtc_vad.lib
	gcc $@.o build/libwebrtc_vad.lib -o $@

%.o: %.c
	gcc -c $< -o $@

.PHONY: clean

clean:
	rm -f *.o main
