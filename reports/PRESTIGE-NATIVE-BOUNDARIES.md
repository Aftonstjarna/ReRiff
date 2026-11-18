# Prestige: native achievement, save and XP boundaries

Research date: 2026-11-13. Original reference:
`/var/home/afton/Applications/SteamDepotDownloader/depots/351991/1687655/Game.exe`.
SHA-256 checked directly:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Evidence is the original managed CIL surveys and PE metadata under
`analysis/native-port/il/` and `decompiled/Module.cs`. Consulted recovery
`Decomp/native/STARTUP.md`, `save_device_io.S`, `save_device_open.S` and
`save_device_set_name.S`. This is ReRiff integration research; no matching-byte
credit or complete original-source reconstruction is claimed.

## Local achievements are not the Steam achievement ledger

`gaDriverProfileMenu.setupAchievementsList`, token `0x060013EA`, CIL RVA
`0x325BBC`, loops to 39 both when preparing achievement layout (body survey
+0x16F) and when iterating rendered entries (+0x6E9). **Correction:** this loop explicitly skips slot 18 at body +0x1B3..+0x1B6,
so only 38 achievements are rendered despite the 39 serialized storage flags.
The earlier interpretation that all 39 were required for displayed completion
was incorrect; see the eligibility correction below. Do not substitute the 28-element
`s_ingameAchievements` array: `gaAchievements.setAchieved` uses it to decide
between queuing an achievement popup and immediately awarding bonus XP.

`gaAchievements.setAchieved`, `0x06000E80` / `0x2B4B38`, updates the progression
flag, the manager's local byte and the unlock manager, and can dispatch a
network achievement transaction. Clearing flags by repeatedly calling this
function is not proven safe for preserving Steam achievements.
`gaAchievements.reset`, `0x06000E82` / `0x2B4A88`, zeroes its first 39 bytes;
its body contains no platform call. `gaSaveGameChunkProgression.reset`,
`0x06001254` / `0x2FC230`, clears the local progression fields; it is separate
from resetting the complete save or issuing a platform achievement operation.
No live reset has been performed by this investigation.

## Cloud and startup are part of the save boundary

`gaGame` startup creates UI, online services, achievements and save state,
then creates paint/unlock/driver-level managers and calls `gaSaveGame.load`.
The survey's save creation call is +0x1BE; driver level creation is +0x1DD and
save load is +0x1E7. This establishes ordering inside that method, not the
runtime timing of ReRiff's endpoint selection relative to those calls.

`gaSaveGame.load`, `0x060011EB` / `0x2F6138`, calls `enableCloud(s_useCloud)`,
then `load2`, then disables cloud access. `save`, `0x060011ED` / `0x2F6160`,
uses the same wrapper pattern. Original `s_useCloud` is at image RVA `0x7ED7BC`;
it is a configured native flag, not proof of current platform cloud state.
Do not infer isolation solely from copying a local save.bin or replacing a
path after the original game has loaded.

## Permanent XP and cycle XP need distinct consumers

`gaDriverLevel.addPlayerXP`, `0x06001038` / `0x2CBEA4`, updates progression XP,
checks level achievements and can award level-up coins. The level-achievement
check, `0x06001039` / `0x2CBDAC`, derives current level through the original
`getDriverProfileLevel`. Leaving permanent XP in that progression field after
a wipe would therefore re-earn level achievements and bypass a fresh cycle.
Preserve the original threshold calculation but separate its progression
consumers from permanent profile display. No replacement XP curve is justified.

`gaVehiclePaintManager.getPackPrice`, `0x0600138D` / `0x3196C8`, reads the pack
price field from the native per-car pack array. Price display and spending must
use one adjusted value; adding only a discounted label would be incorrect.

## Existing server-driven reward configuration

`gaOnlineTuneables.searchParse`, `0x0600114B` / `0x2E68F0`, queries the native
Parse `Configuration` object. `unpackParseData`, `0x0600114D` / `0x2E6E68`,
reads fields including `OnTrackCoinValue`, `ShareScoreCoinReward` and
`TrackCreatorCoinReward`. Extend this existing configuration transport for
suitable base rewards rather than replacing those consumers with a separate
reward system. Prestige identity, independent XP and balance provenance are
new ReRiff state and still require a versioned extension. These configuration
fields alone do not establish server-authoritative progression verification.

