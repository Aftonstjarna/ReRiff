# Bundled-song analysis and track generation

The Linux viewer can now generate a complete road from a bundled song's original
cached analysis. The reference example is **Rolling On**, profile ID
`17SJWbBsrv` (601 beats, 635 nodes including the original lead-in/out padding).

```sh
python3 scripts/view-vehicle.py --list-songs
python3 scripts/view-vehicle.py --song 'Rolling On'
python3 scripts/view-vehicle.py --song 'Rolling On' --overview
```

Home shows the entire road; 1–4 focus four inspection vehicles. Orbit/zoom/pan
and the existing camera controls work. The default generation genre is `dance`,
matching this bundled example. `--genre rock` and `--genre hiphop` are explicit
overrides; comparison requires the **same generation genre** in both programs.
A song's genre and a user's saved visual theme mapping are separate original
settings. Other bundled songs can be inspected with the supported genre overrides;
this does not establish parity with their original default genres/themes.

No MP3 decoding or new signal analysis is performed. These are the original
bundled analyses, rather than synthesized beat sequences. Audio playback, driving,
original ramp models at jump gaps, props, and animated shaders remain separate work.
Asset preparation uses Python on Linux; evaluation and orientation use the existing
native C++ library, and display uses the SDL3/Vulkan viewer. Windows is not needed
to generate or view the track.

## Packed song format

The installed `audio/music/bundled/*.json` files are binary despite their suffix.
The decoder also accepts this packed format from a cache; no independent generated
node cache was found or used. There are 25 installed bundled analyses.

| Offset | Type | Meaning |
| --- | --- | --- |
| 0 | u32 LE | `0x534e4730`, bytes `30 47 4e 53` (`0GNS`) |
| 4 | u32 LE | compressed profile byte count |
| 8 | u32 LE | compressed analysis byte count |
| 12 | u32 LE | expanded profile byte count, including terminal NUL |
| 16 | u32 LE | expanded analysis byte count, including terminal NUL |
| 20 | bytes | zlib profile stream, then zlib analysis stream |

Both payloads are JSON objects terminated by exactly one NUL. The profile has a
`song` object with its case-sensitive ID, title, artist and summary timing. Analysis
has `track`, `bars`, `beats`, `beatVols` and `sections`. Beats are seconds; beat
volumes are normalized; sections have start, duration, loudness and a beat index.
The decoder validates lengths, zlib stream termination, expansion bounds, JSON
numbers, beat ordering, matching loudness counts, and section ranges.

`bundled-songs.json` transcribes the 25 entries plus timing overrides from the
original executable's `gaSaveGame::BUNDLED_TRACKS` table (26 records including a
zero sentinel, 36 bytes each). The MP3 filenames and table artist spelling can
differ from profile JSON. The generator seeds from the **profile ID**, preserving
its case, rather than the lowercase filename. Bundled audible-start/first-beat/
mid-beat overrides are retained in the output metadata. They affect music/vehicle
time synchronization; generation uses the analysis beat timestamps and tempo.

## Recovered pipeline

`SongGenerator` reads original `segments_PC.cfg` and `sections_PC.cfg` from the
installed `vfs.zip` using the checked GFCF decoder. Config values that are two-item
arrays interpolate between 100 and 180 BPM and clamp outside that range.

1. Seed both original MT19937 streams with `0x12345678 + sum(signedChar <<
   (index/2+1))`, preserving x86 uint overflow and shift masking.
2. Assign gameplay section types with weighted choices from original section
   configuration, next-section loudness, and current-type exclusions.
3. Allocate `beatCount+34` nodes; start is node 10, finish is `beatCount+9`.
4. Interpolate speed from 160 to 200 mph, convert with the original `0.4470399916`
   multiplier, and derive nominal beat spacing. Each generated span instead uses
   its actual average beat interval times speed where beat data exists.
5. Divide at song-section boundaries and up to six detected loudness drops.
   Select original weighted segment entries using RMS loudness raised to 0.8,
   exclusions, checkpoints, forced followers and minimum/maximum node counts.
6. Apply straight, random twist, XYZ rotation, bump, chicane, jump, step/drop,
   and mega-jump transforms. Accumulate original matrices, apply up relaxation,
   set gap/visibility flags, reject close earlier nodes, and retry/backtrack with
   the original counters and reduced separation threshold when necessary.
7. Apply loudness-driven lane-width and wall-height modulation.
8. Feed world positions and up guides into recovered Catmull–Rom and orientation
   routines. Sample at float32 `row*3/20`, use original row visibility rules,
   width remapping and road/wall cross sections, and retain intentional gaps.

The port preserves unusual details, including inclusive modulo integer ranges,
float32 random scaling (which can round to one), RNG consumption with equal range
endpoints, after-span retry exclusions, and the original relaxation cross-product
order. It intentionally does not replace these with Python's random library or
standard spline routines. Host sine/cosine replaces the original Windows math
library; generation has not been certified bit-for-bit equivalent as a whole.

Primary methods are recorded in the local decompilation, not included here as
proprietary code:

