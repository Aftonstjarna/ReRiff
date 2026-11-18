# Text-entry prototype and kana search groundwork

## Original reference and scope

Original Windows Game.exe: Steam depot 351991/build1687655, 8,635,392 bytes,
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted recovery evidence, original strings, CLI picker/confirmation methods
and the existing native overlay adapter. No live decomp checkout was modified.
This is an intentional extension, not original byte-matching reconstruction.

No complete editable game text widget was identified in the examined UI
constructors/types and input methods. That is a bounded negative finding, not
proof that no unused editor exists anywhere in the executable. Text rendering,
focus, confirmation overlays and the picker reel are present and reused.

## Reused overlay

The existing gaQuitConfirmMenu vtable is image+0x7A7900. In addition to the
previously guarded enter/init/input slots, the prototype wraps slot3 (tick):
original callable thunk RVA0x360BC1, exact six bytes `ff257479ba00`, whose
absolute target location relocates to image+0x7A7974. Existing slots2/15/24
remain at RVAs0x361068/0x361B88/0x360B60 with original target locations
image+0x7A7978/0x7A7980/0x7A796C. The installer checks all four before writing.

The native state9 overlay retains original visual construction, confirmation
buttons, lifecycle and callbacks. Editor rendering uses its existing title and
message items; no separate renderer or substitute confirmation menu is added.
The window hook transports UTF-16 character/key/clipboard operations into a
small shared editor state protected by a critical section. Only the menu thread
calls native UI or pushes/pops an overlay. Accepted addresses update pending
Servers settings; the existing explicit save commits them. Cancel discards the
edit. No addresses or typed text are written to diagnostic logs.

The new rows use the original Settings spacing, focus and native reel scroll.
Tests exercise the expanded rows and preserve original Audio/input isolation.
Native tests passed for UTF-16 surrogate-safe editing, clipboard, deferred
accept/cancel, URL scheme/host validation, duplicate-editor refusal and native
confirmation ownership. Real typing, controller focus, IME composition and
appearance still require live validation; automated character insertion does
not establish those behaviors.

## Search groundwork

KanaSearch creates derived search keys only: NFKC normalization, kana script
folding and a kana-to-romaji map including digraphs, voiced forms, small-tsu
consonant doubling and prolonged vowels. The original name/path is preserved.
It supports literal kanji matching but does not infer kanji readings.
Fourteen managed checks passed on the Windows CLR for mixed names, tsu/つ/ツ,
halfwidth and combining forms, kana script equivalence and preserved filenames.
This matcher is not yet wired into the live picker.

Original gaFileSelectInterface.setupDirectoryContents token0x0600104A at
RVA0x2D5814 destroys and rebuilds the native 28-byte dirEntry vector. Filtering
must preserve its wstring ownership and the one-to-one row/reel mapping; hiding
arbitrary rows or rewriting just one vector is not a validated search method.
Nested native overlay ownership and space for a search control must be checked
before adding a field to the picker. The prior unsafe wraparound is not reused.

## First live editor test and undo extension

The user fully restarted and confirmed typing, caret movement, paste and cancel
in the server-address overlay. No changed address was saved in this test.
The picker layout trace showed native overlay ownership already belongs to the
picker (manager+44 == picker). A second confirmation overlay therefore cannot
be assumed safe for search. The observed first directory row was y286, height40,
with11 source entries; this is one tested layout, not a universal space budget.

The shared editor now has bounded per-field undo/redo history. Consecutive typing
within750ms can form one step; paste and cursor/selection changes break groups.
Tests passed for Ctrl+Z, Ctrl+Y, Ctrl+Shift+Z, Unicode paste reversal, selection
replacement, history branching and reset when opening another field. These
are new ReRiff behaviors, not recovered original editing functionality.

## Address viewport and caret follow-up (2026-11-15)

Consulted the recovery project's `Decomp/RERIFF-EVIDENCE.md` confirmation
lifecycle evidence, then dumped the original methods with dnlib from the
hash-verified depot executable above. Local inspection artifact:
`build/text-entry-layout-probe/original-confirm.il` (original IL, not a patch).