## Read-only prototype and limits

`ExperimentalMenu/progression_probe.h` reads fields only after the existing
full Game.exe hash/runtime guards pass, requires explicit tracing and diagnostic
logging, and samples using bounded `ReadProcessMemory` calls. The native globals
are save pointer RVA `0x7ED7B8` and driver-level pointer RVA `0x7E9428`.
Save +16 points to settings, +32 to progression. Progression +12 holds 39 local
flags, +280 XP and +284 coins. Settings +324 is chosen car; +236+4*car stores
chosen pack. The driver's first native vector contains XP thresholds.

Synthetic checks cover field extraction, unchanged source bytes, invalid
pointers, car bounds, insufficient output space and threshold validation. Live
observation has now confirmed valid selected car/paint fields, the 39 local
achievement flags and a 41-entry native XP vector beginning at zero and ending
at 1,000,000,000 XP. Original `getDriverProfileLevel` clamps to the last table
entry and returns `count-1`, establishing level 40 for this loaded table.
This confirms the observed table, not universal thresholds for every mod.

`addPlayerExperience` (`0x06001262`, `0x2FC8F8`) uses a separate 2,000,000,000
storage clamp after an unchecked signed addition. That is distinct from the
1,000,000,000 level-display threshold; future tuned rewards must bound addition
before calling the original path rather than relying on post-addition clamping.

The expanded probe also records whether save/driver objects exist synchronously
at endpoint initialization, bounded native save-file slot names and whether the
save-device open slot still points to the recovered local implementation.
A live Proton startup subsequently confirmed that endpoint initialization
runs with save and driver pointers null while the local save device already
exists. The later observation recorded one native file slot, `save.bin`, and
an open slot pointing to original RVA `0x135B10` outside cloud operations.
This is an observed integration point for this backend, not proof that the
cloud-enabled load/save interval uses that same device. A subsequent read-only
probe recorded the cloud getter at RVA `0x16FC50`, matching the original bytes
`a1 30 cd bf 00 c3`: it returns the singleton at VA `0xBFCD30`.

It does not intercept file operations or toggle cloud access. Save-domain
isolation is still required before enabling a
prestige action. Unknown progression counters, ownership granularity, Steam
Cloud/account routing and authenticated server reward verification remain open.

## Verified native device switching and account getter

Direct `objdump -d -Mintel` comparisons against the hash above establish:

- Cloud enable VA `0x56CA20` through `0x56CA59` (58 bytes, `ret 4`) stores
  the cloud object's secondary interface (`this+4`) at active-device global
  VA `0xBFCB4C` when enabled. When disabled it restores the original local-device
  pointer from VA `0xBFCD08`. Original entry bytes are `55 8b ec 33 c0`.
- Cloud object's primary table is VA `0xA00EB4`; the file-device secondary table
  is VA `0xA00EC8`. The original local file-device table is VA `0x9FFE2C`.
  Its slots are exists, create, open, close, size, delete, read, write.
  Consulted recovered `save_device_open.S` and `save_device_io.S` for the
  original open modes and stack argument order. Mode 1 is read, modes 2/3
  are update. Read arguments after native `this` are offset, buffer, byte count,
  out-read-count; write arguments are offset, buffer, byte count. The local
  stdio call setup and cloud read's memcpy at VA `0x56CCC0` agree on that order.
  This was not inferred from generic Win32 file modes.
- Network user singleton is stored at VA `0xBFB610`. Its secondary interface
  at `object+4` uses vtable VA `0x999D50`; slot +124 points to VA `0x4150E0`.
  Exact getter bytes `8b 81 d4 01 00 00 c3` read `[secondary_this+0x1D4]`,
  equivalent to original object +`0x1D8`. This is the numeric account value
  used by the original online-ID path, not an authenticated server identity.
- `gaOnline.getUniqueOnlineID`, token `0x060012F3`, RVA `0x30A774`, uses this
  secondary getter and formats `%u` into its own instance buffer. Calling it
  with a null `this` before gaOnline initialization is invalid; the prototype
  instead reads the verified field and refuses zero.

