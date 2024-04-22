# Executable name
BIN=tcsynth

# Path to microcontroller code
MCU=../Tesla_Coil_MIDI_Synth

# Files to compile and patch if necessary
SOURCES=seded/MIDI.cpp seded/Synth.cpp Drum.cpp Coil.cpp AudioEngine.cpp
HEADERS=seded/MIDI.h seded/Synth.h patched/Voice.h Coil.h AudioEngine.h RingBuffer.h

# Get portaudio flags from pkg-config
PORTAUDIO_CFLAGS:=$(shell pkg-config --cflags portaudiocpp)
PORTAUDIO_LIBS:=$(shell pkg-config --libs portaudiocpp)

# Specify libremidi based on your platform (some auto-detection below)
# see https://github.com/jcelerier/libremidi/blob/master/docs/header-only.md
LIBREMIDI_CFLAGS:=-DLIBREMIDI_HEADER_ONLY=1
LIBREMIDI_LIBS:=

INCLUDE=-I. -Ibuild/seded -Ibuild/patched -I$(MCU) -Ilibremidi/include
LIBS=$(OTHER_LIBS) $(PORTAUDIO_LIBS) $(LIBREMIDI_LIBS)
CXXFLAGS=$(TARGET) -O2 -Wall -Wextra -Wno-unused-parameter -std=c++17 -DF_CPU=84000000 $(INCLUDE) $(PORTAUDIO_CFLAGS) $(LIBREMIDI_CFLAGS)

OBJS:=$(SOURCES:%.cpp=build/%.o)
HEADERS:=$(HEADERS:patched/%=build/patched/%)
HEADERS:=$(HEADERS:seded/%=build/seded/%)

# sed in-place command syntax
INPLACE:=-i

# OS-specific overrides
UNAME:=$(shell uname)

# macOS
ifeq ($(UNAME), Darwin)
	INPLACE:=-i '' # BSD sed requires different syntax
	
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

# Modify files with sed
build/seded/%: build/patched/% sed-scripts/%.sed
	@mkdir -p $(@D)
	cp $< $@
	sed -Ef $(word 2,$^) $(INPLACE) $@

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
	for i in build/patched/*.{cpp,h}; do j=$$(basename $$i); diff $(MCU)/$$j $$i > patches/$$j.patch || true; done
