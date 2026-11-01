# Road reconstruction and native inspection scene

Launch from the project root:

```sh
build/native-linux/reriff --track
```

This displays the original-scale ALF vehicle, a reconstructed straight road,
sidewalls, and the decoded original Dance ramp model. Optional flags:

```sh
build/native-linux/reriff --track --genre rock --vehicle bui
build/native-linux/reriff --track --overview
```

Use left-drag to orbit, the wheel to zoom, right-drag to pan, arrow keys to move
the camera horizontally, and Q/E to lower/raise it. F or R returns to the startup
scene view; Home frames the entire scene. W or Tab toggles wireframe.
These controls move the camera, not the car.

## What was recovered

The archive has road textures but no standalone road BDAE mesh. Decompiled
`gaTrackStrip.spawnNewRoadSegment` constructs the road dynamically from splines.
The relevant executable methods are:

| Method | MethodDef token | RVA |
| --- | --- | --- |
| `gaNode.{ctor}` (default widths/heights) | `0x06000464` | `0x204fa0` |
| `gaTrackStrip.makeSplines` | `0x060009ee` | `0x26d760` |
| `gaTrackStrip.spawnNewRoadSegment` | `0x060009f0` | `0x26db40` |
| `gaTrackStrip.spawnNewWallSegment` | `0x060009f1` | `0x26e33c` |

Evidence is in the local IL disassemblies and ILSpy's
`analysis/native-port/decompiled/Module.cs`. The same original executable
identity recorded in `ASSET-FORMATS.md` applies.

The straight specialization reconstructs these original rules:

- Nine cross-section vertices at lateral offsets `3*i - 12`.
- Per-side width remapping `width * 0.6666666865348816 + 0.3333333432674408`.
  Default node width factors are 1, yielding a 24-unit-wide road.
- The negative offsets use the right width factor and nonnegative offsets use
  the left factor, following the recovered branch.
- Spline parameter advances by `3/20` per sample.
- UVs are `(scaled_lateral/24 + 0.5, parameter * 1.600000023841858)`.
- Sixteen triangles connect consecutive nine-vertex rows. Their recovered
  index order points upward on the straight fixture.
- Seven vertices describe each visual wall profile, including the underside
  descending to centerline Y=-8. Visual wall height defaults to 1. This is the
  visual profile, not the distinct collision profile in the same method.

The original uses `elSpline.evaluate`, `evaluateTangent`, and `elMatrix.setOrientation`
to obtain curved/banked positions. Those native spline methods have not yet been
ported. The scene instead specifies a straight, level centerline with 20-unit
control spacing, 13 rows at three-unit intervals and total length 36. This
spacing/length is an inspection fixture, not recovered song data. Width,
cross-section topology and UV rules come from the original code.

## Original assets and scale

The scene decodes these installed assets without modifying them:

- `graphics/genre/dance/road/batchTextures.brres`
- `graphics/genre/dance/sidewall/batchTextures.brres`
- `graphics/genre/dance/ramps/ramp_8m.bdae`
- `graphics/genre/dance/ramps/textures/batchTextures.brres`
- `scripts/config/visualSections.cfg`

Rock and Hip-hop use their corresponding genre directories. `scene-manifest.json`
records source hashes, the palette, placements, bounds and counts.

No vehicle/ramp scale factors are introduced. ALF's inspection geometry is about
1.862 units wide and 4.536 units long; the road is 24 units wide. Vehicle +Z is
the visual forward direction, +Y is up, and the road lies at Y=0. The car is
translated to Z=-8. Its lowest wheel vertex is shifted by approximately -0.000751
in Y so the wheel minimum is exactly zero. This is geometric placement; it does
not establish the original physics resting height or collision margins.

The original ramp is decoded with its three mesh sections and material texture
references. Its local geometry is mostly flat; the game applies additional
transforms at runtime. The inspection places it flat at `(0,0.03,16)` at authored
scale. Dynamic ramp rotation, collision and animation are still pending.

## Textures and shader limits

Road/sidewall DDS files contain RGB shader masks. The original road material
binds `road` and `steps` textures and uses animated shader settings; the viewer
currently samples `road`, not the complete multi-texture effect. A static
approximation maps RGB mask channels to the recovered genre's base/accent/pulse
colors from `visualSections.cfg`. This mapping is for inspection and is not a
reconstruction of the original shader. Music pulses, emissive glow, ramps'
animated atlas stepping, transparency, shadows and reflections are absent.

The full decoded source DDS and PNGs remain in the scene's `textures/` folder.
For native upload, the exporter builds 512x512 sRGB RGBA layers: white,
road, sidewall, and the ramp's referenced images (six total for Dance/Rock,
seven for Hip-hop). Only base
levels are uploaded; original DDS mip chains remain in the source copies.

## Native interchange and verification

Scene data is written under `analysis/native-port/track-dance-alf/` by default.
These are generated local game assets and remain excluded from source releases.

`scene.rmesh` uses the new `RRMESH2\0` header. Counts and bounds retain the
40-byte RRMESH1 header layout. Each vertex is now fourteen float32 values (56
bytes): position XYZ, normal XYZ, material color RGB, metallic, roughness, UV,
texture layer. Indices remain uint32 triangle indices. The viewer continues
to accept old RRMESH1 files, supplying white texture layer zero internally.

`scene.rtex` contains eight bytes `RRTEX1\0\0`, uint32 width, height and layer
count, then tightly packed RGBA8 layer data. The viewer validates its extent,
file length and every vertex's texture layer before upload. Its sampling uses an
SDL3 GPU texture array and Vulkan fragment sampler.

The ALF/Dance sample has 24,725 vertices and 16,790 triangles in total. Tests
verify width remapping, original triangle order/upward normals, mirrored wall
profiles, config palette decoding, malformed config rejection and RRMESH2 binary
layout. Vulkan offscreen rendering was inspected for original road texture
patterns, assembled car placement and the ramp. A desktop-window test rendered
five frames successfully. The legacy vehicle-only Vulkan capture is byte-for-byte
unchanged after introducing texture-array support. Mouse/keyboard interaction
has not been manually exercised.

Rock and Hip-hop were also decoded and rendered offscreen. Their ramp texture
references differ from Dance and are resolved by material name rather than
assuming one shared set of image names.

Build checks and capture:

```sh
make -C NativeLinux test
VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
  build/native-linux/reriff --track --screenshot /tmp/reriff-track.ppm
```

The Mesa software ICD path is distribution-specific. Native screenshot output
is P6 PPM. The next reconstruction boundary is supplying a recovered sequence of
centerline/width/up-vector samples to replace this straight fixture, then
validating curved road geometry before adding driving physics. Curved, sloped
and banked fixtures using the recovered original math are now available; see
[spline reconstruction and validation](SPLINES.md).