The opt-in ReRiff isolation prototype intentionally changes five vtable slots:
local exists/create/open/delete redirect only `save.bin`, and cloud enable stays
disabled during isolated sessions. First-import reads use the configured native
device before interception and write a separate temporary local file through
the original create/open/write/close routines, followed by a no-replace rename.
The configured cloud path fails closed if its backend is unavailable; it does
not silently import a stale local copy. Synthetic tests verify routing, ABI
argument forwarding, exact copy, no replacement, partial-read failure and
configured cloud/local source choice. The first live test loaded clean
progression; a read-only comparison confirmed the original Steam save remained
byte-identical. This is a failed live isolation test, not proof of an accepted
import. Import hashes and bounded file-operation diagnostics were added to
distinguish source/copy failures from native load behavior. No reset ran.
These hooks and new profile filenames are intentional patches, not original
reconstruction, and this submission requests no matching-byte credit.

## Correction: cloud source is not initialized at endpoint selection

The second diagnostic run refused the original-source open before save
construction. Inspection of the original PE section layout places RVA
`0x7ED7BC` in the zero-filled tail of `.data`: its initial value is zero.
`gaSaveGame.{ctor}`, token `0x060011E3`, RVA `0x2FBA98`, writes one to
`s_useCloud` at body offset +`0xFC`. The earlier prototype therefore selected
local storage using an uninitialized value; the first version's absent-source
fallback allowed a clean profile. The later safeguard correctly refused it.
The log did not establish an unavailable Steam backend; that interpretation
was too broad. The corrected prototype defers source selection, profile lock
and import until the first exact `save.bin` access, after save construction,
and bypasses the installed name hooks when reading an original local source.
Unrelated cache-file accesses do not trigger import.

A subsequent live run logged cloud source selection, an exact 704-byte import,
successful mode-1 native open and an unchanged local root. The source, new
isolated file and original Steam file matched byte-for-byte by SHA-256. Native
XP, coins, equipped car/pack and achievement flags matched the earlier observer;
the user confirmed existing progression in the menus. This validates initial
import for this tested configuration. Isolated writes and restart persistence
remain unverified; no prestige reset has run.

## Retaining equipped ownership without replaying unlock rewards

`gaSaveGameChunkProgression.setCarUnlocked`, token `0x0600125B`, RVA
`0x2FC7BC`, calls the original packed car setter only when the value differs
and marks progression +4 dirty. `setPackUnlocked`, token `0x0600125E`, RVA
`0x2FC818`, similarly updates a car/pack flag and dirties the chunk. Neither
body calls the achievement manager. `gaSaveGameChunkSettings` already owns the
selected car and per-car selected pack; resetting only progression leaves
those settings separate from ownership.

Do not substitute the higher-level `gaUnlockManager.unlockPack`, token
`0x060012D8`, RVA `0x308A9C`, when restoring retained ownership: it counts
unlocked packs and can call `gaAchievements.setAchieved` for IDs 20–23.
`unlockCar`, token `0x060012D2`, RVA `0x308F4C`, can also unlock the car's free
packs through that higher-level path. Native progression ownership is per
car/pack, not an independently verified per-paint purchase flag. A retained
equipped paint must therefore account for its pack's ownership granularity.
These conclusions come from original CIL body comparison, not a live reset.

## Persistence investigation: observe native save gating first

The initial garage-change/restart test reported a reverted selection. Across
those sessions the name hooks recorded mode-1 reads but no mode-3 update open,
create or delete call; the isolated file retained its imported hash. This does
not establish a failed native write, because no write attempt was observed.

Original `gaSaveGame.save2`, token `0x060011EE`, RVA `0x2F5CCC`, returns success
without I/O when save object byte +13 is nonzero. Later in its body it requires
both chunk +5 and dirty byte +4 before opening a file in mode 3. Original
`gaLoadSaveScreen.doSave`, token `0x06001372`, RVA `0x314680`, checks that same
save +13 byte before calling the original save wrapper. Original settings
setters `setChosenCar` (`0x0600127C`, RVA `0x2FD39C`) and
`setChosenPackIndexForCar` (`0x0600127B`, RVA `0x2FD37C`) set the settings dirty
byte when their selection changes; they do not themselves write the file.

The read-only observer now includes save +13, settings +4/+5 and progression
+4/+5 and samples for ten minutes. A native race/save transition is the next
test rather than adding a separate save-on-exit implementation. No arbitrary
dirty/enabled flag writes or forced save calls have been introduced.

