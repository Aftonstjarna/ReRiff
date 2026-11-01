# Original EXE startup recovery

Target SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
All addresses below refer to its preferred image base `0x00400000`.

| Block | RVA | Bytes | Rebuilt comparison |
| --- | --- | ---: | --- |
| PE entry jump | `0x4FD6D0` | 6 | exact |
| `_WinMainCRTStartup` wrapper | `0x4F8A84` | 10 | exact |
| Security-cookie initializer | `0x4F9B78` | 156 | exact |
| CRT common startup, filter and handler | `0x4F8A8E` | 436 | exact |
| SEH frame prologue and epilogue | `0x4F9C90` | 89 | exact |
| CRT exception scope table (data) | `0x79E5C0` | 28 | exact |
| Application entry | `0x1CFC90` | 922 | exact |
| Startup object at `0xBFCCFC` | `0x1CFB10` | 72 | exact |
| Startup object at `0xBFCC44` | `0x1CFB60` | 121 | exact |
| Startup object at `0xBFCD2C` | `0x1CFBE0` | 121 | exact |
| Startup object at `0xBFCD08` | `0x1CFC60` | 45 | exact |
| Message pump | `0x08CE10` | 85 | exact |
| Worker deleting destructor | `0x08DB80` | 44 | exact |
| Worker destructor | `0x08DBB0` | 16 | exact |
| Worker start | `0x08DBC0` | 130 | exact |
| Worker virtual dispatch | `0x08DC50` | 15 | exact |
| Synchronous worker request | `0x0A1870` | 135 | exact |
| Request dispatcher, padding and tables | `0x0A1900` | 696 | exact |
| Request resource cleanup | `0x0A1BC0` | 111 | exact |
| Request case 2 and table | `0x0A1CD0` | 1,112 | exact |
| Request case 3 | `0x0A2130` | 319 | exact |
| Main window mode/position | `0x08CB30` | 315 | exact |
| Security-cookie verification | `0x4F8958` | 15 | exact |
| Secondary window lifecycle | `0x08CC70` | 411 | exact |
| Secondary window frame adjustment | `0x08CAB0` | 123 | exact |
| Viewport dimensions and centering | `0x0A22E0` | 212 | exact |
| Fit viewport dimensions | `0x0A1C30` | 159 | exact |
| Float floor wrapper | `0x026D20` | 53 | exact |
| Synchronized pointer removal | `0x0A23C0` | 148 | exact |
| Original graphics settings loader | `0x0A2460` | 474 | exact |
| Graphics resource resize and fallback | `0x128F80` | 299 | exact |
| Original graphics settings saver | `0x0A2640` | 394 | exact |
| Save-device constructor | `0x135710` | 81 | exact |
| Save-device close | `0x135C20` | 38 | exact |
| Save-device delete | `0x135CC0` | 189 | exact |
| Save-device read | `0x135D80` | 92 | exact |
| Save-device write | `0x135DE0` | 70 | exact |
| Save-device open | `0x135B10` | 267 | exact |
| Save-device existence check | `0x135810` | 212 | exact |
| Save-device create | `0x1358F0` | 533 | exact |
| Save-device size query | `0x135C50` | 99 | exact |
| Application worker body | `0x1282E0` | 1,611 | exact |
| Worker queue processing | `0x128930` | 417 | exact |
| Application frame | `0xA0AF0` | 3,441 | exact |
| Device resource release | `0xA6330` | 545 | exact |
| Device resource restoration | `0xA6560` | 319 | exact |
| Query resource creation | `0xA4C50` | 201 | exact |
| Query resource release | `0xA4D20` | 83 | exact |
| Colour/depth targets | `0xA5240` | 752 | exact |
| Effect target | `0xA5530` | 397 | exact |
| Auxiliary target A | `0xA56C0` | 393 | exact |
| Auxiliary target B | `0xA5850` | 393 | exact |
| Device state defaults | `0xA5E80` | 1,194 | exact |
| Scratch buffer allocation | `0x1429A0` | 155 | exact |
| Device creation | `0xA3630` | 1,027 | exact |
| Device teardown | `0xA5C40` | 566 | exact |
| Presentation parameters | `0xA33F0` | 575 | exact |
| Vertex declarations | `0xA3A40` | 4,611 | exact |
| Graphics startup | `0xA0480` | 1,110 | exact |
| Graphics shutdown | `0xA08E0` | 513 | exact |
| Graphics capabilities | `0xA2BE0` | 2,063 | exact |
| Graphics options | `0xA27D0` | 502 | exact |
| Minimum capabilities | `0xA59E0` | 281 | exact |
| Display modes | `0xA29D0` | 520 | exact |