`gaQuitConfirmMenu.enter`, token `06001589`, RVA `00360BC8`, stores panel
height at menu+256: resolution height times 0.7 (IL+001C..0028,
+031C..0325). It stores width at menu+252: that height times
1.3272727272727274 (IL+0326..033B). `initScreens`, token `0600158B`,
RVA `003610AC`, passes menu+252 as the message text's width to the native
`elUIItemText.create` call (IL+0315..0333), then stores the message at
menu+264 (IL+0338..0345). The editor retains this original text item, its
width constraint, fonts and native dialog controls. These are original IL
observations, not matching-byte reconstruction credit.

Intentional ReRiff changes: routine tips are hidden; the address viewport now
allows 48 UTF-16 units instead of scrolling after 22 characters. The sample
`https://official-na-east.reriff.online` stays complete at either caret end.
Longer strings retain a bounded viewport without cutting surrogate pairs.
The caret alternates with a blank using GetCaretBlinkTime, honors disabled
blinking, and resets on field opening, typing, paste and key navigation.
New regression cases cover full-address rendering, blink timing, navigation
reset, disabled blinking and longer-address scrolling. Final appearance across
resolutions/UI scales remains a live test; no panel or button geometry changed.

## Whole-library search groundwork (2026-11-15)

The user specified whole-library search, including unimported audio files.
`PickerSearchIndex` now caches literal/romaji keys and returns stable source
indices, preserving duplicate names and original strings. `LibrarySearchCache`
projects filename, song ID, title and artist from the existing v19 cache format
documented by `NativeLauncher/song_sync.h` and recovery evidence. It rejects
unsupported headers, malformed UTF-8, truncated records and trailing data,
and never writes the cache. `LibrarySearchScan` recursively enumerates supplied
music roots, merges cached metadata by full path, includes unimported audio,
deduplicates overlapping roots, supports cancellation and reports partial or
unreadable results. It does not decode files; indexing M4A does not restore its
decoder. Nested reparse points are skipped to prevent cycles; an explicitly
selected music root may itself be a link.

These are intentional ReRiff components, not a completed in-game search bar.
Native root discovery, asynchronous menu lifecycle, search-field placement,
result-list construction, and selection handoff remain to be integrated and
live-tested. No game DLLs were installed for this groundwork.

Rechecked the depot Game.exe SHA-256 stated above. Read-only dnlib inspection
is retained in `build/search-native-probe/original-library.il`, with the probe
source alongside it. Consulted recovery `Decomp/RERIFF-EVIDENCE.md` song-cache
and M4A evidence. Additional original observations:

- `gaRecentTracksMenu.setupRecentTracksList`, token `060015A0`, RVA
  `0036394C`, starts by copy-constructing a localSong vector from gaSaveGame+176
  (IL+0000..0012), then calls applySongListFilter and removeDuplicates
  (IL+0013..0022). This is a copied display list, not the live source vector.
- `applySongListFilter`, token `0600159E`, RVA `00363878`, branches on
  s_showBundled. The non-bundled branch sorts by last played and retains at
  most 20 entries (IL+0057..00C6); entries have stride200. Removal uses the
  original vector erase, preserving native ownership. This routine is not
  already a whole-library text filter.
- `gaFileSelectInterface.onFileSelected`, token `0600104F`, RVA `002CEBD4`,
  uses setChosenMusicFilename, sets the new-track filename, pops the picker
  overlay, and requests state18. Reusing its filename conversion/selection
  lifecycle requires verification; raw UTF-8 or guessed std::string arguments
  are not established ABI substitutes.

These observations confer no original matching-byte credit.

## Native field, result vector, and controller keyboard candidate

The next candidate connects whole-library indexing to an inline native field.
It reuses the shared editor (selection, Unicode paste, undo/redo and blinking
caret) without opening another game overlay. Click/Ctrl+F or Xbox Y opens it;
A/Enter finishes editing, B/Esc cancels. Native X/favourites remains unchanged
outside search, as do LT/RT alphabet jumps. Favourite actions are ignored while
showing search results. The native font measures the field; the viewport is
shortened until it fits, without scaling/compressing native rows. When the
space above the panel cannot hold a native-height field, the existing path
header is reused. Actual appearance and pointer/controller behavior require
live validation, including high UI scales.

