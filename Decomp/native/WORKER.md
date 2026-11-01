# Original message loop and worker requests

All addresses refer to the supported original Game.exe at image base 0x400000.
The source-built spans listed in STARTUP.md compare exactly; this is not a
complete executable or a runtime test. External callees and data remain at
their original addresses, and their presence is not implied by a successful
fragment comparison.

## Object initialization

`startup_objects.S` reconstructs four lazy allocation helpers called by the
application entry. They allocate 0x14, 0xBBC, 0xF4 and 0x44 bytes respectively.
The first initializes priority 1, zero handle/thread ID, vtable 0x9FE634 and a
pointer at offset 0x10. The middle two retain their original exception frames;
their constructors are external. Null allocations follow the original branches.

## Message pump and thread lifecycle

`message_pump.S` drains PeekMessageA with PM_REMOVE, calling TranslateMessage
then DispatchMessageA for each message. It retains the three-byte LEA padding.

`worker_thread.S` retains the original `_beginthreadex` call, object argument,
thread-ID output at offset 0xC and handle storage at offset 8. A nonzero boolean
argument selects flag 4 (create suspended). The code tests the resulting handle
against -1, not zero: reconstruction deliberately preserves this check.
Priority values 0, 1 and 2 select -1, 0 and +1 respectively; other values select
zero. The return from SetThreadPriority is ignored and success is returned.
The thread entry at 0x48DC50 invokes vtable slot 8 on the supplied object.
The startup object's slot points to 0x5282E0; that body is now reconstructed as
`application_worker.S`, described below. Its external callees remain partial.

Both destructor forms restore vtable 0x9FBE20 and call CloseHandle on offset 8.
The deleting form conditionally calls operator delete when argument bit 0 is set.

Import-table evidence (IAT VA):

| Address | Import |
| --- | --- |
| 0x92D55C | PeekMessageA |
| 0x92D57C | DispatchMessageA |
| 0x92D59C | TranslateMessage |
| 0x92D37C | _beginthreadex |
| 0x92D0D8 | SetThreadPriority |
| 0x92D0D4 | CloseHandle |
| 0x92D4B0 | operator delete (`??3@YAXPAX@Z`) |
| 0x92D4B4 | operator new (`??2@YAPAXI@Z`) |

## Request handoff

`worker_request.S`, VA 0x4A1870, first rejects requests when byte +0xBA0 is set.
Otherwise it waits for both events at +0x460/+0x464 with an infinite timeout,
stores the caller's request pointer at +0xBA4, clears result +0xBA8 and sets
byte +0x50. It resets the selected completion event, signals the selected work
event at +0x458/+0x45C, then waits for completion and returns byte +0xBA8 after
clearing +0x50. Selection reads the index at +0x488 each time, as in the original.
Wait return codes are not checked. The import slots are WaitForMultipleObjects
0x92D0C8, ResetEvent 0x92D104, SetEvent 0x92D120 and WaitForSingleObject 0x92D124.

Inspection of frame body 0x4A0AF0 establishes the counterpart: a pending +0xBA4
request is passed to 0x4A1900; its result is saved at +0xBA8, the request pointer
is cleared and completion is signaled. That frame body is not yet reconstructed.

## Request dispatcher

`request_dispatch.S` reconstructs VA 0x4A1900..0x4A1BB8: 642 executable bytes,
two bytes of MOV padding, and two symbolic address tables (32 + 20 bytes).
The first table dispatches request IDs 0 through 7; out-of-range unsigned IDs
return false. Case 6 sets +0xBA0 and returns true, explaining the worker's
shutdown request. Other cases retain all original calls, flag writes, register
arguments, return paths and the case-5 tail call. Their higher-level names are
not guessed. In particular, case 4's 16-iteration loop and case 5's five-entry
float-selection table retain the exact instruction sequence.

Case 1 calls `request_cleanup.S`, VA 0x4A1BC0..0x4A1C2F. It conditionally calls
vtable slot 8 for pointers at offsets 0x440, 0x43C, 0x448, 0x444, in that order,
and zeros each field. This is consistent with COM-style Release, but the helper
alone does not establish each pointer's interface type. It returns true.

The jump-table addresses are produced by labels and the linker, not copied
binary bytes. Exception dispatch, vtables, referenced constants and most
callee implementations are still unresolved reconstruction dependencies.

## Display requests and rollback

