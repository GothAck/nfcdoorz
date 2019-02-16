BOARD := arduino:avr:mega

SOURCES := $(wildcard *.ino)
TARGETS := $(SOURCES:%.ino=%.hex)
UPLOADS := $(SOURCES:%.ino=upload_%)
PORT:= /dev/ttyACM0

all: $(TARGETS)

%.hex: %.ino Makefile
	arduino-cli compile -b $(BOARD) $< -o $(@:%.hex=%)

upload_%: %.hex
	arduino-cli upload -b $(BOARD) -i $< -p $(PORT)

upload: $(UPLOADS)

clean:
	-rm $(TARGETS)
