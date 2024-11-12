# Executable name
BIN=tcsynth

# Path to microcontroller code
MCU=../Tesla_Coil_MIDI_Synth

# Files to compile and patch if necessary
SOURCES=\
	patched/MIDI.cpp \
	patched/Synth.cpp \
	patched/Voice.cpp \
	patched/Audio.cpp \
	patched/LCD.cpp \
	patched/Knob.cpp \
	AudioMode.cpp \
	AudioModeBinary.cpp \
	AudioModeBinaryDDT.cpp \
	AudioModeClampedBinary.cpp \
	AudioModePWM.cpp \
	AudioModePWMDDT.cpp \
	AudioModePredictive.cpp \
	AudioModePulseEnergy.cpp \
	AudioModeSchmitt.cpp \
	Drum.cpp \
	BarChars.cpp \
	Coil.cpp \
	AudioEngine.cpp \
	Convolution.cpp \
	LiquidCrystal.cpp \
	ir.s \
	LCDFont.s

HEADERS=\
	patched/MIDI.h \
	patched/Synth.h \
	patched/Voice.h \
	patched/Audio.h \
	patched/LCD.h \
	patched/Knob.h \
	Coil.h \
	AudioEngine.h \
	Convolution.h \
	LiquidCrystal.h \
	RingBuffer.h

# Get portaudio flags from pkg-config
PORTAUDIO_CFLAGS:=$(shell pkg-config --cflags portaudiocpp)
PORTAUDIO_LIBS:=$(shell pkg-config --libs portaudiocpp)
FFTW3_CFLAGS:=$(shell pkg-config --cflags fftw3)
FFTW3_LIBS:=$(shell pkg-config --libs fftw3) -lfftw3f
SDL2_CFLAGS:=$(shell pkg-config --cflags sdl2 SDL2_ttf)
SDL2_LIBS:=$(shell pkg-config --libs sdl2 SDL2_ttf)

# Specify libremidi based on your platform (some auto-detection below)
# see https://github.com/jcelerier/libremidi/blob/master/docs/header-only.md
LIBREMIDI_CFLAGS:=-DLIBREMIDI_HEADER_ONLY=1
LIBREMIDI_LIBS:=

INCLUDE=-I. -Ibuild/seded -Ibuild/patched -I$(MCU) -Ilibremidi/include
LIBS=$(OTHER_LIBS) $(PORTAUDIO_LIBS) $(FFTW3_LIBS) $(SDL2_LIBS) $(LIBREMIDI_LIBS)
CXXFLAGS=$(TARGET) -O2 -Wall -Wextra -Wno-unused-parameter -std=c++17 -DF_CPU=84000000 $(INCLUDE) $(PORTAUDIO_CFLAGS) $(FFTW3_CFLAGS) $(SDL2_CFLAGS) $(LIBREMIDI_CFLAGS)

OBJS:=$(SOURCES:%.cpp=build/%.o)
HEADERS:=$(HEADERS:patched/%=build/patched/%)

# OS-specific overrides
UNAME:=$(shell uname)

# macOS
ifeq ($(UNAME), Darwin)
	# Configure libremidi
	LIBREMIDI_CFLAGS+=-DLIBREMIDI_COREAUDIO=1
	LIBREMIDI_LIBS+=-framework CoreMIDI -framework CoreAudio -framework CoreFoundation
endif

# Linux
ifeq ($(UNAME), Linux)
	# Configure libremidi
	LIBREMIDI_CFLAGS+=-DLIBREMIDI_ALSA=1
	LIBREMIDI_LIBS+=-lasound -pthread
endif

# Windows
ifneq (,$(findstring NT,$(UNAME)))
	# Configure libremidi
	LIBREMIDI_CFLAGS+=-DLIBREMIDI_WINMM=1
	LIBREMIDI_LIBS+=-lwinmm

	# Windows is reluctant to reveal the value of PI
	CXXFLAGS+=-D_USE_MATH_DEFINES

	# Always make static build
	LIBS+=-lole32 -lsetupapi
	CXXFLAGS+=-static
endif

# Check for release build
ifdef RELEASE_BUILD
	CXXFLAGS+=-DRELEASE_BUILD
endif

# Link final executable
$(BIN): main.cpp $(OBJS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ main.cpp $(OBJS) $(LIBS)

# Create macOS universal binary
.NOTPARALLEL: $(BIN)_universal
$(BIN)_universal: $(BIN)_x64 $(BIN)_arm64
	lipo -create -output $@ $^
	rm $(BIN)_x64 $(BIN)_arm64

# Pull static libportaudio.a and libportaudiocpp.a files from static/x64 and static/arm64
$(BIN)_x64:
	TARGET="-target x86_64-apple-macos11" OTHER_LIBS=-Lstatic/x64 make -eB
	rm -rf build
	mv $(BIN) $@

$(BIN)_arm64:
	TARGET="-target arm64-apple-macos11" OTHER_LIBS=-Lstatic/arm64 make -eB
	rm -rf build
	mv $(BIN) $@

# Patch microcontroller sources
build/patched/%: $(MCU)/% patches/%.patch
	@mkdir -p $(@D)
	@cp $< $@
	@patch $@ $(word 2,$^)

# Generate object files
build/%.o: %.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

build/%.o: build/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

build/%.o: $(MCU)/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

.PHONY: clean patches $(BIN)_universal $(BIN)_x64 $(BIN)_arm64

clean:
	rm -rf $(BIN) $(BIN)_* build

# Create patches from edited files in build/patched
patches:
	for i in build/patched/*.{cpp,h}; do j=$$(basename $$i); diff -u $(MCU)/$$j $$i > patches/$$j.patch || true; done
