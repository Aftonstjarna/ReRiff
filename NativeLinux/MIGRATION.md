# C++ migration

The native port builds, decodes original assets, generates tracks, validates
geometry, prepares playback and runs without a Python interpreter. No converted
asset cache is required: `reriff` reads the installed game directly.

```sh
make -C NativeLinux all test
build/native-linux/reriff --song 'Rolling On' --play
```

`scripts/view-native.sh` is an optional shell launcher that builds first.
Existing Python script names are short compatibility argument adapters only;
they execute the same native programs. The optional read-only IL metadata survey
remains a separate Python research tool; it is not a build or runtime dependency.
The C# Microsoft CLR reference host is also diagnostic tooling, invoked only by
an explicit `reriff-reference capture` command.

| Component | Native source |
| --- | --- |
| Checked ZIP / raw deflate / CRC and GFCF records | `archive.hpp` |
| JSON, hashing, binary I/O and process execution | `native_common.hpp` |
| Packed SNG0 analysis and original segment generation | `native_song.hpp` |
| EADB meshes, half floats, TABT / DDS, PNG and glTF | `native_assets.hpp` |
| Road/wall meshes, wheel support validation and playback preparation | `native_scene.hpp` |
| Recovered spline/orientation math | `track_math.hpp` |
| Song-distance timing | `playback.hpp` |
| SDL audio clock, pause and seek | `song_audio.hpp` |
| Native preparation / launch command | `prepare_main.cpp` |
| Original capture and comparison commands | `native_reference.hpp`, `reference_main.cpp` |
| Shader compilation | `compile_shaders.cpp` |
| Synthetic and installed-data checks | `test_native.cpp`, `test_playback.cpp`, `test_audio.cpp` |

Build dependencies are a C++17 compiler, GNU Make 4.3+, zlib, OpenSSL and libpng
headers/libraries, SDL3, Vulkan and `libshaderc_shared`. The shell bootstrap can
fetch the pinned official SDL headers when system development headers are absent.
FFmpeg is used as an external audio decoder. JSON uses the vendored MIT-licensed
nlohmann/json 3.11.3 single header, with its license retained under `third_party/`.

The existing original-game comparisons remain available natively:

```sh
build/native-linux/reriff --song 'Rolling On' --play --mute --prepare-only \
  --reference-nodes analysis/native-port/original-rolling-on/original-nodes.json
build/native-linux/reriff-reference math \
  --game-dir '/path/to/Riff Racer' \
  --nodes analysis/native-port/song-17sjwbbsrv-dance-alf/song-nodes.json
build/native-linux/reriff-reference timing
build/native-linux/reriff-reference capture --song 'Rolling On'
```

The original-capture command requires the previously prepared, isolated Microsoft
.NET prefix. Ordinary native generation and playback do not use Wine or .NET.
Original executable bytes, decoded audio, raw reference dumps and exported assets
stay in ignored `build/` / `analysis/` directories.

Rolling On retains all 635 node values and 132 segment choices/parameters from the
previous implementation. Against the independent original capture, maximum
position error remains 0.0079461 world units, below 0.01. Widths, wall heights,
flags and visibility match exactly. Native timing is numerically identical on
101 samples with original cumulative distances; the Linux geometry's maximum
parameter difference remains 0.000091553 of a node.

The port retains the measured original random API and explicit floating-point
rounding. Compiles disable contraction. Direct original x86 tests cover 32,000
random values, 100 matrix products, 800 spline evaluations and 400 orientations.
Installed-data tests also decode 26 configurations, 16 vehicle meshes and all 25
bundled songs, generate every bundled dance track, and validate five road fixtures.

Texture inspection masks now use native bilinear resampling in place of Pillow's
Lanczos. These approximate textures can differ; generation state and geometry are
unaffected. Vehicle glTF, body/wheel inspection glTF, DDS, decoded PNG/cube faces
and runtime meshes are preserved. The previous Python contact-sheet and software
preview images are replaced by the native viewer and its screenshot option.

The placeholder driving integrator has been removed. See [original physics recovery](DRIVING.md)
for located routines and retained scalar parity checks. Original rigid-body
integration, animated shaders and hardware audio latency remain unfinished.

The C++ vehicle behavior layer now also ports original pace/auto/manual control,
drift-input state, steering angle, drive impulses, grip/friction, adhesion,
skid state, skid forces and yaw damping.
The native force/torque accumulation and acceleration conversions are also ported.
Suspension spring, damping, adhesion and load smoothing are ported as well.
Lateral tire forces and front/rear drive allocation now match four-wheel reference cases.
The regular tests replay 21,447 original-method cases with 126,703 output checks;
all currently match numerically exactly. See [physics recovery](PHYSICS-RECOVERY.md)
for coverage and the remaining force/contact integration.
