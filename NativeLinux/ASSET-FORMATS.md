# Vehicle mesh and texture decoding

The installed `Game.exe` was decompiled with ILSpyCmd 9.1.0.7988 into
`analysis/native-port/decompiled/Module.cs` (319,746 lines). The generated C# is
research output, not compilable recovered game source. Most engine graphics
loaders are native machine code, outside ILSpy's C# reconstruction. The module
does expose the game-side node callbacks, matrix handling, and graphics resource
interfaces. Native loader disassembly and the assets supply the layout evidence
below. No original executable or asset was changed.

## Reproduce the export

From the project root, with Pillow installed (the tested version is pinned in
`NativeLinux/asset-requirements.txt`):

```sh
python3 NativeLinux/assets.py \
  --game-dir '/path/to/Riff Racer' --vehicle alf \
  --output analysis/native-port/vehicle-alf
```

This produces `vehicle.gltf` with `vehicle.bin`, the original DDS payloads,
decoded PNG textures, all six environment cube faces, `texture-sheet.png`,
`geometry-preview.png`, and a manifest with source hashes and shader parameters.
Keep the glTF, BIN, and textures directory together. All exported game material
is local research output in the ignored `analysis/` directory.

For Blender inspection, import `vehicle-inspection.gltf`: it includes only the
body and wheel nodes. The complete `vehicle.gltf` also includes electric-effect
shells and the shadow plane, which appear opaque without the original shaders
and obscure the detailed body in Blender. Both files share `vehicle.bin` and
the textures directory. Start with an empty Blender scene to avoid overlap with
its default cube. Solid viewport mode displays gray geometry; material preview
shows the approximate exported materials.

The inspection file automatically substitutes PBR approximations for paint,
glass, wheels, metal, rubber, trim and lights. Paint uses the recovered `g_color0`
when present. Yellow shader-mask textures are disconnected for these materials.
Glass is opaque dark blue-gray for predictable inspection. These presets improve
readability and do not reproduce the original game's shaders. The complete
`vehicle.gltf` retains its original texture mappings and parameter metadata.

The geometry preview uses diagnostic colors and flat lighting, and displays
only body/wheel nodes. The glTF preserves all 26 mesh sections, including electric
effects and the shadow plane. Its basic materials are approximations: several
original DDS channels encode shader inputs rather than ordinary diffuse color.
The exporter does not implement the original shaders, effects, animated livery,
paint overrides, alpha rules, reflection lookup, or damage.

Run synthetic and installed-asset checks:

```sh
make -C NativeLinux test
RERIFF_GAME_DIR='/path/to/Riff Racer' python3 tests/test_vehicle_assets.py
```

Reproduce ILSpy output:

```sh
mkdir -p analysis/native-port/decompiled
DOTNET_ROOT=/tmp/reriff-dotnet build/tools/ilspycmd --disable-updatecheck \
  -t '<Module>' '/path/to/Riff Racer/Game.exe' \
  > analysis/native-port/decompiled/Module.cs
```

