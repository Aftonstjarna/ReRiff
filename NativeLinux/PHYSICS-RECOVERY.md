# Original vehicle behavior ports

These are selected behavior ports, not a byte-matching reconstruction of the
original executable. The required source-rebuild baseline remains incomplete;
see [matching reconstruction status](../Decomp/README.md). Zero observed error
in the fixtures below is not a whole-program equivalence claim.

`vehicle_behavior.hpp` ports the original pace and vehicle-control calculations
into C++. It consumes caller-supplied body, track, wheel-contact and player state.
It supplies target velocities, drive impulses and control/friction outputs for the
eventual original rigid-body integration. It does not advance a replacement car
simulation. `--drive` remains unavailable until that integration is recovered.

## Implemented behavior

| C++ entry point | Original method | Preserved behavior |
| --- | --- | --- |
| `autoAccelBrake` | `gaPlayerLocal.calcAutoAccelBrake` | Separate acceleration/braking response ranges, speed feed-forward, active-race clamp and ramp braking suppression |
| `manualSpeedOffset` | Expression in `gaPlayerLocal.tickInput` | Auto/manual speed setting, pedal clamps, manual drive offset and lower limit |
| `Spring::tick`, `matchingFriction` | `gaSpring.tick`, `gaSpring.getMatchingFriction` | Original 30-row friction table, extrapolation, intermediate float stores and state integration |
| `targetSpeed` | `gaPlayerLocal.tickTargetSpeed` | Pace error, spring state, dead zone, top-speed clamp, override, gravity-relative speed correction and whether to call the body velocity setter |
| `tickInputs` | `gaVehiclePhys.phys_tickInputs` | Steering reversal/onset, pedal smoothing, drift button, timer and latch behavior, countersteering exit, release modes and peak drift state |
| `steeringAngle` | `gaVehiclePhys.phys_calcSteerAngle` | Velocity-relative steering bounds, low-speed heading fallback, drift bias, self-steering and exit angle |
| `grip` | `gaVehiclePhys.phys_calcGrip` | Slip-cone response and angle reflection past 90 degrees |
| `laneCount` | `gaNodeTracker.getNumLanes_DAT` | Interpolated asymmetric widths and missing-node fallback |
| `lateralFriction` | `gaVehiclePhys.phys_calcLatFriction` | Surface coefficient, throttle lift-off, lane-dependent drift blend and braking blend |
| `forwardFriction` | `gaVehiclePhys.phys_calcForwardFriction` | Parked response, braking/drift resistance and sand-contact resistance |
| `stickToTrack` | `gaVehiclePhys.shouldStickToTrack` | Ramp state, forced-adhesion flag, release flag and track-angle release |
| `tickDrive` | `gaVehiclePhys.phys_tickDrive` | Gearbox force, drive direction, cached speed, manual/boost impulse and prevention of reversing through a negative offset |
| `skidState` | `gaVehiclePhys.phys_calcSkid` | Lateral smoke intensity, forward/reverse burnout, pedal and drift braking skid thresholds, contact and neutral-gear suppression |
| `skidSpeedup` | `gaVehiclePhys.phys_applySkidSpeedup` | Opposing lateral response, heading-dependent blend, two-contact minimum, acceleration cap and center-force output |
| `yawDamping` | `gaVehiclePhys.phys_applyYawDamping` | Tire torque cancellation, lateral response latch, lane-dependent drift angle limits, steering bias, angular damping and body setter boundary |
| `suspension` | Suspension portion of `gaVehiclePhys.phys_applySuspAndTyreForces_HI` | Compression cutoff, adhesion, normal correction, wheel-point velocity, spring load, smoothing and damping force |
| `wheelDriveWeight`, `tireForces` | Force assembly in `gaVehiclePhys.phys_applySuspAndTyreForces_HI` | Front/rear drive allocation, slip grip, load equalisation, lateral force limiting, slope response and combined wheel force |

Manual speed changes an extra drive impulse. It does not bypass the original
pace-to-pedals controller. This was verified by invoking the complete original
`tickInput` and examining its manual offset and copied vehicle inputs. Camera
mode suppression, score/boost dispatch and other player/UI state in that method
are not claimed as ports of `manualSpeedOffset`.

The native `targetSpeed` result includes `applyVelocity`: the body boundary must
use its velocity setter when true. The original setter also wakes a sleeping
body and resets its deactivation timer; the reference checks this side effect.
The API preserves the additional offset read from the holder's second component
at byte 268 as an explicit input, without assigning it guessed gameplay meaning.

## Evidence and parameter mappings

