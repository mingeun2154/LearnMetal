APP_00WINDOW_OBJECTS=study-metal/00-window/00-window.o

CC=g++

CFLAGS=-Wall -std=c++17 -I./metal-cpp -I./metal-cpp-extensions -fno-objc-arc
LDFLAGS=-framework Metal -framework Foundation -framework Cocoa -framework CoreGraphics -framework MetalKit

VPATH=./metal-cpp


%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

all: build/00-window

.PHONY: all

build/00-window: $(APP_00WINDOW_OBJECTS) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(APP_00WINDOW_OBJECTS) -o $@

clean:
	rm -f $(APP_00WINDOW_OBJECTS) \
		build/00-window
