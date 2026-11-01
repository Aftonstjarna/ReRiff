# Original application frame and device recovery

These sources reconstruct original x86 instructions, not replacement rendering
logic. The original EXE SHA-256 is
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Ghidra exports in `analysis/ghidra/recovery/004a0af0.txt`, `004a6330.txt`
and `004a6560.txt` were checked against GNU instruction listings under
`build/decomp-container/draft-check/`. The C++ symbolic drafter produced the
assembly, which was reviewed, compiled and compared without masked bytes.

| Source | Original VA range (exclusive end) | Exact bytes |
| --- | --- | ---: |
| application_frame.S | 0x4A0AF0..0x4A1861 | 3,441 |
| device_release.S | 0x4A6330..0x4A6551 | 545 |
| device_restore.S | 0x4A6560..0x4A669F | 319 |
| query_create.S | 0x4A4C50..0x4A4D19 | 201 |
| query_release.S | 0x4A4D20..0x4A4D73 | 83 |
| colour_targets.S | 0x4A5240..0x4A5530 | 752 |
| effect_target.S | 0x4A5530..0x4A56BD | 397 |
| aux_target_a.S | 0x4A56C0..0x4A5849 | 393 |
| aux_target_b.S | 0x4A5850..0x4A59D9 | 393 |
| device_defaults.S | 0x4A5E80..0x4A632A | 1,194 |
| buffer_allocate.S | 0x5429A0..0x542A3B | 155 |
| device_create.S | 0x4A3630..0x4A3A33 | 1,027 |
| device_destroy.S | 0x4A5C40..0x4A5E76 | 566 |
| presentation_parameters.S | 0x4A33F0..0x4A362F | 575 |
| vertex_declarations.S | 0x4A3A40..0x4A4C43 | 4,611 |
| graphics_startup.S | 0x4A0480..0x4A08D6 | 1,110 |
| graphics_shutdown.S | 0x4A08E0..0x4A0AE1 | 513 |
| graphics_capabilities.S | 0x4A2BE0..0x4A33EF | 2,063 |
| graphics_options.S | 0x4A27D0..0x4A29C6 | 502 |
| minimum_caps.S | 0x4A59E0..0x4A5AF9 | 281 |
| display_modes.S | 0x4A29D0..0x4A2BD8 | 520 |

The frame has 12 instruction-padding bytes: LEA at 0x4A0D5D (3), LEA at
0x4A1007 (7), and MOV EDI,EDI at 0x4A100E (2). The latter nine bytes occupy
a gap in Ghidra's function body. There are no inline jump tables in these spans.
The sources retain calling conventions, SSE/x87 operations, absolute addresses,
SEH setup and stack-cookie operations exactly. External implementations and
referenced data are not included in their byte credit.

## Frame handoff

The frame routine returns immediately if object byte +0xBA0 is set. Otherwise
it waits indefinitely on the event selected by index +0x488 from +0x458.
A pending request at +0xBA4 is sent to the already recovered 0x4A1900
dispatcher; its AL result is stored at +0xBA8, the request is cleared, and
the corresponding +0x460 event is signalled without advancing the buffer index.

The ordinary frame path reads the selected work buffer from +0x468. A skip
flag at buffer +0x91 bypasses rendering but still marks completion, advances
the two-buffer index, and signals the original completion event. Changes to
buffer +0x92 invoke the recovered viewport routine 0x4A22E0 with the original
dimensions. Counters and timing statistics are updated before the device check.

The device virtual call at slot +0x0C is compared to 0x88760868 and 0x88760869.
On the former status, or the gated foreground-window mismatch, resources are
released when object byte +0xA74 is clear and the thread sleeps 50 ms. On the
latter status, the same release precedes the device +0x40 call. A negative
result completes the frame without rendering; otherwise restoration at
0x4A6560 is called. **The frame does not test restoration's AL result.** This
behavior is preserved rather than adding an invented failure path.