The subsequent native race-completion trace observed settings/progression
dirty bytes becoming set, updated XP/coin/selection values, a successful mode-3
open of the isolated file, and dirty bytes clearing. A restart then loaded the
updated values, confirmed by the user. The original Steam save hash remained
unchanged. This establishes successful isolated native saving and persistence
for that test; the earlier garage-only test had not established a write failure.

## Close boundary for new ReRiff checkpoint metadata

Original local close VA `0x535C20` through `0x535C45` returns an AL byte, not
void. It calls the fclose import at VA `0x92D470` when the stream pointer at
this+16 is nonzero, clears the mode at this+12 and stream pointer, then returns
true unconditionally (`b0 01 5e c3` at the tail). It does not propagate fclose's
error result. The prototype preserves this original return convention; it must
not claim the return alone proves durable disk storage.

New optional open/write/close wrappers observe only the isolated save filename.
Their metadata callback runs after native close and validates bounded loaded
progression fields before committing a ReRiff save/counter generation. This
checkpoint store and its two snapshot slots are new ReRiff state, separate from
the original serializer and from matching-byte recovery. Synthetic tests cover
interrupted commit, domain/counter validation, stale writers, corruption refusal
and working-copy recovery. Live callback integration remains unverified; no
prestige reset or Steam achievement mutation has been introduced.

## Follow-up: native reset/ownership execution fixture

The later checkpoint integration test completed a native race save, committed
a second save/counter generation and reloaded that same generation after a
full game restart. The manifest and snapshot hashes matched the working save,
and the original Steam save hash stayed unchanged. This supersedes the earlier
live-callback uncertainty above, without establishing reset durability.

The unchanged original Game.exe SHA-256 remains
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Original static pointer fields used by the new adapter are Save
`0x0400126F`, Achievements `0x04000DB1`, and VehiclePaintManager
`0x0400147D`. Their exact metadata names and pointer types are checked by
the test extractor and the live binding code.

`gaAchievements.reset` (`0x06000E82`, RVA `0x2B4A88`) clears only 39 bytes
at the manager's start. It does not clear its pending-popup vector, whose
begin/end fields are read at manager +40/+44 by
`getNextPendingAchievement` (`0x06000E7F`, RVA `0x2B4B00`). ReRiff therefore
waits for normal native popup processing to empty that queue before allowing
a confirmed reset; it does not write vector storage or platform achievements.
This guard is an intentional ReRiff policy, not an original reset feature.

`ClientConfig/Tests/ExtractProgressionFixture.cs` selects the exact original
tokens `0x06001254`, `0x06000E82`, `0x0600124C`–`0x0600124F`,
`0x0600125B`–`0x0600125E` and `0x0600138B` from that original binary.
It preserves their instruction opcodes and operands except remapped internal
method tokens, branch/local references and pointer types normalized to native
ints. External calls and unreviewed operand/type kinds are refused. A separate
save observer is explicitly synthetic. No original binary or extracted fixture
is committed or included in client releases. No byte-matching credit is claimed.

Running the fixture under a 32-bit .NET CLR exercised the real reset and packed
ownership IL across 22 car indices and nine pack indices (198 synthetic cases).
Only the selected car/pack bits remained set; local achievement flags, XP and
coins cleared; settings, high-score sentinel buffers and chunk guard bytes
remained unchanged. Invalid selections, incomplete achievements, skip-save
state and pending popups were refused. These are synthetic pack definitions,
not a claim that every actual car has nine purchasable packs.

The production adapter reuses original `gaSaveGame.save` (`0x060011ED`) for
transaction writes, progression-only reset and the direct ownership setters;
it never calls the complete-save reset or platform-clearing functions. Its
post-reset ownership verification queries all 22 ×9 original packed slots.
The combined checkpoint test uses a stubbed save to verify the controller
boundary; a live original-serializer reset round-trip remains untested, and
no live reset callback is registered. Main-level UI and rewards are also pending.

## Follow-up: self-Profile level display and signed coin boundary

