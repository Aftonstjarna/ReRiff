# Unicode metadata transport probe

## Scope and existing evidence

The initial work is an opt-in client diagnostic and isolated backend
compatibility probe. That observer changes no metadata values. The later
opt-in native-boundary correction is documented separately below. Neither
changes the backend implementation, endpoint selection or existing stored
player records. No production server migration or deployment is made.

`UNICODE-TEXT-TRACE.md` established that the tested Japanese source tags reach
native text layout intact, while an artist's U+2010 had already become U+003F
by artist-label layout. That earlier trace did not locate the conversion or
establish what was transmitted to the server. Font rendering alone cannot
answer whether metadata is retained through that path.

Original reference hashes from `packaging/vanilla-reference.json`:

- `Game.exe`: `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
- `Parse.NetFx45.dll`: `4e2f65147faec13ae5333337e3c7cd9e25028f3119fe817e059b1d2eab678522`.
- `Parse.dll`: `21757cde7fa348089fe6398f50bc225371e38208b556df47c18a18259cf86ccb`.

The local probe is layered onto the installed ReRiff-patched transport, not
claimed to match the original binary. Its prior DLL/helper hashes are retained
privately with the local installation backup. No matching-byte credit is claimed.

## Transport observation

Inspection of the installed transport verifies
`Parse.Internal.HttpClient.ExecuteAsync(HttpRequest, uploadProgress,
downloadProgress, CancellationToken)` returning
`Task<Tuple<HttpStatusCode,string>>`. Its original code consumes the request's
`Uri`, `Method` and `Data` (`Stream`) properties.

`Patcher --metadata-trace` adds observation at request entry and at the original
return paths. It retains the existing offline gate, headers, News wrapper,
transport calls and return-path count. Inspection verifies the staged helper's
strong-name identity matches the installed helper. This is an intentional
diagnostic patch, not reconstruction of the original SDK.

`NewsHeaders/MetadataTrace.cs` is disabled unless the installed `server.ini`
contains `metadata_trace=1` under `[diagnostics]`; restart after changing it.
Output is `logs/ReRiff-metadata.log` beneath the game directory. It observes:

- Available JSON MemoryStream data before the transport consumes it. Copying
  the buffer does not seek, read, replace or modify the original stream.
- JSON metadata predicates in the request's `where` query parameter.
- Completed response strings, without replacing the returned task or response.

The observer logs only allowlisted song fields such as SongName, ArtistName,
AlbumName, SongID, title, artist, album and genre. Non-ASCII UTF-16 units are
escaped, including surrogate pairs. It omits hosts, headers, credentials,
complete bodies, files and binary uploads. Routes are restricted to classes,
batch and song-registration paths; this managed observer does not intercept
separate native-launcher HTTP calls. Strings, traversal, records and file size
are bounded. Observation errors include exception types only. A missing record
does not prove a missing request or missing server data.

This is a serialization-boundary snapshot, not packet capture: request data
alone does not prove network delivery, especially for offline/cancelled calls.
Only a response comparison can show the returned metadata at this boundary.

## Validation

`Managed/MetadataTraceSmoke.cs` runs under isolated .NET Framework/Wine. It
checks disabled operation, exact task identity, retained stream position,
request/query/response observation, Japanese/U+2010/supplementary-unit retention,
credential/hostname exclusion, authentication-route exclusion, cancellation,
original failures, and response observation after request-stream disposal.

`scripts/test_metadata_unicode.py` uses temporary storage and Flask's in-process
test client against the maintained backend. It verifies UTF-8 and escaped-JSON
create/read/query/update/disk-reload paths for Score song metadata, including
Japanese, Chinese, Korean, a supplementary ideograph, emoji and a combining
sequence. It also verifies title/artist persistence through the built native
song-catalog registration adapter. Both test cases pass.

These backend probes show no character loss requiring a server fix in the
tested checkout/build. They do not establish the behavior of the user's running
server version or prove that the live client sends equivalent values. A live
Japanese-song load and saved score are the next comparison. Compare source
tags, serialization-boundary request fields, returned fields and native layout
records before attributing any remaining loss to a client or server stage.

## First live result

The live `/classes/Track` request already contains a Windows-1252-style
misinterpretation of UTF-8 in its `Title` field. Its response contains exactly
the same UTF-16 units. Mapping those characters back to the corresponding
Windows-1252 bytes (retaining identity for undefined control-byte positions)
and decoding as UTF-8 recovers the original Japanese title exactly. This is
diagnostic evidence only; no heuristic repair is applied to stored records.

Private snapshot hashes:

- Metadata trace: `a286d22f38bee9effa36be527c73aef4e2dc85714ad84fdf3c090f1ccee1f081`.
- Native layout trace: `5de3b58dfdeac030aea225cdf899a30ce44f5252a9df3d3ce0afa25721ff23a9`.

Inspection of original `ParseOctane.dll` (SHA-256
`d661606db8658adb54d5be18b3e3a3a6f07c8e738f2d78556ece42355f8e62e4`)
identifies `wsNetworkParseImpl.updateObject(int,sbyte*,sbyte*)`, metadata token
`060000DE`. IL `000D` constructs the managed key using `String(sbyte*)`;
IL `0013` constructs the managed value using the same code-page-dependent
constructor; IL `0018` calls `ParseObject.set_Item`. This is consistent with
the observed UTF-8-as-code-page conversion before JSON serialization. The
exact native caller/buffer contract still requires validation before patching
this conversion; other native string paths must not be changed indiscriminately.

Separately, the native pause/title trace receives the correct Japanese UTF-16
units but glyph lookup chooses U+003F for those characters. Thus the visible
question marks are also affected by incomplete fallback rendering coverage,
independent of the outgoing Track metadata conversion. A font-context probe
adds primary/alternate visual and native fallback pointers to non-ASCII layout
records, without changing arguments, to verify the affected drawing route.
The subsequent user-triggered leaderboard and pause run confirms both affected
calls have an owned native fallback font but a null alternate visual argument.
The working file picker has both visuals. This confirms the direct native
drawing-route gap; see `CJK-FONT-INTEGRATION.md` for the scoped native extension
and its remaining animation/lifecycle limits.

The live echo comparison does not indicate server-side alteration of the Title.
No backend fixes, migrations or live record rewrites are justified by this
evidence. The client conversion and direct-drawing fallback coverage remain
open work.

## Scoped native-boundary UTF-8 prototype

Original `wsNetworkParseImpl.refreshObjectFromString`, token `060000FC`,
confirms the paired return conversion: IL `002E` calls
`Marshal.StringToHGlobalAnsi(string)`, copies at most 4095 bytes with the
original native `strncpy` into a shared 4096-byte buffer, and calls
`Marshal.FreeHGlobal` on the temporary. Returning correctly encoded server
Unicode through that ANSI conversion can therefore lose the same characters
the outgoing ANSI constructor misdecodes. ObjectId has a separate branch but
converges on the allocator call, so any correction must preserve opaque fields.

`Patcher --metadata-utf8` intentionally changes only two original managed
method bodies in the existing mixed-mode bridge:

- In `060000DE`, replace only the **value** constructor with
  `ReRiff.MetadataText.DecodeValue(value,key)`. Retain the original key
  constructor, dictionary lookup and ParseObject setter.
- In `060000FC`, pass the original key to
  `ReRiff.MetadataText.AllocateValue(value,key)` at the original temporary
  allocation boundary. Retain the original static buffer, native copy,
  exception handling and FreeHGlobal cleanup. Existing branches enter the
  replacement operation through the same instruction object.

The helper is signed with the existing ReRiff helper identity and is disabled
unless `[diagnostics] metadata_utf8_probe=1` is set before process startup.
Disabled operation retains the original code-page behavior. Enabled operation
uses strict UTF-8 only for Title, Artist, Album, Genre and the corresponding
SongName/SongTitle/ArtistName/AlbumName aliases. Opaque IDs, authentication,
other properties, file operations and query methods are unchanged. Invalid
UTF-8 retains the original ANSI decoding; no heuristic mojibake repair or
Unicode normalization is performed. The helper does not repair a character
already lost upstream in the native source metadata.

Outgoing native-to-managed metadata input is checked for a terminator within
65536 bytes. UTF-8 return allocation uses at most 4094 data bytes followed by
NUL inside the original 4095-byte copy range, stopping at scalar boundaries.
The temporary still uses HGlobal ownership and the original code frees it.
The original shared buffer remains owned by the bridge; no caller ownership
or pointer lifetime changes. Bridge diagnostics report only the metadata
allowlist, escaped text and at most 64 input bytes, through the existing
bounded opt-in metadata log. They omit endpoint/authentication information.

`Managed/MetadataTextSmoke.cs` runs offline under the game's .NET Framework,
loads the actual patched original mixed-mode ParseOctane DLL, instantiates its
original implementation, and invokes `060000DE`/`060000FC` against a ParseObject
in its original object dictionary. No HTTP request is made. Disabled and
enabled processes pass for Japanese, Chinese, Korean, U+2010, supplementary
ideographs, emoji, combining text, ASCII/empty/null cases, invalid UTF-8,
unknown/opaque properties, already-misdecoded managed strings, and oversized
UTF-8 return values. The native return test executes the original copy/free
path rather than simulating ownership in a separate wrapper. Additional
boundary fixtures stop immediately before a three- or four-byte scalar.

Inspection of the staged bridge verifies 240 unrelated managed methods are
unchanged, all method signatures/tokens are retained, 25 native method RVAs
and their first 16 entry bytes match, and the existing endpoint-selection
hook remains. This is a limited entry-byte comparison, not a full binary
match or matching recovery of the intentionally patched methods. The patcher
refuses a different write-body shape, missing original return allocation/free,
unsigned helper or duplicate patch. Original depot files are never rewritten.

Reproducible staging is provided by `scripts/build-metadata-text-probe.py`,
using supplied .NET SDK, Framework/facade references, built Patcher and a
separate output directory. The script copies only required SDK/helper DLLs;
it does not copy the installed server configuration or user data. Run its
MetadataTextSmoke executable once with `--disabled`, then without arguments.
The existing three isolated backend Unicode tests also pass. Live bridge
input/request/response comparison is the remaining validation step. Existing
misdecoded server records are deliberately not rewritten by this prototype.

## Live UTF-8 upload verification

After restart with the scoped prototype enabled, the user opened a new
Japanese track's leaderboard. The native input for Title is the valid UTF-8
sequence `E3-81-82-E3-81-95-E3-81-B2-E3-81-AA-E3-81-90`, decoding to
`あさひなぐ`. The fresh `/classes/Track` request and its server response both
contain exactly `U+3042 U+3055 U+3072 U+306A U+3050`, with no Windows-code-page
reinterpretation. Native font layout also receives those exact title units.
The diagnostic log is append-only and retains earlier misdecoded records;
the comparison starts at the new `bridge-in` Title record rather than treating
historical entries as a regression in the fresh run.

Private snapshots (filenames, endpoints and player data are not submitted):

- Metadata: `08c0615f50f938e7bb02886d9ddc353b146ec3ca382d989ba2b15cfab523346e`.
- Native layout: `167cc9ac21cf18798e45525b05648924703e87d591079826fd1681e157c08821`.

An automated comparison independently decodes the native bytes and escaped
trace fields, then asserts exact equality of bridge input, request, response
and native title layout. It passes. This verifies the new Track upload and
response path on the live server and supports the client-side correction;
no server patch is indicated by this result. The native returned-string
allocation/copy/free path remains covered by the offline original-bridge test;
no `bridge-out` song field was observed in this particular live screen flow.
Existing misdecoded stored records, fields outside the allowlist, source-tag
loss before the bridge, and text query conversions remain separate work.

## Remaining artist placeholder after HD font validation

On 2026-11-11 the user confirmed sharper CJK rendering but reported the
remaining literal `?` in `BUCK?TICK`. Read-only inspection of the source MP3
again confirms Artist `BUCK\u2010TICK`. The current trace records the Artist
query already as `BUCK?TICK`, its response unchanged, and the scoped UTF-8
bridge-out receiving the same literal question mark. The associated response
Title also retains the earlier UTF-8/code-page damage described above. This
is an existing affected Track response, not evidence that the HD atlas fails
to contain U+2010. It does not locate the first lossy artist conversion or
establish whether the current query value originated in source decoding,
local saved state, or an earlier server lookup. A source-to-query comparison
remains required before correcting that path. No replacement heuristic,
record rewrite, cache deletion or server mutation was performed.

Private snapshot hashes, with complete logs excluded from submission:
metadata `409715fe4676cce5bf3775a7327452b910ce49d6f030aa04f673d5523e583994`;
native text `54a2571d4342b3c6a84d15798a6eccac899a7a01c304cb10e66ab3c25ffa8ab7`.

Inspection of original Game.exe's mixed-module IL identifies a separate
`gaNewTrackScreen.updateEssentiaLastFM` path converting its HTTP body through
`elCodec.UTF8ToWindows1252` at IL_00BF before JSON processing. This is a
candidate to investigate, not proof of responsibility for this artist's
initial loss. Its existence does not justify globally replacing that codec.
Original executable hash and provenance are as recorded above; no original
or patched executable was changed in this investigation.

## Native artist source probe

The opt-in `metadata_source_trace=1` diagnostic (also requiring the existing
`unicode_trace=1`) observes original native boundaries without changing values:

- The wsSystem elSystemPC secondary interface has its own vtable at preferred
  VA 0x9FA1A8. Slot +40 calls 0x44F0C0, returning ECX+0x16C (Title); slot
  +44 calls 0x44F0D0, returning ECX+0x26C (Artist). Both getters are a six-byte
  LEA followed by RET. Guards are respectively `8D 81 6C 01 00 00` and
  `8D 81 6C 02 00 00`. The original gaNewTrackScreen tick calls +44 for its
  local 67 and +40 for local 66 on the PC path. These are already decoded
  buffers, not source ID3 bytes and not the server response.
- Native MP3 decode 0x4F87E0 obtains metadata at 0x4F89E8. At 0x4F8A09 it
  copies the first ID3v2 text pointer (+4) to the EBP+0x14 output, and at
  0x4F8A51 the next pointer (+8) to EBP+0x1C. The v1 branch copies +3/+0x21.
  These are the Title/Artist outputs. A post-copy observer at 0x4F8A5A,
  bytes `8B 45 0C 8B 4C 24 10`, reads only those bounded output buffers.
  The worker 0x48D9C0 forwards them into its +0x4C/+0x14C buffers; these map
  to the PC interface getters above. This observation does not alter audio
  decoding, allocation, copying, thread events or output pointers.
- Original Game.exe metadata token 0x06001802 has native RVA 0x1C2500 and
  name elCodec.UTF8ToWindows1252. Its guarded six-byte entry is
  `55 8B EC 83 EC 0C`. The source frame is cdecl(input,length,std::string&).
  A wrapper executes the original function unchanged, then records only
  input length/U+2010 occurrences, output length/question-mark count and
  original result. The whole Last.fm body, URLs and arbitrary JSON fields
  are never written to the log. Input/output scanning is capped at 1 MiB.

The bounded MP3/getter observations write escaped UTF-16 interpretations and
up to 64 source bytes for Artist/Title only, using the existing locked,
deduplicated Unicode diagnostic sink (4 MiB/4096 records). Logs remain private
and ignored. Native getters, codec return values, arguments, reference/string
ownership, LastError and post-original integer/flags/x87/SSE state are retained.
No metadata replacement or live record mutation is part of this diagnostic.

The original-instruction Wine harness confirms the real native getters return
the same buffer pointers with LastError preserved through the observers. It
also executes the original codec: UTF-8 `BUCK\u2010TICK` becomes exactly
`BUCK?TICK`, returning false. The traced original function retains that result,
nine-byte native SSO string and capacity 15. The diagnostic records one input
U+2010 and one output question mark. This establishes the original codec's
loss for this character, but does not yet prove that Last.fm caused the live
artist loss. A subsequent game restart/load must compare MP3 copy, decoded
getter, legacy-codec counts, managed request/response and native text layout.
The existing HD font/native ownership, alignment, highlight and UI scaling
regressions also pass with these diagnostics compiled in.

The original reference hash remains unchanged. Native address discovery used
read-only RTTI/vtable inspection, depot disassembly and mixed-module metadata;
no direct additions were made to the live recovery tree. All hooks are
intentional diagnostics, not byte-matching reconstruction claims.

## Live source-to-upload trace result

The next user-triggered load of a different BUCK-TICK MP3 produced all four
installed observer records. The MP3 tag-copy Artist and decoded-getter Artist
both contain the identical valid UTF-8 bytes
`42-55-43-4B-E2-80-90-54-49-43-4B` (`BUCK\u2010TICK`). The legacy-codec event
then reports 5496 input bytes, one input U+2010, 5469 output bytes, 13 output
question marks, and false result. The count is for the whole response; the
observer intentionally does not log its body or associate every question
mark with a particular field.

The subsequent native font layout already contains literal `BUCK?TICK`.
The new Track bridge-in Artist is the exact ASCII byte sequence
`42-55-43-4B-3F-54-49-43-4B`; request and response agree. This run therefore
rules out initial MP3 tag loss, source-buffer loss, missing hyphen glyphs,
and the server changing a correct uploaded artist value for this sample.
It also shows that the problem is not confined to previously damaged server
records: a fresh Track upload from this run contains the damaged Artist.

Original `gaNewTrackScreen.updateEssentiaLastFM` supplies the mechanism:
IL_00BF runs the whole HTTP body through the native UTF8ToWindows1252 codec
before parsing; IL_021F/026F/02B6/02F6 reads `artist/name`; IL_0323 assigns
that value to its artist string at +480 when the name is accepted and differs.
The source/getter, codec, post-conversion display and upload sequence, together
with the original codec regression and this overwrite path, identifies the
Last.fm response conversion/artist replacement as the correction target.
The safer scope is this response path rather than changing the shared codec
or guessing replacements in arbitrary stored question marks. A correction
must still verify the original JSON UTF-8 contract and preserve the native
string allocator/cleanup and unrelated callers.

An independent local assertion decodes both observed source buffers and
compares the outgoing native bytes, request and response; it passes. Private
snapshots: Unicode `1e8c290d7171fada2059f38d76940bf2efc36734f09f0a00805022178753a6c4`;
metadata `ab6783cc1c2762601a04d0ec6d70e5dacf5a4023815c8307648febce8188cecc`.
No metadata correction, server update, cache deletion or record rewrite has
yet been applied. This supersedes the earlier uncertainty about whether the
current query's artist might only have come from an old saved record.

## Scoped Last.fm UTF-8 correction prototype

The user authorized a local correction after the live source-to-upload trace.
`ExperimentalMenu/lastfm_utf8.h` changes only the in-memory CIL block
IL_00A6..IL_00C4 of original Game.exe method 0x060014DE,
`gaNewTrackScreen.updateEssentiaLastFM` (method RVA 0x34FE9C, 12-byte header,
0x622-byte code). The patch site is RVA 0x34FF4E, length 31. It is opt-in via
`[diagnostics] lastfm_utf8_probe=1` and installed during ReRiff startup before
the user can enter the track-screen Last.fm flow. It requires a fresh process;
it does not invalidate an already JIT-compiled method or claim NGen coverage.
The existing complete on-disk executable hash gate remains required.

The original block loads local 48's raw data and length, local 47's address,
and calls UTF8ToWindows1252 (0x06001802), discarding its bool result. The new
block is `ldloca.s 47; ldloca.s 48; call 0x06000631; pop`, followed by NOPs to
keep every later offset unchanged. Token 0x06000631 is the existing
`std.basic_string<char,...>.=(dest*,const source*)` copy operation, already
used elsewhere in this same original handler. Both locals are constructed
before this block; original scope cleanup continues to destroy both. No
new allocator, native string wrapper or JSON parser is introduced. The original
response bytes are retained through the original copy/parse pipeline instead
of first being converted to a restricted code page. The shared codec's behavior
and every other caller are retained; the separate read-only trace may still
observe its entry. Game.exe and original archives are never rewritten on disk.

The runtime patch checks the exact fat-method header
`0B 30 08 00 22 06 00 00 4A 14 00 11` and all 31 original block bytes. The
replacement starts `12 2F 12 30 28 31 06 00 06 26`. A dnlib verification of the
original and patched images confirms the copy token/signature, its existing
use in this handler, no outside branch or EH boundary entering the middle of
the replaced block, and identical instructions after IL_00C5. Native tests
cover disabled operation, altered-guard refusal, the exact installed block,
NOP extent and idempotence. No original metadata token, local signature,
method size, branch destination or exception range is added or relocated.

`Managed/LastFmUtf8Smoke.cs` loads a disposable DLL-form original reference
under genuine x86 .NET Framework. Its DynamicMethod uses the same native value
locals 47/48 and original call token to execute the replacement copy shape.
ASCII, empty, U+2010, Japanese/Chinese/Korean, JSON, escaped JSON bytes and a
long heap-backed value retain their exact UTF-8 bytes and terminator. The
source remains unchanged, heap copies have independent allocations, and the
original `_Tidy` cleanup releases the destination. The fixture changes only
the three previously documented reference PE header fields, not original
method bodies; its SHA-256 remains
`573c47560a445cc0c4f83adafdc4ec08fe75e25eaa332d55483864d3e616c46a`.
It is not substituted for the depot original in provenance or hash checks.

### Matching corrected artist queries to UTF-8 uploads

Original ParseOctane methods 0x060000F0 (`findAllObjectsTypeWhere`) and
0x060000F4 (`findSortedObjectsTypeWhere`) also construct equality values with
`String(sbyte*)`. Leaving that conversion in place would misdecode a newly
preserved UTF-8 artist during lookup, although upload values were already
corrected. `Patcher --metadata-query-utf8` changes only the four object-query
WhereEqualTo value sites in F0 and the single value site in F4 to the existing
field-scoped `MetadataText.DecodeValue(value,key)` helper. Keys, class names,
user-query branch, sort keys, limits/skip, SDK calls and opaque fields retain
their original behavior. The existing `metadata_utf8_probe` opt-in still gates
that helper. Exact original argument/constructor shape and expected site counts
are checked, including refusal of duplicate or incompatible injection.

`Managed/LastFmPatchVerify.cs` compares every unrelated managed method's
instructions, then all 25 native method RVAs and their first 16 entry bytes
between the installed input bridge and staged corrected output. All remain
unchanged. This is an entry comparison, not a full binary match or recovery
credit. Its separate offline test fixture replaces only FindAsync with a null
task so no HTTP request is issued; that fixture is never installed.
`Managed/MetadataQuerySmoke.cs` executes the actual original query builders in
disabled and enabled processes, checking Artist/Title/Genre values, opaque
objectId behavior and original sorting limit/skip. Both pass. The original
skip/limit parameter order is retained; test invocation uses its recorded
parameter names rather than guessing their order.

### Parser validation and remaining limitations

The original native elJSON.parse (RVA 0x1D0B00), getValueAsString (0x1D0E90)
and destructor (0x1D0AD0) accept raw UTF-8 `artist/name` and ASCII correctly.
The raw response copy therefore supports the observed Last.fm body. A separate
fixture reveals that original escaped `BUCK\\u2010TICK` instead yields
`BUCKTICK`: that parser drops this escape. The copy operation preserves those
input bytes, but this scoped correction does not repair the separate escape
parser limitation or claim every JSON Unicode representation now works.
Existing stored damaged records and any additional source/cached metadata
paths are not rewritten or heuristically repaired.

The native parser/guard, original CLR copy, query-builder and existing font
ownership/alignment/highlight/UI-scale regressions all pass. Local files now
contain Notify SHA-256
`351cc6fac5a4d9d1520cc71fafbd03d834e1b4e145e67fc5a8317577ab5c9c74` and bridge
`c8a925c292a0776fcaccf961f0add2857359778ae3282cfef0d606d2188985b2`, with prior
files/config backed up. The signed helper, SDK transport, original Game.exe,
vfs.dat/vfs.zip, existing font volumes and user records are unchanged. The
next fresh in-game run must confirm the corrected query/upload/display and
that the startup edit reaches the actual JIT path. No Windows live test or
release publication is claimed.

## Live correction confirmation and remaining range-query path

The user confirmed that the artist now displays correctly. The next trace
shows the startup UTF-8 copy installation and native Now Playing, leaderboard
and pause text retaining U+2010. Native bridge-in Artist and the new Track
request both contain the correct UTF-8 bytes. An earlier cached `BUCK?TICK`
label remains separately visible in the log, consistent with deliberately
not rewriting old records. Private snapshot hashes: engine
`f96d74ac0f6dbaad5e9de60d5b737b5d95b1fa88e6e2e3adb9e4f657e0104a84`, Unicode
`88806092dacb5f0e3e17b18337e90dc969e5a1f7c97b31335363ea798e75cf29`, metadata
`a789fa7a31e1f1c4599b0f1247adeb5c066f9befad6a4da4cf9549a46775487f`.

That run also exposed an incomplete lookup correction: its Artist predicate
still misdecodes UTF-8 as Windows-1252, even though upload and display are
correct. Original gaNewTrackScreen.startParseCheck calls
elNetworkParse.findAllObjectsWhereRange; correcting only F0/F4 equality
builders did not cover the range-query overloads. This supersedes any
implication above that live artist lookup was already established.

`--metadata-range-query-utf8` extends the same field-scoped DecodeValue call
to the two ParseObject equality sites in original token 0x060000F2 and one
in 0x060000F3 (both findAllObjectsTypeWhereRange). Numeric-only F1, ParseUser
branches, range field names and numeric operations remain unchanged. The
original range contract uses total width: the upper endpoint is
`value + range * 0.5` and the lower is `value - range * 0.5`, via double
intermediates and final Single boxing. F2 IL_0154..019E and corresponding F3
blocks establish this; the test calls use the original full-width meaning.

The extended offline original-method fixture passes in enabled and disabled
processes for both overloads: Artist U+2010, optional Title Unicode, duration
bounds 97.5..102.5 for value 100/range 5, and BPM bounds 119..121 for value
120/range 2. No HTTP calls occur. All unrelated managed instruction bodies,
25 native RVAs and their first 16 entry bytes match the prior installed bridge;
only the two additional range-query method bodies intentionally change.
The final bridge is installed locally with its predecessor backed up. Notify,
font assets, original executable/archives, SDK transport and user records are
unchanged. A restart/repeated lookup still needs live validation; no claim of
complete lookup matching or original-server record repair is made yet.

## Final live encoding validation

After the range-query correction and restart, the user's next load retains
`BUCK\u2010TICK` in the observed native source bytes, Track query predicate,
Track request, server Track response and native Now Playing/leaderboard/pause
layout. The metadata observer's latest process-start marker separates this
result from earlier append-only corrupted events. An independent assertion
decodes the source bytes and escaped JSON trace fields and compares all these
current stages; it passes. No legacy-codec event appears in this fresh native
trace, consistent with the scoped response copy reaching the actual JIT flow.
The user had already confirmed the proper visible artist after the copy fix;
this additionally establishes the previously missing lookup encoding path.

A subsequent cached track retains the same opaque song ID observed in its
prior run. The two final score queries refer to different songs, so this log
does not establish two uncached server lookups of the same track or a general
no-duplicates guarantee. It establishes correct encoding end-to-end and cached
ID continuity. No stored metadata rewrite or cache deletion was needed or done.
Old cached placeholder labels can still appear, as expected from that scope.

Private snapshot hashes: metadata
`278e891686e0dc9240dc9a5bfbfa6de67e3ac017f4f1602c4bba628243791b8e`, Unicode
`9225764c4ff46d181c60d1fb6b9ce947fb27e913de0da0e9f6ff516a5fcb750d`, engine
`677ccdb279be9873e2da9f18156f6a4edcd2fb3629a6aa8e7213ae31ac223d57`.
Final installed bridge SHA-256 is
`096b323a65fd70487335a32b7d900e239b60828d5a48850eefa3bbf1db2ea280`;
Notify remains `351cc6fac5a4d9d1520cc71fafbd03d834e1b4e145e67fc5a8317577ab5c9c74`.
All source snapshots stay private/ignored. Windows live coverage and the
separate original JSON escape parser limitation remain open. No release or
matching-byte reconstruction credit is claimed.