Rendering retains the original one/two-pass selection, viewport and target
calls, work-buffer iteration, query operations and presentation calls. Its
timing loop retains QueryPerformanceCounter, ceil/fmod helper calls and
conditional Sleep(1), including the original floating-point operation order.
The frame ends by setting the selected completion byte, advancing +0x488
modulo two and signalling the event for the completed buffer. None of this
establishes that the external rendering callees are fully recovered.

## Resource release and restoration

Both helpers bypass their normal work when object DWORD +0x7C is nonzero;
restoration returns true in that case. Otherwise they use the critical section
at +0xA84 and install the original SEH handler 0x902B88. That handler is an
unrecovered dependency; normal-path lock release alone does not establish
complete exception behavior.

Release calls 0x4ADD20, locks, visits registered objects at +0xA78..+0xA7C
through virtual slot +4, then invokes the global +4 callback and 0x4A4D20.
It releases and nulls the original COM-style resource pointers, clears the
associated counters, sets byte +0xA74 and unlocks. Pointer release order is
retained, including the two-entry interleaved loop at +0x420/+0x428.

Restoration locks, invokes the global +8 callback, then registered objects'
+8 callbacks, stopping on the first false result. It calls 0x4A4C50 followed
by the checked sequence 0x4A5240, 0x4A5530, 0x4A56C0 and 0x4A5850.
The final checked allocation call at 0x5429A0 receives ECX = object +0x214,
the device pointer, bit zero of byte +0x9E, and sizes 0x1000000/0x60000.
Only success clears +0xA74 and invokes 0x4A5E80. All ordinary failure paths
unlock and return false; no additional rollback has been introduced.

## Query resources

The additional Ghidra exports `004a4c50.txt` and `004a4d20.txt` were compared
to the original listings before promoting these sources. Query creation uses
device virtual slot +0x1D8 to request types 10, 10 and 12 for each of two
three-pointer groups beginning at object +0x470. Any negative result releases
all six pointers: failure on the first call uses an inline cleanup loop, while
failure on the second or third calls 0x4A4D20. Those distinct original paths
are retained. No explicit boolean result is synthesized for this routine.

Query release visits each group in reverse pointer order (+8, +4, +0), calls
virtual slot +8 for non-null pointers, and clears every slot. It contains a
three-byte LEA alignment instruction at 0x4A4D2D. The frame's existing query
issue/result operations use these same groups; the device implementation itself
remains external.

## Target creation

All four target helpers return a boolean in AL, despite Ghidra's inferred
`void` prototypes. Original MOV AL,1 and XOR AL,AL paths, SEH construction
states and string cleanup are retained. The matching cookie-check helper
does not alter the success-path AL result. No padding or inline tables occur
within these four ranges.

Colour-target creation uses dimensions +0x23C/+0x240 and format +0x1F8
to create two textures into +0x420/+0x424, obtaining their level-zero surfaces
into +0x428/+0x42C. It then creates the depth surface at +0x430 using format
+0x1E0. Each negative HRESULT records the original error text via the game's
string helpers and returns false. The function does not locally release
previously created textures on a later failure; that behavior is preserved.

The other helpers return true without allocation if their enable byte is zero:

| Helper | Enable byte | Texture/surface fields | Format |
| --- | --- | --- | --- |
| Effect | +0x4BC | +0x434/+0x438 | DWORD +0x1FC |
| Auxiliary A | +0xA5E | +0x43C/+0x440 | 0x15 |
| Auxiliary B | +0xA5F | +0x444/+0x448 | 0x15 |

Enabled helpers use the same dimensions, one level, usage 1, pool 0 and
level-zero surface query. The originals share error-message strings, but their
separate routines and differing addresses/fields are preserved. Original
strings, allocation imports, error formatting helpers and SEH handlers remain
external dependencies and gain no credit from these code fragments.

## State defaults