`request_case2.S` reproduces 0x4A1CD0..0x4A2128 (1,052 code bytes plus a
60-byte table). It compares six existing settings before changing state, saves
the presentation fields, maps values 2..16 through the original jump table,
and invokes vtable slot 0x40 on the object at +0x78. Its structure is consistent
with a Direct3D device reset, but the pointer's interface identity is not proven
by this function alone. A negative result restores the saved presentation
fields and calls the window-mode helper before returning false. A later failure
from 0x4A6560 triggers a second rollback, including restoring the previous
settings. Both distinct failure paths and cookie checks match exactly.

On success it applies the window mode, updates two scale ratios with the
original unsigned-integer conversion sequence, and calls the remaining helpers.
The apparently redundant 15-case mapping has not been simplified. Referenced
float constants and the cookie-failure handler remain external dependencies.

`request_case3.S`, 0x4A2130..0x4A226F, compares and updates the secondary display
settings, invokes 0x48CC70, calculates its two scale ratios and updates +0x4BB.
Both request paths retain the intermediate double-to-single rounding, division
and original constant-table addresses, rather than replacing them with a C++
cast that could generate a different sequence.

`window_mode.S`, 0x48CB30..0x48CC6B, selects style 0xCF0000 or 0x80000000,
calls SetWindowLongA and AdjustWindowRect, and preserves the original windowed
monitor-work-area adjustment and SetWindowPos flags. It saves the requested
mode byte at +0x28. Imports resolve to MonitorFromPoint (0x92D520),
GetMonitorInfoA (0x92D58C), SetWindowLongA (0x92D5AC), AdjustWindowRect
(0x92D580) and SetWindowPos (0x92D56C).

`secondary_window.S`, 0x48CC70..0x48CE0B, handles the second HWND at +8.
When a requested dimension is zero and that HWND exists, it destroys and clears
it. Otherwise it adjusts the requested rectangle and either creates a window
or updates its style/position. Creation keeps the original title/class pointers,
position arithmetic, callback helper 0x48CAB0 and ShowWindow argument 5.
The imports are DestroyWindow 0x92D530, CreateWindowExW 0x92D578, ShowWindow
0x92D574, and GetWindowRect 0x92D524. Window labels and classes are still external
data; no replacement title or behavior is introduced.

`cookie_check.S`, 0x8F8958..0x8F8967, compares ECX against the original cookie,
uses the original REP-prefixed return on success and jumps to 0x8F956B on failure.
The failure body is not reconstructed by this 15-byte span.

## Viewport sizing and related helpers

Ghidra exports are recorded under `analysis/ghidra/recovery`; see
`../GHIDRA.md` for the reproducible workflow and analysis limitations.

`window_frame.S`, 0x48CAB0..0x48CB2B, dynamically loads `dwmapi.dll`, looks up
`DwmGetWindowAttribute`, and requests attribute 9 for the secondary HWND. On
success, a positive difference between the window rectangle's top and the
returned frame top is added to the vertical placement offset. The DLL is freed
on both lookup failure and completion. Ghidra resolved the original strings
and imports; the machine instructions independently confirm the control flow.

`viewport_size.S`, 0x4A22E0..0x4A23B4, stores requested dimensions at +0x48C/490.
With fitting disabled it copies both dimensions, clears offsets, and computes
height / 10 - 2 using the original signed multiply/shift sequence. With fitting
enabled it passes pointers to both dimensions to 0x4A1C30, stores the adjusted
dimensions, computes both half-difference offsets, and updates the window
object's coordinate offsets. Logical right shifts are preserved exactly.

`fit_dimensions.S`, 0x4A1C30..0x4A1CCF, selects between reducing height and
reducing width using the original float arithmetic, floor wrapper, unsigned
comparison, and float-to-integer runtime helper. It preserves SSE conversions,
x87 return handling and the original rounding points. The caller's initial
Ghidra output incorrectly omitted the possible width change; the disassembly
and helper body resolve that ambiguity.

`floor_float.S`, 0x426D20..0x426D55, promotes a float to double, calls the original
floor routine at 0x8F8F40, rounds the result back through float storage, and
returns it in ST0. The CRT floor implementation remains an external dependency.

`remove_registered.S`, 0x4A23C0..0x4A2454, holds the critical section at +0xA84
while searching the pointer range at +0xA78/+0xA7C. It removes the first matching
pointer with memmove and decrements the end pointer by four, preserving both
not-found/empty return paths and the original lock/unlock calls. The identities
of registered objects remain unresolved.