The source is the local `analysis/native-port/decompiled/Module.cs`, recovered
from Game.exe SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
`reference/vehicle-behavior-methods.json` records original MethodDef tokens,
RVAs and implementation kinds. `reference/vehicle-body-access.asm` preserves
the native disassembly used to construct the reference boundary:

- Body pointer: `elPhysBody + 0x78`.
- Linear velocity: body `+ 0x130`; angular velocity: body `+ 0x140`.
- Activation state: body `+ 0xD8`; deactivation timer: body `+ 0xDC`.
- `getVel` RVA `0x3B0B0`; `setVel` RVA `0x3B0E0`.

`reference/vehicle-angular-access.asm` records the original angular getter and
setter (`0x3B160`, `0x3B190`). The yaw capture calls these unchanged native
methods and checks the angular velocity, activation state and deactivation
timer. The C++ result signals whether to call the body setter; it does not
itself implement body activation or the solver.

The reference runner reads the installed `scripts/config/vehicles_PC.cfg`
`carParams.base` preset. Offsets were checked against the original tunable
registration, not inferred from the order of configuration keys:

| Parameter | Original parameter byte offset |
| --- | ---: |
| Grip / slip cone | 64 / 68 |
| Dirt / sand / grass / tarmac / ice / wet grip | 72 / 76 / 80 / 84 / 88 / 92 |
| Two-lane / three-lane drift grip | 96 / 100 |
| Brake / lift-off grip | 112 / 116 |
| Drift / brake / sand slowing | 208 / 212 / 252 |
| Steering onset | 16 |
| Drift delay / hold / onset / release | 164 / 168 / 172 / 176 |
| Skid speedup | 300 |
| Two-lane / three-lane drift angle limits | 104 / 108 |
| Suspension stiffness / damping | 124 / 128 |
| Derived load-smoothing coefficient | 408 |

The count used by `phys_calcForwardFriction` at vehicle byte 956 weights
`SandSlow`, not `GroundSlowInc`. Surface dispatch is dirt=1, sand=9, wet=7,
ice=6, grass=13, otherwise tarmac. The adhesion-release byte at 1320 is computed
from the original track-tangent angle test; it is not a generic airborne flag.

## Independent comparison

`reference/OriginalVehicleCapture.cs` extends the existing offline CLR capture
host. The executable's method bodies are unchanged. It supplies scratch object
state to the original methods, uses the actual native velocity getters/setter,
and records outgoing drive impulses and skid forces at the body's virtual-call boundary.
Input-device lookup is an inert boundary callback; it supplies no control math.
No Bullet world or substitute physics simulation runs in the capture harness.

The checked-in numerical fixture has 21,447 cases and 126,703 compared outputs.
All compared outputs matched numerically exactly on this Linux build. Coverage
includes temporal spring/drift sequences, banked gravity, rotated headings,
manual pedal limits, inactive pace markers, zero pace error, velocity corrections,
body wake state, all friction surface branches, and competing adhesion flags.
Base-preset values and deliberately distinct test coefficients are both tested.

The skid additions cover 8,190 state cases and 1,296 force cases. State cases
straddle the original reverse-speed, pedal and drift thresholds, reset stale
output fields, and include neutral gear and zero wheel contacts. Force cases
cover banked right vectors, low-speed heading fallback, opposing/reinforcing
lateral response, partial angle blending, the 60-unit acceleration cap and
the distinction between no body call and an intentional zero-force call.
The force callback verifies that the application point is the body center.
The source vector at vehicle byte 392 remains an explicit `response` input;
its upstream calculation has not yet been ported. No smoke renderer or body
integration is implied by these numerical ports.

Yaw damping adds 2,880 cases and 31,680 output comparisons, including sequential
latch updates, zero and threshold suspension travel, flat/banked/sideways body
orientations, both signs of angular velocity at and beyond the 6-unit threshold,
independently disabled angle limits, lane interpolation and clamped track angles.
The harness captures outgoing torque without supplying torque calculations.
It also checks that the original angular setter wakes a sleeping body even when
the resulting velocity is unchanged. Damping fields at vehicle bytes 1116/1120
remain explicit inputs; their upstream lifecycle is not claimed as recovered.

`make test` runs this comparison without Wine or the original executable, using
the numerical fixture and its SHA-256 provenance file. The existing installed
asset tests still require the game files. Comparison requires exact numeric
equality except steering-angle outputs, whose portability allowance is 1e-6;
the observed error for those outputs is also zero. Signed-zero identity is not
claimed because the CLR text capture normalizes its representation.

```sh
make -C NativeLinux test-vehicle-behavior

# Regenerate through the unchanged original methods in the existing CLR prefix:
build/native-linux/reriff-reference capture --song 'Rolling On' \
  --output analysis/native-port/vehicle-behavior-reference
build/native-linux/test-vehicle-behavior \
  analysis/native-port/vehicle-behavior-reference \
  analysis/native-port/vehicle-behavior-reference/behavior-parity.json
```