Original `gaDriverProfileMenu.setupDriverLevelWindow` (`0x060013E6`,
RVA `0x324080`) uses an online object's `DriverLevel` when present, or calls
`gaDriverLevel.getDriverProfileLevel` otherwise. It stores the numeric
`elUIItemText` owner at menu +276 (body offsets +0x49D onward); the caption
is a separate object at +272. It uses level/40 for the fill factor.
`setFillbarFactor` is token `0x060013EC`, RVA `0x322998`.

Profile vtable field `0x040014ED` (`??_7gaDriverProfileMenu@@6B@`) has RVA
`0x7A6F94`. Slot 3 contains preferred VA `0x72C53C` (RVA `0x32C53C`),
whose six original bytes are `ff 25 04 70 ba 00`: indirect through preferred
VA `0xBA7004` (fixup RVA `0x7A7004`). The candidate runtime hook verifies
that slot/thunk/fixup before extending the original tick. The existing UI-scale
wrapper subsequently wraps this extension, preserving native scaling order.

Friend/self discriminator field `0x040014FB` is the native friend-ID string
at RVA `0x7A6F2C`. Original `getMenuTitle` (`0x060013D6`, RVA `0x32D1F4`)
tests its length at +16 to choose `PROFILE_Title` or `PROFILE_Friend`. The
candidate display leaves nonempty friend IDs untouched and requires an active
loaded isolated singleplayer save. It modifies UI text/fill only, not XP.

`getDriverProfileLevel` (`0x06001036`, RVA `0x2CBCFC`) accepts an optional
XP pointer; a non-null pointer supplies XP directly, otherwise it reads the
progression singleton. Reusing that explicit argument is sufficient for a
permanent-level display without changing the global helper also used by
level achievements and cycle progression. The extended execution fixture
includes this original body, with the singleton field explicitly remapped to
synthetic storage. Tests compare zero native cycle level against retained
permanent level after a simulated wipe, and reject malformed threshold vectors.

`incTokens` (`0x06001263`, RVA `0x2FC948`) adds a signed delta to progression
+284 before clamping negative balances to zero. Purchases and awards share
this helper. Multiplying every delta would multiply spending as well, and
large positive additions can overflow before the lower-bound clamp. Original
callers include DriverLevel, ResultsTokens, PurchaseMenu, TrackCreatorScreen,
ResultsShare and retention/ad screens. The candidate transform leaves debits
unchanged and bounds awards by `INT_MAX - current` before the original call;
it is tested against the original helper in synthetic memory, not yet installed
as a live reward hook. Client prices/results text and all award sites need
consistent integration; net coin changes are not evidence of reward amounts.

These are intentional display/reward extensions, separate from original byte
reconstruction. New native hook tests cover original tick forwarding,
self/friend gating, callback output bounds and original slot/thunk guards.
No live prestige action or reward transform has been enabled by this stage.

## Follow-up: guarded economy dispatch probe

Native Garage customization obtains pack prices from `getPackPrice` at
`gaCustomise.handleScreenInput` (`0x060013CD`, RVA `0x320F3C`, body +0xE9)
for affordability, then passes that local price to `showPackPurchase`
(`0x06001578`) at body +0x177. Other customization reads feed visible pack
information. Purchase completion negates the cached `ms_tokenCost` field
`0x040016BF` before `incTokens`. This supports a shared getter policy with a
cached purchase quote, rather than independent labels and debit calculations.
No live discounted purchase has been tested.

`gaOnlineTuneables.unpackParseData` (`0x0600114D`, RVA `0x2E6E68`) already
calls native `setPackPrice` (`0x0600138F`, RVA `0x319708`) at body +0xC5D.
Those native definitions can change after initial loading. Applying a discount
repeatedly to that shared table risks compounding or mixing policy with a
later native configuration update. The probe therefore leaves definitions
untouched and observes the shared getter instead.

Exact original getter header bytes at RVA `0x3196C8` are
`03 30 03 00 14 00 00 00 00 00 00 00`; its 20 code bytes are
`04 20 b0 00 00 00 5a 03 1f 0c 5a 02 58 4a 58 1f 24 58 4a 2a`.
The coin header at RVA `0x2FC948` is
`03 30 03 00 38 00 00 00 15 00 00 11`; its 56-byte body is checked against
the full original hex in both `ProgressionEconomy.cs` and
`progression_economy.h`. These remain within the hash-pinned original stated
above; no changed disk executable or original metadata is emitted.

