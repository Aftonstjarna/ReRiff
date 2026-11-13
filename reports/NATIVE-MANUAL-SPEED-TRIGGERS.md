# Original manual-speed trigger path

Inspected 2026-11-02. Research only; no gameplay patch applied and no
matching-byte credit claimed. An in-game controller test remains necessary.

## Original provenance

Reference: Steam depot 351991/build 1687655, from the configured
SteamDepotDownloader original-file directory. Game.exe is 8,635,392 bytes,
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
The existing local IL survey identifies that same executable hash. Its
decompiled C# is explanatory evidence, not an exact reconstruction. IL bodies
below were independently extracted from the original PE sections at their RVAs.

## Verified native and IL behavior

- Native `0x4F8180` reads a trigger float at `this + 0x38 + index*4`, returns
  through x87 ST0 and uses `ret 4`. Exact 14 bytes:
  `55 8b ec 8b 45 08 d9 44 81 38 5d c2 04 00`.
  The original vtable pointer at `0x9FDB98` contains `80 81 4f 00`.
- Native controller update at `0x4F8260` obtains XInput state through the IAT
  at `0x92D620`, decodes its two trigger bytes into offsets `0x38`/`0x3C`,
  and also copies the results into its combined-device backing object.
  The update accepts its elapsed time in XMM1 and its object in ECX.
  Its 982 bytes through RET at `0x4F8635` have SHA-256
  `561ad476c3f9d01ce146f144a93984e931bb5ecf47bfe02f95ad5b33ada33f8b`.
- `gaInputManager.tickX360`, token `0x06000C5E`, RVA `0x29464C`,
  reads trigger index 1 through the X360 interface's virtual slot +32,
  applies the original `elMath.deadZone` (0.1, 1.0), and writes acceleration
  at `gaInputData + 0`. It reads index 0 the same way and writes braking
  at `gaInputData + 4`. Steering reads the stick separately.
- `gaInputManager.tick`, token `0x06000C5D`, RVA `0x294770`, invokes that
  mapping for the current controller before its PC keyboard handling.
  `tickPC`, token `0x06000C5F`, RVA `0x294008`, only overrides acceleration
  or braking when their configured keyboard keys are held.
- `gaPlayerLocal.tickInput`, token `0x060005B8`, RVA `0x212844`, copies the
  player's 28-byte input record to player offset 560. The manual-speed
  branch (manager byte +132 is zero) uses acceleration and braking to compute
  the pace-speed adjustment, then calls the original automatic physical
  throttle/brake calculation. Thus the trigger inputs adjust manual target
  speed; they are not a replacement vehicle physics implementation.

SHA-256 of complete original fat-header-plus-IL bodies, excluding any EH
sections: tickX360 (292 bytes)
`ca0c137204f778bf10387ff10d214e1f975b36e9855c103e3b5eeb7c94068923`;
tickInput (841 bytes)
`25349dfe0f532bb2b3574ab24b15ddca97a4eb716828b0d8b19a3904c9fd4804`;
tick (563 bytes)
`418a54ed84e55d498f75e5d12fa60e6f9e15701196987af218a3a2e4c44773ff`;
tickPC (982 bytes)
`36a6c8f276531aaa8533c36b52c832d1ed2e6a093f0dc4f6f6c8552ea4e55ad1`.

## Direct native execution check

Local diagnostic: `build/controller-research/probe.c`, SHA-256
`1b1665f5d0b3558ef9e29e091a34c28fe1eb9ea77cba8ef5da89ef724ca64115`.
Compiled for x86 Windows with Zig 0.14.1, executed under isolated Proton Wine.
The test maps the original PE without entering the game/CLR, applies PE
relocations when needed, substitutes only XInputGetState with synthetic states,
and executes the original native update and trigger getter. A backing combined
device is supplied and vibration remains inactive. The installed game and
controller mappings are untouched.

| Synthetic input | Original LT value | Original RT value |
| --- | ---: | ---: |
| Neutral | 0 | 0 |
| RT byte 128 | 0 | 0.503467 |
| RT byte 255 | 0 | 1 |
| LT byte 255 | 1 | 0 |
| Left stick Y 32767, triggers released | 0 | 0 |

All checks passed. This verifies native decoding, not an end-to-end race or
physical controller/Steam Input configuration. The managed mapping was checked
against IL; it was not executed in this native-only harness. If triggers fail
in a race, inspect live controller selection, manual-speed state, input values,
and focus/pause handling before replacing the existing input implementation.
