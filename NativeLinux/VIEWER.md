# Native Linux vehicle viewer

From the ReRiff project directory:

```sh
python3 scripts/view-vehicle.py
```

The helper finds the usual Steam installation, exports the ALF body/wheels,
builds the native executable if needed, and opens an SDL3 GPU window using the
Vulkan backend. It reads the original archive without changing the installation.
Select another vehicle with `--vehicle bui`, or supply
`--game-dir '/path/to/Riff Racer'` for a different Steam library.

Add `--track` to inspect the car on a reconstructed road with original track
textures and a decoded ramp. See [track reconstruction details](TRACK-SCENE.md).
Use `--section combined` for the connected spline test course, or select
`curved`, `uphill`, `downhill` or `banked`. See [native spline math](SPLINES.md).
Use `--song 'Rolling On'` for the first complete bundled-song reconstruction;
add `--overview` to frame the full route. Home also opens that overview, and 1–4
focus the inspection vehicles. `--list-songs` lists all installed bundled analyses.
See [song tracks](SONG-TRACKS.md) for the current original-game comparison limits.

## Controls

| Input | Action |
| --- | --- |
| Left mouse drag | Orbit around vehicle |
| Mouse wheel | Zoom |
| Right or middle mouse drag | Pan |
| R | Reset camera |
| F | Return to the startup focus |
| 1–4 | Select a vehicle focus in the combined test course |
| Home | Frame entire scene |
| Arrow keys | Move camera horizontally |
| Q / E | Move camera down / up |
| W | Toggle wireframe |
| Esc or close window | Exit |

The window resizes and supports high pixel density displays. Depth testing
keeps nearer surfaces in front. Colors, metalness and roughness use the
approximate inspection materials; shaders add directional and studio lighting.
Track mode samples decoded and statically remapped road, sidewall and ramp
textures. Vehicle materials still use constant approximations. The game's
original shaders, effect shells and shadow plane remain unimplemented. This is
an asset viewer, not a playable game.

## Build dependencies

- C++17 compiler, GNU Make (4.3+), and Python 3.12+ for the preparation helper.
- Pillow for original DDS decoding (`asset-requirements.txt`).
- SDL 3.2+ runtime and Vulkan drivers.
- SDL3 development headers, or let `setup-viewer.py` download the official
  SDL 3.2.0 headers into the ignored `build/native-deps` directory. The download
  is pinned to SHA-256
  `bf308f92c5688b1479faf5cfe24af72f3cd4ce08d0c0670d6ce55bc2ec1e9a5e`.
- System `libshaderc_shared` to compile GLSL to SPIR-V at build time.

On the development Bazzite machine the runtime, compiler and shader compiler
library already existed; only local SDL headers were needed. No system packages
were installed. SDL's license is retained with the local header archive.

Build/run directly:

```sh
python3 NativeLinux/setup-viewer.py
make -C NativeLinux viewer
build/native-linux/reriff-viewer analysis/native-port/vehicle-alf/vehicle.rmesh
```

The viewer is an x86-64 Linux ELF binary linked to SDL3 and the C++ runtime. It
does not launch Windows code, Wine, Proton, Blender, or Python during rendering.
Shaders are read from `shaders/` beside the executable, or `--shaders DIR`.
Keep those compiled shader files with the executable when moving it.

## Verification

The ALF runtime mesh has 23,768 vertices and 16,102 triangles after effect/shadow
nodes are excluded. The larger complete glTF preserves 24,795 vertices and
17,933 triangles. The native loader validates the binary length, counts, finite
vertex values, bounds and every triangle index before uploading buffers.

`make -C NativeLinux test` covers configuration decoding, recovered gameplay
calculations and asset exports, including runtime mesh layout and exclusion of
effect geometry. A Vulkan software-device render was captured and inspected for
vehicle shape, assembled wheels, orientation and shading. The desktop window
also rendered five frames and exited successfully. Pointer interaction has not
been manually exercised.

Capture a Vulkan render without opening a window (P6 PPM format):

```sh
python3 scripts/view-vehicle.py --screenshot /tmp/reriff-vehicle.ppm
```

For a machine with Mesa's software Vulkan ICD, a deterministic CPU-based check:

```sh
VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
  python3 scripts/view-vehicle.py --screenshot /tmp/reriff-vehicle.ppm
```

The screenshot mode selects SDL's offscreen video driver. The ICD path is
distribution-specific. `--frames 5` closes the ordinary desktop window after
five rendered frames, for startup/swapchain checks. Software Vulkan is for
verification; normal use should select the available hardware driver.

## Decoded runtime mesh format

`assets.py` now also writes `vehicle.rmesh`. This is a ReRiff intermediate file,
not an original game format. It contains body/wheel vertices in world space,
with node transforms applied, and the approximation's material values:

| Part | Layout |
| --- | --- |
| Header | Eight bytes `RRMESH1\0`, uint32 vertex count, uint32 index count, six float32 bounds (min XYZ, max XYZ); 40 bytes total |
| Vertices | Eleven float32 values: position XYZ, normal XYZ, color RGB, metallic, roughness; 44 bytes each |
| Indices | uint32 triangle indices |

All fields are little endian; the current native loader targets little-endian
Linux machines. Binary mesh data is generated locally from the installed game
and is excluded from source distribution. This compact interchange boundary
lets the native renderer advance independently of the original archive decoders.

Track mode adds the RRMESH2 textured vertex layout and RRTEX1 texture array;
see [the track format description](TRACK-SCENE.md).