The intentional startup wrappers reuse original cdecl StandAloneSig tokens
`0x1100050D` (void(IntPtr, const signed-char*, int32)) and `0x1100050C`
(void(IntPtr, const signed-char*)). Pointer-shaped parameters carry integer
bits to small native adapters; they are not dereferenced as strings. The
price adapter writes its result into the callee's by-value argument-0 slot,
then IL converts that temporary pointer value back to int32 and returns it.
It does not write paint definitions or the caller's pointer. This fits the
original 20-byte code space with MaxStack intentionally raised from 3 to 4.
The 56-byte coin body retains its local signature and size, with unused bytes
padded by nop after the new return. All original headers/body bytes must match
before either patch is written; callbacks require prior ABI validation.

The preserved bodies contain no metadata operands or exception handlers. The
managed bridge checks their original byte arrays, then re-emits the reviewed
opcodes, constants, local operations and short-branch targets using ILGenerator,
checking each original instruction offset and total code length. New dynamic
method signatures use native ints in place of original pointer types. This is
controlled runtime adaptation, not a matching-byte original reconstruction.

Isolated tests initially rejected DynamicILInfo-based cloning with
NotImplementedException; the first dynamic-thiscall ABI test also failed.
Neither candidate reached the installed game. The cdecl implementation passes
the full CLR calli/native adapter/managed callback/preserved-body route for an
original price, positive coins and a negative debit. Native synthetic-image
tests verify original-byte guards, headers, MaxStack, signature tokens and nop
padding. These tests do not prove startup JIT timing or stack-trace caller
visibility in the actual game. The live candidate remains trace-only and
forwards original prices/deltas; caller classification and a real quote/payment
trace are prerequisites for enabling rewards or discounts.

## Follow-up: separate generation reward and purchased-coin sources

Read-only CIL inspection of the same hash-pinned depot executable identifies
`gaTrackCreatorScreen.enter`, token `0x060015C5`, RVA `0x368314`. At body
+0x42 it loads the native online-tuneables singleton, reads its int32 at +20,
and calls `incTokens` at +0x4B. This is a separate payout from ResultsTokens.
There is no one-time eligibility check inside this enter method: the caller's
transition into this screen must be traced before claiming generation cannot
award twice or that cached tracks bypass it. No generation multiplier is live.

`gaResultsTokens.enter`, token `0x06000752`, RVA `0x22E880`, stores base
race tokens at self+388, accumulates level/other bonuses at +392, snapshots
the prior balance at +396, then passes the sum to `incTokens` at body +0xE7.
Changing only the shared coin addition would leave original result components
unscaled; the display integration must be handled with the payout.

Positive coin deltas are not uniformly earned rewards. For example,
`gaPurchaseMenu.tick`, token `0x06001585`, RVA `0x35CFA8`, credits purchased
coins from self+356 at body +0x168. An unconditional positive-delta prestige
multiplier would also multiply these purchases. Other direct callers include
ResultsShare.giveReward, ResultsTokens.onResume, DriverLevel.addPlayerXP,
AdOverlay.tick and RetentionPopup.handleScreenInput. Each needs an explicit
policy and display/lifecycle review; unknown sources should retain originals.

These are verified original instruction observations, not a matching-byte
reconstruction or proof of live generation lifecycle behavior. The read-only
inspector rejected any executable whose SHA-256 differed from the recorded
original. Local disassembly artifacts remain under ignored build storage.

## Follow-up: generation reward transition and displayed amount

Same original depot SHA-256 and CIL provenance as above.
`gaTrackCreatorScreen.{ctor}` (`0x060015C2`, RVA `0x367FCC`) passes state 63
to the native menu constructor. `gaNewTrackScreen.goRace` (`0x060014E9`,
RVA `0x34B110`) reads self+1272 at body +0xF8 and selects state 63 at
+0x108 only when that byte is nonzero. Otherwise it calls the ordinary
load/save transition to state 12 at +0x115.