`device_defaults.S` preserves the original immediate assignments, including
float bit patterns and sentinel values, rather than substituting generic API
defaults. It has no external calls. Its 16-iteration loop initializes eight
DWORD arrays starting at +0x824, +0x864, +0x8A4, +0x8E4, +0x924, +0x964,
+0x9AC and +0x9EC with respective values 1, 1, 0, 1, 1, 1, 0 and 0.
The one-byte NOP at 0x4A62BF is counted as instruction padding. The final
writes clear +0xA68/+0xA6C/+0xA70 and set +0xA64 to 0xFFFFFFFF.
The frame's state-rebinding branches consume these fields. Specific semantic
names for all cached state fields are still unresolved; their offsets and
values are preserved exactly. Ghidra export: `004a5e80.txt`.

## Scratch allocation and device lifetime

The scratch allocator returns HRESULT in EAX and uses RET 16. It chooses
usage 0x248 for a nonzero flag, otherwise 0x228, calls device slot +0x68
for the first buffer, and +0x6C for the second with four times the requested
element count and format 0x66. Failure of the first call returns immediately.
Failure of the second releases and clears the first buffer and returns the
saved failure HRESULT. Success stores and AddRefs the device, records both
requested sizes, zeroes the two cursors and returns zero. Integer shifting,
failure order and original arguments are preserved.

Device creation uses the primary factory's +0x40 slot when +0x74 is null;
otherwise it uses that alternate factory's +0x50 slot with the original
optional display descriptor. The alternate success path stores the resulting
device in both +0x7C and +0x78 and adds the second reference. On creation
failure it saves 56 bytes of presentation parameters, clears byte +0x4BF,
requests 1280x720 through 0x4A22E0, and calls 0x4A33F0. It retries recursively
only when the newly computed parameters differ; otherwise it records the
original error and tears down. The six-byte LEA at 0x4A378A is padding.

After device creation the routine creates colour/effect targets, the scratch
buffers and resources through 0x4A3A40. It calls the original D3DX import for
three embedded images at VA 0xBAAF88 (21 bytes), 0xBAAFA0 (6,208 bytes) and
0xBAACC8 (704 bytes), then creates query resources and resets cached state.
These asset bytes are referenced, not reconstructed or counted. The original
boolean return in AL, SEH states and failure cleanup branches are retained;
Ghidra's inferred `void` return is not used as the source contract.

Teardown calls query release, releases the three embedded-image textures,
visits 33 pointers beginning at +0x298, releases scratch buffers and their
device reference, releases colour/effect/auxiliary targets, then releases
the two device and two factory references. Every visited pointer is cleared.
The four-byte LEA at 0x4A5D3C is padding. There are no new substitute cleanup
steps or changes to reference ownership. Research exports for these routines
are `005429a0.txt`, `004a3630.txt` and `004a5c40.txt`.

## Presentation parameters and declarations

The parameter builder walks 24-byte mode records at +0x27C..+0x280.
With byte +0x4BF clear, its maxima scan updates height only when the record's
width is at least the current width maximum. It is not two independent maxima.
With the byte set, exact requested dimensions win; otherwise it tracks an
upper candidate whose two dimensions are no larger than the previous upper
candidate, and a lower candidate whose two dimensions are no smaller than the
previous lower candidate. It chooses the upper candidate if present, otherwise
the lower. The original unsigned comparisons, iteration order and empty-list
behavior are preserved; no nearest-area heuristic replaces them.

The selected dimensions are propagated to +0x23C/+0x240, +0x244/+0x248 and
+0x1B8/+0x1BC, with the original calls to the viewport helper. The windowed
field +0x1D8 is set from the inverse of +0x4BF and +0x1E8 is cleared. A
three-byte LEA at 0x4A345D accounts for this routine's instruction padding.

The declaration routine builds 33 original descriptor arrays on its stack,
sets the corresponding stride fields beginning at +0x31C, and submits them
in the original order through device slot +0x158. Result pointers occupy
+0x298..+0x318. Every packed descriptor constant and sentinel is present in
the mnemonic source; no descriptor bytes are copied from the input binary.
Failure records the original error, frees temporary strings, calls the recovered
device teardown and returns false in AL. Success returns true. Ghidra inferred
`void`; the instruction listing establishes the actual result. Semantic names
for each of the 33 layouts still need mapping to their mesh users.

