# Configuration files

| File | Purpose |
| --- | --- |
| `server.ini` | Dedicated/public server connections, fallback and networking mode. |
| `client.ini` | Graphics, languages, launcher preferences and diagnostics. |
| `singleplayer.ini` | Isolated Singleplayer progression and balance. |
| `localserver/server.ini` | Launcher-managed LAN service port and data directory. |

The game migrates the former combined `server.ini` automatically. Explicit
values already in the new files win. Unknown sections remain in the original
file. Exact pre-migration configurations are kept under `.bak/configs`, with at
most two snapshots per filename. Legacy values remain readable until migration
finishes. Existing saves, race records, font files and server data stay in place.
The LAN helper also accepts its old `native/config.json`; an explicit LAN INI
wins and a new installation writes the INI.

Restart the game after editing its settings. Restart the local server after
editing its service INI. The client INI never overrides a multiplayer server's
balance. This prestige beta applies its configurable economy to isolated
Singleplayer only; multiplayer progression and enforced balance remain separate
server integration work.

## Singleplayer balance

`singleplayer.ini` contains a `[balance]` section. Defaults preserve the tested
prestige economy:

```ini
[balance]
coin_reward_rate = 1
creator_reward_rate = 1
prestige_multiplier_growth = 1
prestige_multiplier_cap = 0
paint_price_scale = 1
discount_level_10 = 0.10
discount_level_20 = 0.20
discount_level_30 = 0.25
discount_maximum = 0.30
discount_interval = 10
discount_remaining_gap_factor = 0.5
main_level_cap = 0
main_level_xp_step = 0
main_level_xp_growth = 1
```

Race coins use `coin_reward_rate`; first-generation creator coins use
`creator_reward_rate`. Both multiply by `1 + prestige × prestige_multiplier_growth`.
A cap of `0` is uncapped; any other cap must be at least `1`. Purchases retain
their original debit; only earned awards are multiplied. Results display and
saved credits use the same amount, and credits saturate at the native limit.

Discounts are fractions. Levels 10, 20 and 30 use their corresponding values;
after level 30, each `discount_interval` levels multiplies the remaining gap to
`discount_maximum` by `discount_remaining_gap_factor`. Native coin-pack prices
are multiplied by `paint_price_scale`, then discounted and rounded down. Paid
packs still cost at least one coin. Native IAP identifiers are unaffected.

Rates and price scale accept 0–1,000,000. Discounts must increase monotonically
and stay below 1 (maximum 0.99); the gap factor is 0–1. The interval must be a
positive whole number. An invalid policy is rejected as a whole, logged when
enabled, and the default policy is used. Loading a new policy never prestiges or
resets a save. The next save records the effective balance revision.

### Main levels above 40

The native XP curve is retained through level 40. After that, defaults repeat
the original 39→40 cost: **250,000,000 XP per level**. For example, level 40
requires 1,000,000,000 permanent XP, level 41 requires 1,250,000,000 and level 42
requires 1,500,000,000. Both ordinary and prestige-cycle XP count toward the
permanent total. Car unlocks and native level-up coin awards still use the
original prestige-cycle level table.

- `main_level_cap = 0` adds no configured cap. A whole number of at least 40
  sets a maximum displayed main level; use 40 to restore the vanilla maximum.
- `main_level_xp_step = 0` reads the original final XP interval. A positive
  whole number overrides the cost of level 40→41.
- `main_level_xp_growth = 1` repeats that cost. Values above 1 multiply the
  cost for each subsequent level. With a step of 250,000,000 and growth of 2,
  40→41 costs 250,000,000, 41→42 costs 500,000,000, and 42→43 costs 1,000,000,000.
  Fractional growth is supported; integer XP must reach the resulting threshold.

Changing these settings recalculates main level from existing permanent XP;
it never removes XP, resets progression or grants extra cycle rewards.
Restart to apply. With a configured cap, the original Profile bar shows level
relative to that cap. Uncapped mode retains the original bar below 40 and shows
progress toward the next level from 40 onward. Level identity uses a signed
32-bit integer (maximum 2,147,483,647); XP remains the existing unsigned 64-bit
counter. XP step accepts 0–18,446,744,073,709,551,615 and growth accepts
1–1,000,000. These settings currently apply to isolated Singleplayer only.

## LAN service

```ini
[server]
port = 5144
data_dir = data
```

The port must be 1024–65535. `data_dir` is `data`, or `podman/data` for a retained
legacy installation. Service configuration is independent of connection fields
in the game root's `server.ini` and of Singleplayer's economy.