The capture also rechecks the original song nodes/segments and verifies that the
installed Game.exe hash is unchanged. Fixture hashes and both capture-host
source hashes are recorded in `behavior-provenance.json`.

## Suspension force recovery

`vehicle_suspension.hpp` ports the suspension calculation from the original
combined tire method. The 576 reference cases invoke that complete method
unchanged, with one wheel and zero drive, lateral grip, slope aid and skid boost.
The observed wheel force is forwarded to the original native force accumulator;
the test checks load, smoothed load, wheel force, total force and total torque.
All 6,912 comparisons match exactly, including the original gradient constant.

Stiffness and damping come from the installed base vehicle configuration, not
invented tuning. Tests cover positive, zero and negative compression, both sides
of the adhesion cutoff, rising/falling wheel velocity, angular velocity, banked
body orientation, tilted contact normals and normal replacement. Smoothing is
tested disabled and at two nonzero coefficients, carrying load state across each
compression sequence.

The C++ function accepts a world-relative wheel point. The reference records
that point after the original matrix transform; this test does not claim to
validate a C++ local-to-world transform. Wheel ray queries and altered drift
moment arms remain unported. These isolated fixtures do not demonstrate a
complete drivable car.

## Combined tire forces

`vehicle_tires.hpp` now implements drive-weight selection and the remaining
per-wheel force assembly. The caller supplies the wheel's steered frame and the
surface-friction result from `lateralFriction`. Allocation uses the original,
uncorrected contact normal and the original adhesion compression test, then
divides by the greater of summed weights and 0.5. Lateral forces retain the
unnormalized projected side axis, load equalisation, slip and tilt response,
traction blend, 50-unit cap and per-step lateral-speed limit. The port also
preserves slope-dependent grip loss, drive direction correction and climb aid.

The new reference group executes the complete original method on four wheels
in 288 scenarios (6,912 output comparisons). It tests each wheel's force and
smoothed load, plus accumulated body force/torque and total suspension loads.
Mixed compression, opposite-facing contact normals, front/all/rear drive bias,
zero to four reported contacts, banked and sloped frames, positive/negative drive,
grip reduction and traction/load overrides all match exactly.

These force-isolation cases deliberately vary grip while setting surface
multipliers to one, steering to zero, drift inertia changes to zero and skid
boost to zero. The existing separate friction, steering, yaw and skid tests
still run. This group does not yet validate nonzero steering transforms,
drift-adjusted force positions, or complete frame-to-frame integration. The
caller must accumulate `absoluteLateralSpeed * travel` with the original double
intermediate before storing to float for the final skid calculation.

## Remaining integration

### Native force boundary recovered

`vehicle_body.hpp` now ports the original native force-at-position and torque
accumulation calls, plus linear/angular impulse-to-acceleration conversions.
These preserve per-axis factors, the world inverse-inertia matrix, single-float
SSE arithmetic and activation behavior. Applying a force wakes an eligible body,
even for zero force; applying torque alone does not. Static/kinematic flags
suppress wake changes, while activation states 4/5 preserve their state but
still clear the deactivation timer when the flags allow waking.

The original constructor installs the vtable at RVA `0x5F982C`. Slots +32/+36
resolve to force-at-position RVA `0x3B660` and torque RVA `0x3BB50`.
`reference/vehicle-force-access.asm` records the table and native routines.
The CLR capture invokes these original addresses in the loaded, hash-checked
image; no replacement accumulation formulas produce the expected values.
It reads accumulated force/torque through original getters and calls both
original acceleration converters. The 2,304 cases compare 32,256 outputs with
zero numerical error, including successive force application, nonzero initial
totals, center/off-center points, blocked axes, zero inverse mass, off-diagonal
inertia and activation states 1/2/4/5 with all low-two-bit flag combinations.

Body offsets: inverse inertia rows `+0x100/+0x110/+0x120`, inverse mass `+0x150`,
linear factors `+0x154`, angular factors `+0x218`, total force `+0x194`, total
torque `+0x1A4`. No rigid-body world is constructed by this isolated validation.

Steered wheel transforms, drift-adjusted force positions, whole-frame force
orchestration, the gearbox state machine, gravity and
braking forces, wheel-ray queries, body contact response, track-tangent update,
and rigid-body solver are still required before interactive driving can resume.
Several caller-state fields consumed here come from those routines. Original
boost awards, full race lifecycle, menus and audio lifecycle integration also
remain separate work. The next boundary is the per-wheel force/torque output of
`phys_applySuspAndTyreForces_HI`, followed by the actual body/contact integration.