These 30,522 bytes comprise 30,295 bytes of code, 87 bytes of instruction padding,
28 bytes of exception data and 112 bytes of jump tables. They are reconstructed
from assembly mnemonics, symbols and data fields, with no
binary inclusion or literal opcode byte arrays. The ELF objects are build
intermediates only. Comparing their generated `.text` bytes does not establish
a Windows executable rebuild, correct PE relocations, or complete CRT behavior.

See [frame and device recovery](FRAME.md) for the frame handoff, device-loss
branches, query allocation and outstanding dependencies.

## Startup path

The OS-visible PE entry jumps through the `mscoree!_CorExeMain` import at VA
`0x0092D690`. CLR metadata identifies token `0x0600161B`, named
`_WinMainCRTStartup`, at RVA `0x4F8A84`. Its wrapper calls the cookie initializer
at VA `0x008F9B78`, then tail-jumps to CRT startup at VA `0x008F8A8E`.
The original wrapper has a five-byte near jump with zero displacement; keeping
the target external during assembly and resolving it at link time preserves
that encoding rather than shortening the jump.

The cookie initializer ends at the `ret` at VA `0x008F9C13`; its branches stay
within the recovered 156-byte span and its external calls are indirect imports.
It reads the cookie at VA `0x00BAABC0`. If the cookie differs from `0xBB40E64E`
and has a nonzero high word, it preserves it and refreshes its complement at
`0x00BAABC4`. Otherwise it XORs file-time halves, thread/process IDs, performance
counter halves and a stack address. It replaces the default-cookie collision
with `0xBB40E64F`, ensures a nonzero high word using `0x4711`, and stores the
cookie and bitwise complement.

Import-table evidence resolves the four calls:

| IAT VA | Imported function |
| --- | --- |
| `0x0092D040` | `GetSystemTimeAsFileTime` |
| `0x0092D0EC` | `GetCurrentThreadId` |
| `0x0092D044` | `GetCurrentProcessId` |
| `0x0092D0F4` | `QueryPerformanceCounter` |

## Reproduce in the development container

```sh
distrobox enter development -- bash -lc \
  'cd /var/home/afton/.dev/ReRiff && make -C Decomp OUT=../build/decomp-container all test-recovered -j2'
```

The fixed-address link of `crt_entry.S` resolves only the two original targets;
it does not inject the original implementations. The checker rejects unknown
EXE hashes, unknown blocks and wrong output lengths and compares every byte.
The initializer's expected SHA-256 is
`e67f4ccf8d2a63c949ac87ba6ab8364d0d8d1d005d1fbe543edf6e14465a8e24`;
the CRT wrapper's is
`c478c036b149ef24c88a8d31efbd51530cdfa40b19758b6949100aec7b5a17ab`.

## Remaining startup work

The CRT common body is now reconstructed. It acquires the startup lock using
the thread's stack-base value, distinguishes in-progress/already-completed
initialization, runs C/C++ initializer tables, releases the lock when acquired
by this invocation, and conditionally invokes the validated callback. It scans
past the executable name with quote tracking and multibyte lead-byte handling,
skips whitespace, then dispatches to VA `0x005CFC90` with the image base,
command-line tail and show-window mode. It preserves the original early return
of 255, exception filter/handler and exit/cleanup branches.

The import table identifies `__crtGetShowWindowMode`, `_acmdln`, `_ismbblead`,
`exit`, `_exit`, `_cexit`, `_initterm_e` and `_initterm` at the referenced slots.
The SEH frame helpers install and restore `fs:[0]`, encode the scope pointer
with the security cookie and preserve the original register/stack layout. The
28-byte scope table retains its original cookie fields and filter/handler
addresses inside `crt_startup.S`. It is structured data, not additional code.

The [application entry](APPLICATION-ENTRY.md) now also matches, including its
single-instance check, timer setup, startup branches, main-loop dispatch and
shutdown waits. Selected callees now match; see [worker recovery](WORKER.md).
The worker body now matches as well. The render-thread frame body and most
object implementations remain open.
External exception dispatch at `0x008F9CE9`, callback validation, runtime helper
thunks and initializer tables still need recovery.
No assembled block was executed as a substitute Windows runtime in these tests;
the validation is exact byte comparison of source-generated fragments. A full
PE build, relocations and original source/toolchain reconstruction remain open.
DLL modifications for the working sync server are outside this EXE reconstruction.
