# Original application entry

`application_entry.S` reconstructs the complete native block at VA
`0x005CFC90..0x005D002A` (RVA `0x1CFC90`, 922 bytes). Its three return sites
end with `ret 0x10`; the following bytes are `int3` padding before another
function at `0x005D0030`. All local branch targets stay within the recovered
span. External calls retain original addresses.

The assembled/linked code matches all 922 original bytes with no masking.
Generated block SHA-256:
`e7d8feda0a875bb65344826b2c4af0857bfe0eecb4593cccf32cd8c500065b8b`.
The test target is `make -C Decomp OUT=../build/decomp-container test-application`,
run inside the `development` Distrobox. This is a source-generated code fragment,
not a complete executable or a running replacement game.

## Behavior established directly from instructions and imports

- Creates the named mutex `OCTANE`, requesting initial ownership. If
  `GetLastError` returns `0xB7`, closes the returned handle and returns `-2`.
  The original code's handling of other mutex errors is preserved unchanged.
- Queries performance-counter frequency. Failure displays the original error
  dialog and returns `-1`; success converts and scales the frequency through
  original helper/data addresses.
- Stores the current thread ID, calls `CoInitializeEx(0,0)`, and executes a
  one-time guarded initialization call. This block does not branch on COM's
  return value; the reconstruction does not introduce a new check.
- Constructs the command-line string and invokes a transformation helper with
  the imported `tolower` function. Original short-string storage decisions,
  allocation/deallocation calls and cleanup branches are retained.
- Performs further startup calls and status checks. One failure path builds
  text, calls `MessageBoxW`, frees temporary strings and returns `-1`. The
  internal purpose of unresolved helper `0x00560310` is not asserted here.
- Uses `SetDllDirectoryW`, invokes additional startup helpers and virtual calls,
  then enters the loop at `0x005CFF90` while the original stop flag is clear.
  It calls `0x0048CE10`, tests the stop flag again, polls the handle at object
  offset `+8` with a zero wait, then calls `0x004A0AF0` before repeating.
- On shutdown, sets the byte at another object offset `+0xBA0`, signals its two
  events at `+0x460/+0x464`, waits on the first object's `+8` handle indefinitely,
  calls `0x004A08E0`, releases the command string and returns zero.
- Preserves stack alignment, cookie checks on each return, and the original
  seven-byte `lea esp,[esp+0]` loop-alignment instruction plus shutdown-loop NOP.

## Unresolved dependencies

The `fn_ADDRESS` symbols deliberately keep helpers unnamed until their behavior
and identities are verified. None of the selected startup helper addresses
resolved directly to names in the current CLR MethodDef inventory. In particular,
the implementations behind the main-loop calls and virtual methods are not
claimed as recovered by reproducing this caller.

String/data pointers, globals, imports, runtime helpers and the larger initializer
graph must still be reconstructed in a full PE build. The working server-support
DLLs are outside this EXE work. The Linux viewer remains separate and unchanged.