The module takes several minutes to decompile. `.NET` under `/tmp` is temporary.
Native disassembly was generated separately using `objdump -d -Mintel`.
Addresses below apply to executable SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`,
image base `0x400000`.

## Native loader evidence

| Virtual address | Observed behavior |
| --- | --- |
| `0x51191a` | Compare texture-batch magic against `TABT` |
| `0x511922` | Require texture-batch version 4 |
| `0x511952`–`0x51197a` | Derive 24-byte entry array at header +28, followed by names and payload |
| `0x5249a0` | Entry of EADB header/setup routine |
| `0x5249aa` | Compare model magic against `EADB` |
| `0x5249be` | Require model version `0x26` (38) |
| `0x5249d6`–`0x524a15` | Add file-relative offsets from header `0x44` through `0x60` to file base |
| `0x543106` | Call imported `D3DXCreateCubeTextureFromFileInMemory` |
| `0x543146` | Call imported `D3DXCreateTextureFromFileInMemoryEx` |

The original loader's DDS handoff agrees with finding a complete DDS image at
each TABT payload offset. ILSpy's `gaVehicleVis.setWheelVars` also confirms that
wheel visual transforms are copied as 48-byte matrices. Metadata-only structs
and empty method stubs should not be mistaken for complete loader source.

## PC TABT version 4

All integers below are little endian. Header is 28 bytes:

| Offset | Meaning |
| --- | --- |
| `0x00` | `TABT` |
| `0x04` | Observed marker `0xffee` |
| `0x08` | Version 4 |
| `0x0c` | Absolute payload offset |
| `0x10` | Texture count |
| `0x14` | Name pool byte size |
| `0x18` | Payload byte size |

Each following 24-byte entry has six uint32 fields: offset into name pool,
offset into payload, payload length, width, height, and flags. The name pool
immediately follows the entries. Names are NUL terminated. Each payload is a
complete DDS file, with its own dimensions, compression, mip chain, and cube
flags. TABT flags are preserved without assigning broader meaning.

The ALF batch contains 16 DDS files using DXT1/DXT5. One, `_lobbycube`, is a
six-face cubemap (`DDS caps2 = 0xfe00`); its faces are exported separately in
DDS order +X, -X, +Y, -Y, +Z, -Z. Mip chains remain in the original DDS files;
PNGs represent the base level. Texture names match material references after
removing the `.tga` suffix and ignoring case.

## PC EADB version 38

Header is 128 bytes. Important uint32 fields:

| Offset | Meaning |
| --- | --- |
| `0x00` | `EADB` |
| `0x04` | Version 38 |
| `0x08` | Observed PC variant 1 |
| `0x0c` | Material count |
| `0x10` | Shader parameter count |
| `0x14` | Node count |
| `0x18` | Mesh section count |
| `0x44` | Material table offset |
| `0x48` | Shader parameter table offset |
| `0x4c` | Node table offset |
| `0x54` | Mesh section table offset |
| `0x58` | Signed int16 link array offset |
| `0x5c` | Vertex buffer offset |
| `0x60` | Index buffer offset |

Material records are 464 bytes: a 64-byte name, eight 32-byte texture names
starting at +64, runtime/unknown fields, parameter count at +436, first parameter
index at +440, and a shader identifier at +444. Runtime-looking pointer fields
are ignored. Parameter records are 128 bytes with a 64-byte name, an observed
runtime/unknown word of 0 or 2 at +100 and four float32 values at +112.
That word is preserved without interpreting it as a parameter type.
These preserve settings such as
`g_color0`, specular response, flake scale, and reflection strength; their exact
shader interpretation remains to be ported.

Node records are 156 bytes: a 32-byte name, twelve float32 transform components
at +32, and first mesh-link index at +120. The ALF transforms have identity
bases followed by local translations. Its four wheel translations and body
translation assemble correctly in the diagnostic preview. Mesh links are signed
int16 section indices terminated by -1. This exporter targets the observed
vehicle layout, not arbitrary animated/skeletal EADB files. More complex node
hierarchies and non-identity rotations need independent visual validation.

Mesh records are 48 bytes. The first six uint32 values are packed flags,
vertex stride, first vertex, vertex count, first index, and index count. Low
12 flag bits select the material. A vertex is at:

```
vertex_buffer + (first_vertex + local_vertex) * stride
```

The first-vertex value is specific to its stride: do not assume one uniform
global vertex array. Indices are uint16 values and include `first_vertex`;
subtract it to get mesh-local indices. Consecutive triples describe triangles.

Observed vertex prefix: position FLOAT3 at +0, normal HALF4 at +12, UV0 HALF2
at +20. ALF uses strides 24, 28 and 36; trailing bytes are retained by the decoder
but their color/tangent/additional-UV semantics are not fully resolved. Exported
glTF normals are normalized to remove half-float rounding error. UV0 is exported
without flipping. Coordinate handedness and face winding still require comparison
with the running game; the diagnostic export uses double-sided materials.

## Validation and remaining work

ALF: 16 materials, 86 shader parameters, nine nodes, 26 mesh sections,
24,795 vertices and 17,933 triangles. Every index is within its section's vertex
range, all decoded coordinates are finite, and all mesh sections are referenced
by nodes. All 16 installed `lod0_livery.bdae` vehicle files also pass geometry
layout checks. These checks establish structural decoding, not visual parity for
every vehicle. The inspected ALF preview confirms a recognizable assembled car
with correctly placed wheels.

The decoded body/wheel buffers now render in a native SDL3/Vulkan viewer with
orbit controls and approximate paint/metal/glass materials. See [the viewer
instructions](VIEWER.md). Track mode samples decoded road/ramp textures with
static mask colors; vehicle texture effects and reflection shaders still need
reconstruction. See [track scene findings](TRACK-SCENE.md). A playable Linux game
remains a later milestone.
