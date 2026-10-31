CXX ?= c++
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off
OUT ?= ../build/native-linux
SDL_CFLAGS ?= $(shell pkg-config --cflags sdl3 2>/dev/null || echo -I../build/native-deps/SDL3-3.2.0/include)
SDL_LIBS ?= $(shell pkg-config --libs sdl3 2>/dev/null || echo -l:libSDL3.so.0)
CORE = archive.hpp native_common.hpp third_party/nlohmann/json.hpp
SONG = $(CORE) native_song.hpp track_math.hpp
SCENE = $(SONG) native_assets.hpp native_scene.hpp playback.hpp
REFERENCE ?= reference/vehicle-behavior

all: $(OUT)/reriff-native $(OUT)/reriff $(OUT)/reriff-reference viewer

$(OUT)/test-vehicle-behavior: test_vehicle_behavior.cpp vehicle_behavior.hpp vehicle_body.hpp vehicle_suspension.hpp vehicle_tires.hpp driving.hpp $(CORE) track_math.hpp playback.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) test_vehicle_behavior.cpp -o $@ -lz -lcrypto

test-vehicle-behavior: $(OUT)/test-vehicle-behavior
	$(OUT)/test-vehicle-behavior $(REFERENCE) $(OUT)/behavior-parity.json

$(OUT)/reriff-native: main.cpp archive.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) main.cpp -o $@ -lz

$(OUT)/reriff: prepare_main.cpp $(SCENE)
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) prepare_main.cpp -o $@ -lz -lcrypto -lpng

$(OUT)/reriff-reference: reference_main.cpp native_reference.hpp $(SONG) playback.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) reference_main.cpp -o $@ -lz -lcrypto

$(OUT)/compile-shaders: compile_shaders.cpp archive.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) compile_shaders.cpp -o $@ -ldl -lz

$(OUT)/shaders/vehicle.vert.spv $(OUT)/shaders/vehicle.frag.spv &: shaders/vehicle.vert shaders/vehicle.frag $(OUT)/compile-shaders
	$(OUT)/compile-shaders shaders $(OUT)/shaders

sdl-headers:
	../scripts/setup-native-sdl.sh

$(OUT)/reriff-viewer: viewer.cpp playback.hpp song_audio.hpp track_math.hpp $(OUT)/shaders/vehicle.vert.spv $(OUT)/shaders/vehicle.frag.spv | sdl-headers
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) viewer.cpp -o $@ $(SDL_LIBS)

viewer: $(OUT)/reriff-viewer

$(OUT)/libtrackmath.so: track_math.cpp track_math.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) -fPIC -shared track_math.cpp -o $@

track-math: $(OUT)/libtrackmath.so

$(OUT)/test-native: test_native.cpp $(SCENE)
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) test_native.cpp -o $@ -lz -lcrypto -lpng

$(OUT)/test-gameplay: test_gameplay.cpp gameplay.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) test_gameplay.cpp -o $@

$(OUT)/test-playback: test_playback.cpp playback.hpp track_math.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) test_playback.cpp -o $@

$(OUT)/test-audio: test_audio.cpp song_audio.hpp | sdl-headers
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) test_audio.cpp -o $@ $(SDL_LIBS)

test-audio: $(OUT)/test-audio
	SDL_AUDIODRIVER=dummy $(OUT)/test-audio

test: all $(OUT)/test-driving $(OUT)/test-native $(OUT)/test-gameplay $(OUT)/test-playback test-audio test-vehicle-behavior
	$(OUT)/test-native
	$(OUT)/test-gameplay
	$(OUT)/test-playback
	$(OUT)/test-driving

.PHONY: all viewer track-math test test-audio test-vehicle-behavior sdl-headers

$(OUT)/test-driving: test_driving.cpp driving.hpp $(SCENE)
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) test_driving.cpp -o $@ -lz -lcrypto -lpng