## Original graphics settings file

Ghidra identifies the string at 0x9FC2E8 as `graphics.dat`.
`load_graphics.S` reproduces 0x4A2460..0x4A263A, including all 474 bytes of its
startup bypass, read/validation, application and failure paths. Holding both
Shift and Control (high bits of GetAsyncKeyState 0x10/0x11) bypasses loading and
returns false. The loader requests exactly 36 bytes and requires a successful
read with that reported length. Both dimensions must be at most 30,000 using
unsigned comparisons; this function does not reject zero dimensions.

| Record offset | Observed use |
| --- | --- |
| 0x00, 0x04 | Width and height passed to 0x4A22E0 |
| 0x08 | Bits 0, 1, 2 stored at object +0x4BF, +0x4BA, +0xA5D |
| 0x0C | Value stored at object +0x9A4 |
| 0x10 | Value stored at global object's +0x260 |
| 0x14 | Float passed in XMM1 to 0x528F80 with stack argument 1 |
| 0x18..0x23 | Twelve bytes individually expanded into fields +0xA2C..+0xA58 |

Higher-level labels for the last fields are not established by this routine.
On a bad read it closes via vtable slot 0xC, then invokes slot 0x14 with the
filename; on invalid dimensions it reaches that latter operation after closing.
Slot 0x14 is now confirmed as deletion through the original save-device vtable
and its call to DeleteFileW, as detailed below.
It restores the previous global file context on normal success and failure.
Ghidra's output did not express the XMM1 argument to 0x528F80; the reconstruction
retains it from the original instructions. No graphics settings file was edited
or removed while recovering this behavior.

`save_graphics.S`, 0x4A2640..0x4A27CA, reconstructs the corresponding 36-byte
write. Width/height come from presentation fields +0x1B8/+0x1BC. Flag bit 0 is
derived from +0x1D8 being zero; bits 1 and 2 come from +0x4BA/+0xA5D. The float
at record offset 0x14 comes from the resource object's +0x40. Twelve component
fields are packed with the original shifts and ORs, without extra masking.
File creation/open/write failure and successful close paths restore the file
context and preserve the original boolean result.

`resize_resources.S`, 0x528F80..0x5290AB, releases and clears resource pointers
in 20-byte entries, stores the new XMM1 setting at +0x40, computes a size through
0x8F8F50, then invokes device vtable slot 0x5C with equal width and height. The
argument layout is consistent with texture creation. On failure with fallback
enabled it recursively restores the old setting with fallback disabled, then
returns false. With fallback disabled a failed creation does not advance the
entry index: the original loop retries that entry. That behavior is retained,
not replaced with a timeout or a success assumption. Its nine padding bytes
are counted separately from executable behavior in the coverage breakdown.

## Save-device interface evidence

Ghidra's recovered RTTI labels identify `wsSaveDevice` and `elSaveDevicePC` in
constructor 0x535710. `save_device_ctor.S` preserves all three vtable writes,
initial flag/handle/string fields and the active-file-context assignment.
The primary vtable at 0x9FFE2C points slot 0xC to 0x535C20 and slot 0x14 to
0x535CC0. Vtable bytes themselves have not yet been counted as rebuilt data.

`save_device_close.S` calls fclose on a nonnull +0x10 handle, clears +0xC and
the handle, and returns true even if fclose reports failure. A null handle
returns true without those writes, exactly as the original does.

`save_device_delete.S` retains the original exception frame, filename/path
helper calls, short-string/heap selection, DeleteFileW call, heap cleanup and
boolean conversion of the Windows result. The path helpers and exception
handler remain external reconstruction dependencies. The resolved virtual
target confirms that the graphics loader's bad-read/dimension path deletes
the settings file; this reconstruction work only compares generated bytes and
does not execute that deletion.

`save_device_io.S` reconstructs read at 0x535D80..0x535DDC and write at
0x535DE0..0x535E26. Both seek from the start before I/O. Read stores the fread
byte count and returns whether ferror is clear, so a short EOF read can still
return true. A null handle or failed seek stores count zero and returns false.
Write returns true only if fwrite reports the full requested byte count. These
different success criteria explain the graphics loader's separate length check.