## Graphics startup and shutdown

Startup creates two pairs of events: each work event is auto-reset and initially
unsignalled, while each completion event is manual-reset and initially signalled.
It preserves the original lazy singleton allocations and SEH construction states.
After capability setup it initializes the viewport, loads graphics settings,
processes options, builds presentation parameters, creates the window/device,
retains another device reference and records the creating thread ID. Shader
setup failure releases that reference, destroys the window and device, and
reports the original error. Success initializes further state, records the
performance-counter low DWORD and clears byte +0x50. The return is boolean AL
with RET 12; external constructor and shader implementations remain unresolved.

Shutdown sets +0x50, destroys the original singleton objects in order, clears
resource collections, releases the additional device reference and calls device
and window teardown. It closes and zeroes both pairs of events and clears
+0xBA0. Its unusual lazy allocation of two empty collection objects during
cleanup is preserved. External helpers are not replaced with no-ops. Neither
startup nor shutdown contains inline tables or instruction padding.

The new research exports are `004a33f0.txt`, `004a3a40.txt`, `004a0480.txt`
and `004a08e0.txt`; corresponding original instruction listings are retained
under `build/decomp-container/draft-check/`.

## Capabilities, modes and command-line options

Capability detection preserves Direct3DCreate9 SDK argument 0x20, adapter zero,
device type 1, and the ordered display, intensity, intensity+alpha, ARGB and
depth-format probes. Depth selection checks both format support and compatibility
with the chosen display format. The original candidate constants referenced
through SIMD loads remain external data, not credited reconstructed bytes.
Failures preserve error strings and the original factory Release/nulling path.
Success returns true in AL, despite Ghidra's inferred `void` prototype.

The minimum-capability helper sets an output boolean when vertex-shader version
is below 0xFFFE0300. That selects software processing later rather than causing
immediate failure. It rejects pixel-shader versions below 0xFFFF0300, the original
power-of-two restriction combination, square-only textures, dimensions below
4096x4096, or fewer than three simultaneous render targets. The caller selects
behavior flags 0x20, 0x40 or 0x50 using the exact capability bits and output flag.
No requirements have been relaxed for the native port.

Display-mode recovery retains fifteen multisample probes and all four checks
per candidate (two formats, windowed and fullscreen), including original ROL
instructions used to construct the supported mask at +0x288. It enumerates
format 0x16, reserves the original vector, and appends successful results as
24-byte records. Empty results return false and record the original error;
successful enumeration returns true. The original oversized-vector exception
and vector helper calls remain dependencies, not replacement implementations.

Options are substring searches, not token parsing. `-fullscreen` first sets
+0x4BF. `-windowpos` copies the suffix, removes one leading character when
nonempty, finds a comma and the following space, and passes the two resulting
substrings to atoi for +0x20C/+0x210. The final `-window` search clears +0x4BF
and also matches `-windowpos`. This precedence and permissive parsing are
preserved, including negative coordinates and atoi's original behavior.

Capability detection contains 24 instruction-padding bytes at 0x4A2DDF (1),
0x4A2E6A (6), 0x4A2EF9 (7), 0x4A2F8A (6) and 0x4A318C (4).
Mode enumeration contains 15 at 0x4A2A29 (7), 0x4A2A4A (6), and 0x4A2B3E (2).
The other two routines have no identified instruction padding. Research exports
are `004a2be0.txt`, `004a27d0.txt`, `004a59e0.txt` and `004a29d0.txt`.

## Verification and remaining work

Run inside the development container:

```sh
make -C Decomp OUT=../build/decomp-container coverage test-draft -j2
```

All 64 registered fragments pass: 30,522 / 8,635,392 bytes (0.35345240%).
These are source-built fragment comparisons, not a linked Windows EXE.
Shader setup, vector/string helpers, callbacks, SEH handlers, imports,
PE layout, metadata and other executable regions still require reconstruction.