| Method | Metadata token | RVA |
| --- | --- | --- |
| gaTrackGen.initFromJson | 0x060008cd | 0x255c38 |
| gaTrackGen.makeTrack | 0x060008cf | 0x255220 |
| gaTrackGen.subdivideSection | 0x060008d5 | 0x2539c8 |
| gaTrackGen.genSegment | 0x060008d7 | 0x2504f8 |
| gaTrackGen.applyNodeGeomStuff | 0x060008d2 | 0x24eac8 |
| gaTrackGen.calcVol | 0x060008d6 | 0x24e5fc |
| gaSegmentManager.applySegment | 0x06000c70 | 0x294cf4 |
| gaAudioAnalysis.detectDrops | 0x06000ec7 | 0x2c1fac |
| gaAudioAnalysis.detectDrops | 0x06000ec8 | 0x2c1638 |
| gaTrackStrip.makeSplines | 0x060009ee | 0x26d760 |

The authoritative address inventory is generated locally in
`analysis/native-port/methods.json` for Game.exe SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.

## Outputs and validation status

The scene directory `analysis/native-port/song-17sjwbbsrv-dance-alf/` contains
`scene.rmesh`, original textures, decoded profile/analysis, `song-nodes.json`,
`scene-manifest.json` and `song-validation.json`. All are local ignored artifacts;
original audio and game assets are neither copied into tracked source nor modified.

Geometry checks measure row width against the sampled width spline, normal length,
frame orthogonality, triangle degeneracy/inversion, and continuity at every knot.
The continuity estimate compensates for float32 parameter rounding at large node
indices. Intentional jump gaps are reported rather than joined. Four rigid vehicle
poses use contact patches projected onto the actual exported triangles; this is
not suspension or collision simulation.

Run the normal checks:

```sh
make -C NativeLinux test viewer
```

For direct original-executable math comparisons:

```sh
python3 scripts/verify-song-math.py --game-dir '/path/to/Riff Racer' \
  --nodes analysis/native-port/song-17sjwbbsrv-dance-alf/song-nodes.json
```

This isolates original x86 functions without Windows startup. It compares 32,000
random results (five seeds, four APIs, across state refreshes), 100 matrix products,
and 200 evaluations/tangents at points along the generated song. All tested values
matched bitwise. The existing `verify-track-math.py` separately tests orientation,
spline endpoints, extrapolation and degenerate vectors. A host must permit 32-bit
Linux syscalls for these reference executables.

**Rolling On now passes a full original-node comparison.** The offline Microsoft
.NET 4.5.2 host executed original `gaTrackGen.initFromJson` and captured all 635
nodes and 132 segments. Segment boundaries, types and float32
parameters match exactly. Widths, wall heights, visibility and flags match exactly.
Maximum Euclidean position error is 0.0079461 world units (tolerance 0.01);
maximum up-vector error is 0.000002908. Positions are not bitwise identical.

This caught and corrected a finish-padding bug: the original subdivides nodes
610–634 using the intro section configuration, then forces only the final
subsegment straight. Making the whole padding straight changed both the tail and
subsequent width-modulation RNG state.

Reproduce with the existing isolated, normal 64-bit Microsoft .NET prefix:

```sh
python3 scripts/capture-original-song.py --output analysis/native-port/original-rolling-on
python3 scripts/view-vehicle.py --song 'Rolling On' \
  --reference-nodes analysis/native-port/original-rolling-on/original-nodes.json
```

The capture host uses a disposable DLL-form copy with three entry/header changes;
original generation method bodies and the installed game remain unchanged. A
read-only file bridge supplies original serialized configuration bytes, and the
original JSON parser receives the bundled analysis/profile. No server, game loop
or renderer is needed. Hashes identify the executable, configuration, analysis and
reference. Raw vectors, segment records, logs and comparison results remain in
ignored `analysis/`. See [the runtime investigation](DOTNET452-INVESTIGATION.md).
This verifies this song and genre in the diagnostic host, not original rendered
vertex buffers, physics or complete gameplay. Other song/genre combinations still
need independent captures. Without `--reference-nodes`, the viewer correctly marks
that run as lacking a full-layout comparison.

## Comparing an independent original capture

Capture after `gaTrackGen.initFromJson` (which calls `makeTrack`) has returned,
before playback changes game state. `gaNodeManager+4` and `+8` point to the vector
begin/end. Each node is 244 bytes. Relevant offsets are:

| Offset | Field |
| --- | --- |
| 0 | node ID (i32) |
| 148 | world up (3 float32) |
| 172 | world translation / spline position (3 float32) |
| 208, 212 | left/right width factors (float32) |
| 216, 220 | left/right wall-height factors (float32) |
| 228 | road visibility (byte) |
| 240 | flags (u32) |

Save an independent JSON reference with `source: "original-game"`,
`executable_sha256` equal to the hash above, `song_id: "17SJWbBsrv"`,
`genre: "dance"`, and `nodes` containing `position`, `up`, `widths`, `walls`,
`visible` and `flags` for each original node. Do not relabel a Linux export as an
original capture. Then run:

```sh
python3 scripts/compare-song-track.py \
  analysis/native-port/song-17sjwbbsrv-dance-alf/song-nodes.json \
  /path/to/independent-original-nodes.json --output /tmp/song-comparison.json
```

The comparison rejects mismatched song/genre/executable metadata, reports numeric
errors and flag/visibility mismatches, and exits unsuccessfully on differences.
It does not mark the scene as original-layout validated without that reference.
