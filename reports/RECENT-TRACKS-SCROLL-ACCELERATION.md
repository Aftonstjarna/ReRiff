# Recent Tracks scroll acceleration (2026-11-18)

## Original evidence

Reference: depot 351991/build 1687655 Game.exe SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Read-only dnlib inspection; recovery project string/symbol references consulted.

`gaFileSelectInterface.tickInput`, token 06001055, RVA 2CFC00:
- Held Up/Down use masks 64/128 from device virtual slot 5 and button slot 0.
- Direction latch is +376; repeat countdown +380; repeat interval +384.
- IL 024F–02B3 decreases interval by `dt * 0.07000000029802322`, clamps
  to 0.02 seconds, and restores 0.25 seconds when released. Constants are
  `ldc.r8` at 0268, `ldc.r4` at 0286/028D, and `ldc.r4` at 02AD.
- Native press/repeat signals can also invoke `changeSelection` early.

`gaRecentTracksMenu.tickInput`, token 060015A8, RVA 3621F0, checks native
press/repeat events without that countdown/acceleration. It marks the adjacent
row's +372 flag with bit 4 before navigation. `setupRecentTracksList`
(060015A0, RVA 36394C), IL 0AAF–0AEC, sets mode +152 to 1 and Up/Down IDs at
+156/+160, clamped at the list boundaries. Rows are in the vector at +1248.
The native screen (+184) holds its focus item at +20.

## Intentional ReRiff extension

Extend the existing guarded Recent Tracks tick hook. After original tick, use
its current row focus and held direction to supply missing accelerated steps
through the row's original virtual direction handler (slot 28). Never call
track activation or rebuild the list. Preserve native sorting and reel ownership.

The adapter follows the picker curve (250 ms minus 0.07 times held milliseconds,
minimum 20 ms), emits at most one additional step per frame, and emits none on
a frame where original navigation already changed focus. Neutral/opposed input,
direction changes, sort changes, row-vector replacement, inactive menu/window,
overlays and long gaps reset timing. List ends clamp; no wraparound is added.
This is an intentional adaptation, not a byte-matching recovery of the picker.

Automated validation covers timing, rollover, original-step deduplication,
resets, boundaries, input exclusion and native direction dispatch. Live held
keyboard/controller navigation remains to be tested.

## Live-test correction: native menu lifecycle

The first live test did not show acceleration. The initial adapter incorrectly
required zero at menu +408, copied from a different menu's transition guard.
That is inside Recent Tracks' embedded `gaTabGroup` (+248 through +1199), not
its active-menu state. Initial mocked tests wrongly made that region zero.

Verified original `gaMenu.tick` (06001128, RVA 2E21FC) switches on +28; phase
1 invokes active ticking, while 0/2/3 are transition/lifecycle paths. The
Recent Tracks constructor (06001597, RVA 362D74, IL 0011–001D) constructs
its tab group at +248. `elUIScreen.getFocusItem` (06000334, RVA 1F81CC,
IL 0002–000A) confirms the focus pointer at screen +20.

The adapter now requires phase 1 at +28 and ignores +408. An overlay blocks
acceleration only if it is a different menu. Regression fixtures use nonzero
tab data and test active, transitioning, self-overlay and nested-overlay cases.
Bounded logs record blocked/ready state and accelerated dispatch outcomes.
A new live test is still required; the previous automated pass did not establish
working live acceleration.

## Keep the viewport with selection

The next live test reported selection outrunning the reel animation.
`gaRecentTracksMenu.updateFocus` (060015A4, RVA 36204C, IL 0084–00AB)
queues `gaMenuReel.moveToSelection` with a null duration when the focused row
is outside the viewport. The reel pointer is menu +1244; its requested first
row is +456 and pending selection movement state is +436 == 4.

`gaMenuReel.moveToSelection` (0600113E, RVA 2E2948) uses fixed speed 250 for
null duration (IL 0161–016D). A non-null pointer to zero instead takes its
immediate native branch (IL 0045–015F): set the clamped position, clear state
+436 and invoke the parent's onReelSettled callback. That callback restores
native row visibility/input after movement. No extra reel tick is needed.

ReRiff now completes pending focus-follow movements using this existing zero
-duration path after the original menu tick. It uses the native queued target,
checks the reel's original vtable and target range, and leaves other movement
states (including dragging) alone. This intentionally changes focus-follow
animation timing only; it does not change selection or native reel ownership.
Regression cases cover immediate movement, settled no-op, other-state no-op,
invalid targets and inactive-window exclusion. Live appearance remains to test.