The flag is cleared in `gaNewTrackScreen.enter` (`0x060014CC`, RVA
`0x35113C`) at body +0x95..+0x9C. It is set after metadata object commit
in `checkParse` (`0x060014E0`, RVA `0x34F650`, +0x199..+0x1A0),
`tickPlaylistCheck` (`0x060014F4`, RVA `0x34B644`, +0x51E..+0x525), and
`tickPlaylistCheck2` (`0x060014F5`, RVA `0x34C3D4`, +0x46D..+0x474).
These instruction observations establish session/metadata-creation gating,
not a durable per-player lifetime award ledger. Live fresh-versus-cached
comparison is still needed; deletion/recreation behavior remains unverified.

`gaTrackCreatorScreen.initScreens` (`0x060015C7`, RVA `0x36837C`) reads
the same native tuneables+20 at body +0x218..+0x220 and passes it into
the localized NEWTRACK_CreatorLine3 formatting at +0x23E. Therefore payout
and screen text need one consistent quote, without multiplying the shared
tuneables table repeatedly. `tick` (`0x060015C9`, RVA `0x367F50`) waits
for five seconds, fades out and calls the native load/save transition to
state 12 at +0x64. No intentional reward patch has been installed here.

Live validation of the generation transition: on the installed prestige-zero
Singleplayer trace build, generating a new song and reopening the same song
produced one `gaTrackCreatorScreen.enter` coin credit of the configured 100.
The user observed the creator-reward screen only on the first opening. The
resulting native save/checkpoint hashes matched and the original Steam save
was unchanged. No modified reward amount was installed. This verifies that
specific fresh-versus-reopen sequence; it does not prove durable deduplication
after cache deletion, server metadata recreation or a prestige reset.

## Follow-up: original entry and display hook bindings

Same original SHA-256/provenance as above. ResultsTokens vtable RVA 0x7A3F10
slot 2 points to thunk RVA 0x22ECEC with exact bytes `FF 25 90 3F BA 00`
(preferred image base 0x400000; fixup RVA 0x7A3F90). TrackCreatorScreen
vtable RVA 0x7A7B74 slot 2 points to enter thunk RVA 0x368374 with bytes
`FF 25 E0 7B BA 00` (fixup RVA 0x7A7BE0); slot 15 points to initScreens
thunk RVA 0x368970, bytes `FF 25 E4 7B BA 00` (fixup RVA 0x7A7BE4).
Native gaMenu.enter (`0x06001111`, RVA 0x2DA7DC) dispatches its virtual
initScreens via vtable +60 at body +0x23D. ResultsTokens.enter pays before
calling gaMenu.enter at body +0x11C, so adjusted result fields can feed its
original screen construction. The exact native tuneables singleton is field
`0x04001013`, RVA 0x7EB8F0. Both creator reward reads have original bytes
`7E 13 10 00 04 1F 14 58 4A` (absolute RVAs 0x368362 and 0x3685A0).

ReRiff intentionally extends these virtual calls for scoped reward policy;
the thunks and original instructions are evidence/guards, not newly recovered
matching-byte reconstruction. Synthetic native and x86 CLR fixture checks pass;
live entry timing and nonzero-prestige screen appearance remain unverified.

## Correction: 38 displayed achievements, not 39 required flags

Original `gaDriverProfileMenu.setupAchievementsList` (`0x060013EA`, RVA
`0x325BBC`) initializes loop index zero at body +0x1AA..+0x1AB; at +0x1B3
it loads that index, compares it to 18 (+0x1B4) and branches directly to the
loop increment at +0x6D8 when equal (+0x1B6). The upper bound 39 at +0x6DD
therefore traverses 39 storage slots but renders only 38 entries. The old
loop-bound-only survey missed this explicit exclusion.

The test profile's only unset storage flag was exactly index 18. Eligibility
now follows the native displayed list: all slots 0..38 except 18 must equal
one. Both native prompt and managed reset adapter enforce the same list;
reset still clears the original 39-byte manager/progression storage through
the original reset functions. The achievement enum's recovered metadata
exposes only its underlying value field, so this evidence does not provide
a trustworthy player-facing name or purpose for hidden slot 18. Do not
invent a missing achievement or modify Steam/local flags to satisfy it.

## Follow-up: hidden slot award audit and intentional ReRiff reuse

