CXX ?= c++
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -Werror
OUT ?= ../build/native-linux
SDL_CFLAGS ?= $(shell pkg-config --cflags sdl3 2>/dev/null || echo -I../build/native-deps/SDL3-3.2.0/include)
SDL_LIBS ?= $(shell pkg-config --libs sdl3 2>/dev/null || echo -l:libSDL3.so.0)

all: $(OUT)/reriff-native

$(OUT)/reriff-native: main.cpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) main.cpp -o $@ -lz

$(OUT)/test-gameplay: test_gameplay.cpp gameplay.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) test_gameplay.cpp -o $@

test: all track-math $(OUT)/test-gameplay
	$(OUT)/test-gameplay
	python3 ../tests/test_native_linux.py $(OUT)/reriff-native
	python3 ../tests/test_vehicle_assets.py
	python3 ../tests/test_track_scene.py
	python3 ../tests/test_splines.py
	python3 ../tests/test_song_tracks.py

$(OUT)/shaders/vehicle.vert.spv $(OUT)/shaders/vehicle.frag.spv &: shaders/vehicle.vert shaders/vehicle.frag compile-shaders.py
	python3 compile-shaders.py $(OUT)/shaders

$(OUT)/reriff-viewer: viewer.cpp $(OUT)/shaders/vehicle.vert.spv $(OUT)/shaders/vehicle.frag.spv
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) viewer.cpp -o $@ $(SDL_LIBS)

viewer: $(OUT)/reriff-viewer

$(OUT)/libtrackmath.so: track_math.cpp track_math.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) -ffp-contract=off -fPIC -shared track_math.cpp -o $@

track-math: $(OUT)/libtrackmath.so

.PHONY: all test viewer track-math
