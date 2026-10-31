# Original vehicle physics recovery

The guessed driving integrator has been removed at the user's request.
There is no replacement bicycle model, guessed acceleration curve, spring-body
integrator, chassis clamp, wall speed penalty, or approximate wheel animation.
The `--drive` and `--drive-demo` options now fail explicitly until the original
physics integration is ported. The song preview remains an inspection animation;
it is not a vehicle simulation.

The original vehicle mesh, textures, song analysis, generated nodes, spline
evaluation, road orientation and audio preview remain available:

```sh
build/native-linux/reriff --song 'Rolling On' --play
```

The next recovery stage is now implemented in `vehicle_behavior.hpp`: original
pace/auto/manual control, drift-input transitions, steering angle, drive impulses,
grip, surface friction, track adhesion, skid state, skid forces and yaw damping.
The native body force boundary is in `vehicle_body.hpp`; suspension forces are
in `vehicle_suspension.hpp`; combined tire forces are in `vehicle_tires.hpp`.
The 21,447 original-executable cases compare 126,703 outputs with zero observed
numerical error. These are reusable behavior ports; steered wheel transforms,
whole-frame force orchestration and body/contact integration remain
pending. See [port scope, source evidence and tests](PHYSICS-RECOVERY.md).

## Located original code

The local ILSpy output is `analysis/native-port/decompiled/Module.cs`.
Its source executable SHA-256 is
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.

| Original routine | Source line | Responsibility |
| --- | ---: | --- |
| gaPlayerLocal.calcAutoAccelBrake | 61470 | Pace target to accelerator/brake |
| gaPlayerLocal.tickInput | 61516 | Manual speed offset and controls |
| gaPlayerLocal.tickTargetSpeed | 61767 | Pace correction and smoothing |
| gaVehiclePhys.phys_tickDrive | 143755 | Apply drive impulse |
| gaVehiclePhys.phys_calcLatFriction | 143810 | Surface, drift and boost friction |
| gaVehiclePhys.phys_calcForwardFriction | 143841 | Forward resistance |
| gaVehiclePhys.phys_tickGravityAndBraking | 143866 | Gravity and braking forces |
| gaVehiclePhys.phys_calcSteerAngle | 144454 | Steering state |
| gaVehiclePhys.phys_applySuspAndTyreForces_HI | 144543 | Suspension and tire force/torque |
| gaVehiclePhys.phys_tickInputs | 144897 | Physics input state |
| gaVehiclePhys.phys_spawnWheelRay | 145602 | Wheel collision queries |
| gaVehiclePhys.phys_testWheelRay | 145637 | Wheel contact results |
| gaVehiclePhys.phys_tickGround | 145777 | Ground/air transitions |
| gaVehiclePhys.phys_contactCallback | 146228 | Body contacts |
| gaVehiclePhys.phys_calcGrip | 146350 | Tire grip curve |
| gaVehiclePhys.phys_calcSteer | 146368 | Speed-dependent steering limit |
| gaVehiclePhys.phys_tickAirControl | 146471 | Air control |
| gaVehiclePhys.phys_tickCollision | 146527 | Collision state |
| gaSpring.tick | 238576 | Original spring integrator |

The physics path calls `elPhysBody` and `elPhysWorld` native functions.
The decompilation identifies Bullet world/importer types, and declares engine
body functions as external methods (e.g. `elPhysBody.phys_moveCallback` at
line 318730). Native velocity access and its body-state offsets have now been
recovered from disassembly and used in the comparison host. The remaining
body/contact operations still need recovery or a validated integration. Finding the managed
force routines alone does not complete that work. Some ILSpy expressions in the
tire-force routine also have unresolved stack types; verify them against IL or
disassembly rather than guessing.

## Retained, independently checked ports

`driving.hpp` contains the steering-limit and signed brake-decision scalar
ports. `gameplay.hpp` retains the recovered gearbox torque curve.
The isolated original executable reference host supplies 108 steering and
90 brake evaluations. Native outputs match numerically, with zero maximum
steering error. These results establish helper parity only.

```sh
make -C NativeLinux all test
build/native-linux/test-driving analysis/native-port/driving-reference
```

Next integration must use original vehicle tuning, contact geometry, force and
torque routines, gearbox and pace controller together. Compare captured original
state and force outputs before enabling driving again. No placeholder dynamics
should be introduced to fill missing dependencies.