`save_device_open.S`, 0x535B10..0x535C1B, maps mode 1 to the original wide `rb`
string and modes 2/3 to `r+b`; other modes return false. It builds a path, checks
GetFileAttributesW, opens via _wfopen, stores the handle, and records the mode
only on success. The original does not close an existing handle in this method.
String destruction and SEH restoration retain their original order. Ghidra
inferred a void return, but the machine code explicitly returns a boolean in AL;
the reconstruction preserves that result. File creation is a separate vtable
method, now reconstructed below. Shared path helpers and SEH handlers remain
external dependencies.

`save_device_exists.S`, 0x535810..0x5358E4, tests existence by opening with wide
`rb`, closing a successful handle, then returning true. Thus an existing but
unreadable file can return false. The original does not test existence using
attributes in this method; shared filename conversion and path construction
retain their original calls and cleanup.

`save_device_create.S`, 0x5358F0..0x535B05, scans the wide path starting at index
2. At each slash or backslash it constructs the directory prefix, checks its
attributes and calls CreateDirectoryW if missing. Failure stops creation. It
opens the target with `wb`, zeroes a 4,096-byte stack buffer and writes the
requested size in chunks of at most 4,096 bytes. A short write closes the file
and returns false; a fully written file returns true after closing, even if
fclose itself reports an error. The zero-length case still creates/truncates
and closes the file. All branches, including the separate late failure block,
match exactly. These operations were reconstructed and byte-compared, not run
against user settings files.

`save_device_size.S`, 0x535C50..0x535CB3, returns zero for a null handle. Otherwise
it saves ftell, seeks to end, obtains the size and restores the original position.
Either failed seek returns zero. ftell failures are not independently checked.
This source was mechanically drafted, reviewed, assembled and matched exactly.

## Application worker body

`application_worker.S` reconstructs all 1,611 bytes in VA range
0x5282E0..0x52892B (exclusive end). Its source uses
instruction labels and external address symbols generated by the C++ drafter,
then reviewed against the Ghidra export and original instruction listing.
The span contains the complete normal body, including its shared return path;
exception handler 0x9052D4 and called implementations are not included.

The original starts by recording its thread ID, initializing COM and allocating
several singleton objects with the original SEH construction states. A startup
failure signals and waits for a worker, submits request ID 6 through 0x4A1870,
and returns -1. Other early-failure branches also reach that error return.

The command line is checked using substring searches for `-steamreset`,
`-modelviewer`, `.bdae`, `-scenelink`, `-mutesound` and `-nosound`. These strings
and their original branch order are retained. Steam remote storage and optional
stats reset calls are preserved. This does not change the working server DLL.

The update loop reads the shared quit flag, obtains the time step through a
virtual call, updates minimized/foreground state for both window handles,
dispatches input and game updates, ticks the fader/audio-related objects, calls
the original networkParseTick import, pumps Steam callbacks, then calls
0x528930. Register/SSE arguments are retained even where Ghidra inferred an
incomplete prototype. A return value from 0x492860 updates EDI for the next
iteration; it is not replaced with a constant. Shutdown signals the worker
event, releases a reference-counted object, invokes subsystem cleanup and calls
SteamAPI_Shutdown before returning zero through the shared SEH/cookie epilogue.

This establishes exact bytes for the worker's orchestration, not a complete
implementation of every subsystem it calls or a runnable whole-EXE rebuild.

## Worker queue processing

`worker_queue.S` reconstructs VA 0x528930..0x528AD1 (exclusive end):
417 bytes, including six-byte LEA padding at 0x528A2A and two-byte MOV
padding at 0x528A8E. The Ghidra function body excludes the former gap;
the original listing confirms it contains an alignment instruction, not data.

The ECX argument selects a 24-byte record in a lazily allocated three-record
array. Index zero first checks the virtual method at offset 0x168 and may
return immediately. The shared byte at 0xBFCBBB controls whether queued records
are dispatched or cleared. Enabled records are iterated in 40-byte steps;
the string at each record's start uses inline storage when capacity is below
16, otherwise a heap pointer. Each record calls 0x499120 with its original
arguments and the selected index. Cleanup releases heap strings, resets their
length/capacity, clears the record counters, and resets the queue end pointer.

The selector has no local bounds check and allocation failure is not handled
before accessing the array; these original behaviors are preserved. The
constructor/destructor helpers, dispatch callee and SEH handler 0x90530C remain
external and earn no byte credit from this reconstruction. The queue's
higher-level purpose is not yet established. Strict native-check and aggregate
coverage both pass for this compiled source.
