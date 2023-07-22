# Executable name
BIN=tcsynth

# Path to microcontroller code
MCU=../Tesla_Coil_MIDI_Synth

# Files to compile and patch if necessary
SOURCES=seded/MIDI.cpp seded/Synth.cpp Coil.cpp
HEADERS=seded/MIDI.h seded/Synth.h patched/Voice.h Coil.h

# Get portaudio flags from pkg-config
PORTAUDIO_CFLAGS:=$(shell pkg-config --cflags portaudiocpp)
PORTAUDIO_LIBS:=$(shell pkg-config --libs portaudiocpp)

# Specify libremidi based on your platform
# see https://github.com/jcelerier/libremidi/blob/master/docs/header-only.md
LIBREMIDI_CFLAGS:=-DLIBREMIDI_HEADER_ONLY=1 -DLIBREMIDI_COREAUDIO=1
LIBREMIDI_LIBS:=-framework CoreMIDI -framework CoreAudio -framework CoreFoundation

INCLUDE=-I. -Ibuild/seded -Ibuild/patched -I$(MCU) -Ilibremidi/include
LIBS=$(PORTAUDIO_LIBS) $(LIBREMIDI_LIBS)
CXXFLAGS=-O2 -Wall -Wextra -Wno-unused-parameter -std=c++17 -DF_CPU=84000000 $(INCLUDE) $(PORTAUDIO_CFLAGS) $(LIBREMIDI_CFLAGS)

OBJS:=$(SOURCES:%.cpp=build/%.o)
HEADERS:=$(HEADERS:patched/%=build/patched/%)
HEADERS:=$(HEADERS:seded/%=build/seded/%)

# Link final executable
$(BIN): main.cpp $(OBJS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LIBS) -o $@ main.cpp $(OBJS)

# Patch microcontroller sources
build/patched/%: $(MCU)/% patches/%.patch
	@mkdir -p $(@D)
	@patch -o $@ $^

# Modify files with sed
build/seded/%: build/patched/% sed-scripts/%.sed
	@mkdir -p $(@D)
	cp $< $@
	sed -Ef $(word 2,$^) -i '' $@

# Generate object files
build/%.o: %.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

build/%.o: build/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

.PHONY: clean patches

clean:
	rm -rf $(BIN) build

# Create patches from edited files in build/patched
patches:
	for i in build/patched/*.{cpp,h}; do j=$$(basename $$i); diff $(MCU)/$$j $$i > patches/$$j.patch || true; done
