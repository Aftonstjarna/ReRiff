# Recovered spline math and test sections

From the project root:

```sh
build/native-linux/reriff --section combined
```

The connected course contains a bend, ascent, crest/descent and banking, with
four rigid vehicle poses. Press **1–4** to focus a car, **Home** for the whole
course, **F/R** to restore the selected focus, and use orbit/zoom or arrow/Q/E
camera controls. The starting elevation is raised to see over banked sidewalls.

Individual fixtures: `--section curved`, `--section uphill`,
`--section downhill`, or `--section banked`. `--section` implies `--track`.
The existing genre, vehicle, screenshot, overview and game-directory flags work.
Omitting the section preserves the previous straight and vehicle-only modes.

## Recovered source

Original executable SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
The metadata marks these as native routines, so their x86 instructions were
inspected rather than treating ILSpy's external declarations as source.

| Routine | MethodDef | RVA | Virtual address |
| --- | --- | --- | --- |
| `elSpline.evaluate` | `0x06001656` | `0x1d0030` | `0x5d0030` |
| `elSpline.evaluateTangent` | `0x06001657` | `0x1d0360` | `0x5d0360` |
| `elMatrix.setOrientation` | `0x0600164c` | `0x582a0` | `0x4582a0` |
| Vector normalization helper | — | `0x26d90` | `0x426d90` |

`track_math.hpp` contains the C++ port, with a checked C ABI in `track_math.cpp`.
`native_scene.hpp` calls the recovered C++ evaluator and orientation routines
directly. There is no scripting boundary or alternate spline formula. The native
viewer renders the prepared buffers.
The same header can be used directly in the future native game runtime.

For `t <= 1`, evaluation is `P0 + (P1-P0)*t`, tangent `P1-P0`. Negative values
extrapolate along that line. Above 1, `t` is capped at `point_count - 3`. Let
`i=floor(t)`, `u=t-i`, and `a,b,c,d` be points `i-1,i,i+1,i+2`:

```
P(u) = b + 0.5 * [(-a+c)*u + (2a-5b+4c-d)*u² + (-a+3b-3c+d)*u³]
```

This is uniform Catmull–Rom with tension 0.5. Tangents are analytic derivatives
in parameter units and are not normalized. Beyond the upper clamp the tangent
is still evaluated at the capped parameter, not returned as zero. The C ABI
requires four finite points and rejects non-finite inputs.

The linear-to-cubic join at 1 is position-continuous but not generically C1.
Fixtures begin with three equally spaced collinear points to match tangents
there. Interior cubic joins are C1. These endpoint conventions are preserved.
Float32 accumulation follows the original SSE order; contraction is disabled.

The original frame is `forward=normalize(tangent)`,
`right=normalize(cross(up_hint,forward))`, `up=cross(forward,right)`.
The caller normalizes its interpolated up hint first. The port does likewise.
Normalization returns `(0,0,1)` if all absolute components are at most float32
`1e-6`. Raw original behavior can therefore produce a degenerate frame for some
parallel inputs; tests preserve it, and scene validation rejects bad geometry.

## Geometry and placement checks

The original nine-vertex road cross-section, widths, UVs and winding are swept
through the native frames. Walls use those same frames. Shared row indices
avoid duplicated boundary seams. Control points and up guides are deliberately
authored test inputs, not recovered song analysis. The banked fixture reaches
35-degree guides; the connected course combines curves, slopes and up to
30-degree guides.

Manifests in `analysis/native-port/track-dance-alf-*` record float32-buffer
validation: width error, frame orthogonality/handedness, normal lengths, triangle
areas, winding, knot position-jump estimates, nearby tangent changes and spacing.
Across all five generated ALF fixtures, width error is below `0.000009` world
units and frame-up length error below `0.000001`. No road triangles are
degenerate or inverted.

The original assigns one up normal per row across the road. During bank changes
this shading normal differs from the actual triangle normal; measured deviation
is at most about 10.7 degrees. The report exposes this rather than replacing
the recovered rule or claiming exact geometric normals.

Each car receives the rigid sampled frame without scaling. The 32 lowest
authored vertices per wheel form a support patch. They are projected along the
car's up axis onto the actual float32 road triangles, and the car is raised to
leave approximately `0.002` world units under its lowest patch. All four patches
are checked for penetration. Remaining gaps are below `0.036` world units in
these fixtures. This is geometric inspection, not suspension, tire collision,
or physics settling; a rigid car cannot perfectly fit a twisting surface.

## Original-code parity and reproduction

`reriff-reference math` maps the supported local executable into a small
freestanding x86 Linux harness and calls only the three math entry points. No
Windows startup or game loop runs. Wrapper sources are in `reference/`; generated
original bytes and binaries remain in ignored `build/`. Unknown hashes are refused.

Only the imported `_libm_sse2_sqrt_precise` is replaced by SSE2 `sqrtsd` for
finite nonnegative values. Other spline/orientation bytes come from the original.
No proprietary executable bytes are distributed with these wrappers.

**1,200 comparisons matched bit-for-bit, with maximum absolute error zero.**
Cases cover randomized points/vectors, negative parameters, knots, clamping,
near-zero vectors and parallel guides. This proves parity for the tested inputs,
not every floating-point edge case or whole-game behavior.

```sh
build/native-linux/reriff-reference math --game-dir '/path/to/Riff Racer'
make -C NativeLinux test viewer
```

The native reference tool needs GNU binutils and an x86 Linux kernel permitting
32-bit system calls. Restricted environments may block the harness; use a normal
terminal in that case. Its report is `analysis/native-port/cpp-primitive-parity.json`.
Ordinary tests need neither original bytes nor 32-bit execution.

All five fixtures were rendered offscreen through Vulkan and inspected. The
initial low camera was corrected after it hid cars behind banked sidewalls.
Keyboard/pointer interaction still needs manual exercise. The original ramp is
kept out of spline fixtures until its separate placement behavior is ported;
the older straight inspection scene retains it.

Original bundled analyses now feed the ported node generator and these routines;
see [song tracks](SONG-TRACKS.md). The Rolling On route passes the independent original-node comparison.
Driving physics remains separate work.