A hash-guarded audit of every CIL call to gaAchievements.setAchieved found
constant IDs at every site: all 0..38 except 18, with zero nonconstant-ID
call sites. Together with the Profile exclusion, this supports hidden slot 18
as an unused award slot in this executable's surveyed managed award paths.
It does not establish behavior of arbitrary external save writers. Original
`gaSaveGameChunkProgression.setAchieved`, token `0x06001259`, RVA `0x2FC770`,
is the save-only setter; the extraction fixture now includes it.

The intentional ReRiff First Prestige marker uses that save-only setter and
mirrors the manager's local byte at +18 after original resets. It avoids
calling gaAchievements.setAchieved and therefore avoids that manager's
unlock/network/platform award path. Prestige count remains an independent
checkpoint-ledger value, not inferred from this boolean flag. Later resets
retain the marker by setting it again before the native save. Profile/icon
display is not yet implemented by these changes; this is not original behavior.

## Correction: Credits is root action 3, not action 2

Original gaOptionsMenu.handleScreenInput, token 0x06001199, RVA 0x2F0794,
has a five-entry switch at body +0xA1 (absolute RVA 0x2F0841). Its bytes
are `45 05 00 00 00 5A 00 00 00 05 00 00 00 6E 00 00 00 46 00 00 00
82 00 00 00`; relative to the switch end +0xBA, the targets are +0x114,
+0xBF, +0x128, +0x100 and +0x13C. Action 3 therefore reaches +0x100,
whose `02 1F 2C` loads self and state 44. gaCredits.{ctor}, token 0x060013B5,
RVA 0x31D660, passes state 44 to gaMenu.{ctor} at body +0x1.

The earlier scaffold's action-2 assumption was incorrect. The live trace
recorded action 3 on Credits activation and no reset. The intentional hook now
uses action 3 with original switch/transition byte guards; an independent
hash-pinned PE regression derives the ID from the constructor and switch.

## Live confirmed reset and original difficulty-star bindings

The first real manual confirmation committed prestige count one with retained
permanent XP and equipped settings, zero cycle XP/coins, and only local flag
18 earned. Native snapshot/checkpoint/working hashes matched; the original
Steam save hash was unchanged. User confirmed Profile and Garage retention.
No live doubled-award or post-reset-restart claim follows from that check.

Original gaTrackSummary.initScreens (0x0600088F, RVA 0x24741C)
difficulty construction (CIL body offsets +0x14EC onward) reads sprite def 89, then selects sprite 89 for filled and 90 for
outline in its five-star loop at +0x15BD..+0x15DD. The native prestige badge
reuses filled sprite 89. Original elUIItemText.getFontSize, metadata token
0x06001742, RVA 0x51110, has bytes `8B 81 CC 01 00 00 8B 40 64 C3`: it
returns the font index at +0x64 of the font pointer at text-item +0x1CC.
Original replaceTextRaw, RVA 0x50930, writes generated text extents at item
+0x30 and +0x34 (instructions at 0x50999..0x509A1). These provide native
font/bounds integration instead of fixed pixel offsets. Profile cleanup slot
16 at vtable RVA 0x7A6F94 points to thunk RVA 0x328AA8 with bytes
`FF 25 00 70 BA 00`, referencing fixup RVA 0x7A7000.

Badge children/centering and star-only first-prestige formatting are intentional
ReRiff UI additions, not original matching-byte reconstruction. The native
Profile text, difficulty sprite, child ownership and cleanup are retained.

## Correction: prestige badge panel ownership and digit ink

The first badge candidate was attached to the level-number text and shown
unconditionally; the live user reported a displaced star persisting across
Profile sections. This is an intentional ReRiff integration defect.

Original gaDriverProfileMenu.updateSelection, token 0x060013EE, RVA 0x322EFC,
uses the selected section at menu +404 to select a panel from menu +384.
Index zero is the summary/level panel. The correction gates the badge to that
section and attaches star/suffix as siblings of the number under that panel,
so they share native panel positioning and cleanup.

The native font pointer at text +0x1CC exposes glyph records through
font +0x28/+0x2C, stride 44. Glyph height at +0x18 times texture height
(font +0x14), and vertical offset +0x20, supply the digit ink rectangle;
the padded text extent is not the visible digit height. These existing
font-inspection fields are reused to size and vertically centre the star.
Native tests cover five sizes, sibling ownership, section hiding, anchor
restoration and original cleanup forwarding. Live visual validation remains
pending. No new matching-byte reconstruction is claimed.