`NativePickerSearch` obtains music roots from the original My Music folder,
original saved library shortcuts, the current picker folder and cached song
folders. Scanning/key generation run on workers with session/revision guards;
only the menu thread calls original game methods. Native Windows long-path
expansion reconciles saved short paths with enumerated files. Cache location
matches the launcher's CSIDL_APPDATA / Roaming Octane/DAT/cache.bin path.

The original directory-entry vector is retained through its own swap method.
Search entries use native wstring construction/destruction and vector push_back;
the original setupDirectoryList builds all rows and owns its reel/scrollbar.
Clear/close swaps the original vector back and destroys the search vector via
its original destructor. Result clicks use original getSystemShortFilename and
onFileSelected, retaining the hst: prefix and original new-track transition.
Neither displayed labels nor normalized keys are treated as actual file paths.
The candidate displays the first 200 matches and reports that limit. The scan
is bounded at 100,000 audio files and reports unreadable/partial scans. Nested
reparse points remain excluded; this is not an unrestricted disk-wide search.

Additional bindings checked against the hash-pinned original metadata:

| Function | Token | RVA |
| --- | --- | --- |
| wchar_t string from character pointer | 06001060 | 002CEF28 |
| wchar_t string destructor | 06000985 | 0025BBFC |
| char string from character pointer | 06000023 | 001D4ACC |
| char string destructor | 06000024 | 001D43C0 |
| dirEntry vector destructor | 06001071 | 002D43D4 |
| dirEntry vector push_back | 06001074 | 002D5410 |
| dirEntry vector swap | 0600109F | 002CE000 |
| setupDirectoryList | 0600105B | 002D2574 |
| getSystemShortFilename | 0600105D | 002D0C14 |

The picker input slot24 at image+7A54BC points to RVA002D6940, original
bytes `ff252c55ba00`; the relocated target location is image+7A552C. All six
bytes/operand and the slot are guarded before interception. setupBasePaths
(token0600105C) copies gaSaveGame+348's libraryShortcut vector, with stride48
and the path string first (IL+010A..0151, +01D9..01E1). The picker tickInput
uses the X360 controller's slot3 with 0x4000 for favourites (IL+022B..024A).
The new Y shortcut uses 0x8000 with release-before-press gating. These are
intentional hooks and observations, not reconstructed matching bytes.

### Steamworks contract

