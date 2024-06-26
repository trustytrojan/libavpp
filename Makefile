CC = g++
WFLAGS = -Wall -Wextra -Wno-narrowing
IFLAGS = -Iinclude
CFLAGS = $(WFLAGS) $(IFLAGS) -g -std=gnu++23
AV_LIBS = -lavformat -lavcodec -lavutil -lswscale -lswresample
SFML_LIBS = -lsfml-window -lsfml-system -lsfml-graphics -lsfml-audio
PORTAUDIO_LIBS = -lportaudio
SOURCES = $(wildcard tests/*.cpp)
OBJECTS = $(patsubst tests/%.cpp,bin/%,$(SOURCES))

all: clear makedirs $(OBJECTS)

bin/%: tests/%.cpp
	$(CC) $(CFLAGS) $< $(AV_LIBS) $(SFML_LIBS) $(PORTAUDIO_LIBS) -o $@

clear:
	clear

makedirs:
	mkdir -p bin

clean:
	rm -rf bin