Checked Valve's [ISteamUtils documentation](https://partner.steamgames.com/doc/api/ISteamUtils),
[gamepad-emulation guidance](https://partner.steamgames.com/doc/features/steam_controller/steam_input_gamepad_emulation_bestpractices),
and published [ISteamUtils](https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/steam/isteamutils.h),
[ISteamClient](https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/steam/isteamclient.h)
and [callback ABI](https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/steam/steam_api_common.h)
headers. The shipped steam_api.dll's default Utils string is SteamUtils005;
the extension requests SteamUtils010 through the existing SteamClient/pipe,
without replacing the DLL or initializing another session. If unavailable,
ordinary field editing remains usable.

An actual connected XInput device gates automatic keyboard requests. This uses
the same XINPUT1_3 library imported by the game and also sees Steam Input's
emulated Xbox devices. Steam Input's GetConnectedControllers alone would miss
ordinary Xbox devices not opted into Steam Input; Big Picture/Deck status is
not used as a substitute for controller connection.

The floating single-line keyboard is preferred: it delivers OS character/key
events to the editor and receives field bounds in client pixels. The fallback
uses normal/single-line ShowGamepadTextInput with existing UTF-8 text. Submitted
UTF-8 is retrieved only inside GamepadTextInputDismissed_t; cancellation does
not replace the query. Callback714 uses the current 12-byte structure, and
floating dismissal738 has an empty 1-byte C++ structure. The three-slot,
12-byte x86 CCallbackBase adapter follows the published layout. Existing game
SteamAPI_RunCallbacks drives delivery. No manual callback pump is introduced.
Only one request may be pending; late callbacks are fenced from new fields.
Game actions/focus-loss cancellation are suppressed while Steam owns typing.

Focused native fixtures cover editor isolation, Y/A/B routing, bounded measured
viewports, overlay callbacks, disconnected/unavailable keyboard fallback,
Unicode submission and stale dismissal. Managed fixtures cover scanning and
native ownership using explicit native UI/string/vector mocks. These tests do
not establish live Steam overlay availability or original UI rendering.

Local candidate installed after six native and two managed suites passed. The
managed bridge fixture includes the original updateVisibility binding
(token06001056, RVA002CC7FC) after list rebuild, so clipping is refreshed even
while editing pauses picker input. DLL receipt and rollback are local under
`build/picker-search-field/install.json`; 158 protected paths were preserved.
Steam overlay, real controller interaction and appearance remain live-test
items. The candidate displays `ReRiff development build`; official release
builds retain their exact version label. Development/release native binaries
were separately compiled and their display strings verified. No release or
commit was made for this candidate.

### Live search follow-up: direct picker Back handling (2026-11-15)

Original reference remains Game.exe SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
The retained `build/picker-alphabet/original-picker.il` shows
`gaFileSelectInterface.tickInput`, token `06001055`, RVA `002CFC00`:
IL `0016`–`0042` queries device vtable slot 1 with logical button 2, checks
whether the path vector at menu+264 is empty, and calls
`gaStateManager.popOverlayState(..., true)` directly. It does not go through
picker row selection. `gaFileSelectInterface.tick`, token `06001057`, RVA
`002CFF08`, calls tickInput at IL `0045`. Thus a row callback alone cannot
consume Back at My Computer. The same tickInput reads held logical directions
through device slot 0 (for example IL `008B`–`0091`).

Intentional ReRiff integration: inspect logical Back before original picker
tick while editing or showing filtered results; clear search and suppress
native tick until the Back control is neutral, followed by the short release
interval. Keep the field active over focus changes, because an independently
opened keyboard can temporarily take focus. No original bytes are claimed
reconstructed or matched by these patches.

The user's 46.9-second recording shows unchanged folder rows while a query
remains Searching. Local diagnostics report Steam declining both keyboard API
requests. Progressive scan snapshots and immediate cached-track search replace
the all-or-nothing index publication. Worker results retain the exact song
snapshot used for matching, preventing indices from a previous snapshot from
selecting a different track.

The existing Steamworks APIs remain first choice. A connected-controller-only
fallback dispatches `steam://open/keyboard` through the registered handler after
both API calls return false. This is a desktop-client fallback, not a successful
Steamworks callback or a guarantee the keyboard becomes visible. The protocol
is observed in the firsthand Steam Linux report
https://github.com/ValveSoftware/steam-for-linux/issues/12920 ; the API contracts
remain https://partner.steamgames.com/doc/api/ISteamUtils . Manual and fallback
keyboard visibility/input still require a live Steam retest.

### Cache failure and original window entry (2026-11-15)

The live catalogue has the supported header (magic1128350512, version19,
70 records, recordVersion18), but strict UTF-8 decoding fails at zero-based
record24, field3, byte offset3181. Search treated this optional catalogue error
as fatal. A later query cleared Failed without restarting the worker, resulting
in permanent Searching. ReRiff now rejects the optional catalogue and continues
filesystem enumeration, and a failed worker remains visibly failed.

The original Windows procedure reported by display tracing is VA0048C6E0,
RVA0008C6E0, in the same SHA-256-verified original executable. Byte comparison
and disassembly show its first seven bytes are `55 8B EC 53 8B 5D 0C`:
`push ebp; mov ebp,esp; push ebx; mov ebx,[ebp+0x0c]`. These are complete,
position-independent instructions. Existing bounded native detour machinery
can preserve them in a trampoline. The guarded ReRiff entry route handles
shared editor messages before forwarding to the original; the inactive editor
forwards all messages. The original-file test directly invokes this retained
WNDPROC address with Unicode character messages and checks inactive forwarding.
This covers callers bypassing SetWindowLongPtr subclasses. Whether Steam uses
this route in the live failing session is not yet established; a bounded log
records accepted messages on this route without recording typed text.
