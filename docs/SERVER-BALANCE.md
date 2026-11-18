# Server balance

Available in ReRiff 1.4.0. Balance policy is published at `GET /reriff/balance`.
Enable trusted multiplayer progression separately as described in
[MULTIPLAYER-PRESTIGE.md](MULTIPLAYER-PRESTIGE.md). This is not anticheat.

## Per-server balance settings

Define server balance through **`RERIFF_BALANCE_*` environment variables**.
Use [docker/server.env.example](../docker/server.env.example) as the starting
point. Its balance values match the local singleplayer INI example captured in
[Progression/fixtures/singleplayer.ini](../Progression/fixtures/singleplayer.ini).
The packaged fallback preset is [Progression/balance.json](../Progression/balance.json);
omitting all balance variables gives the same settings as the example.

For `docker/compose.host.yaml`, copy the example to `server.env` beside the
Compose file. For the repository's root `compose.yaml`, put the balance variables
in `.env` beside that file (or export them in the shell). The root Compose file
explicitly forwards them to the container. A custom Compose stack must likewise
pass them via `env_file` or `environment`; an interpolation-only `.env` file is
not automatically passed to containers. Standalone servers read their process
environment. Client singleplayer INI files are never loaded by the server.

After editing the env file, recreate the service with
`docker compose up -d --force-recreate server` (use `reriff` if that is your
service name). A restart alone retains the container's old environment.
The old `/data/balance.json` override is no longer read; move any intended
customizations to the env file. No saved progression or stored files are changed.

The public `GET /reriff/balance` response contains the server UUID, validated
settings and the SHA-256 of their canonical representation. Invalid configuration
returns 503 instead of silently applying easier defaults. Empty values and
unknown `RERIFF_BALANCE_*` names are rejected, including misspelled settings.

Names mirror the singleplayer `[balance]` keys, uppercased and prefixed with
`RERIFF_BALANCE_`. They cover race/creator coin rates, prestige multiplier growth
and cap, paint price scaling, interval or legacy discounts, and permanent main
level cap/XP step/growth. `PRESTIGE_MULTIPLIER_CAP=0` and `MAIN_LEVEL_CAP=0` mean
uncapped; `MAIN_LEVEL_XP_STEP=0` means use the native 39-to-40 cost. Main-level
settings are published policy only; the endpoint does not advance player levels.

Additional server settings are `MAIN_XP_RATE`, `PRESTIGE_XP_RATE`, `SEASON`,
`ANTI_CHEAT`, `CAR_LEVELS`, `REQUIRED_ACHIEVEMENTS`, and `MAXIMUM_PRESTIGE_COUNT`,
all under the same prefix. `CAR_LEVELS` is a JSON mapping such as `'{"5":15}'`;
`'{}'` retains native unlock requirements. `REQUIRED_ACHIEVEMENTS` is `native_all`
or a JSON list of native achievement IDs such as `'[0,1]'`.
`MAXIMUM_PRESTIGE_COUNT=null` is unlimited; `0` disallows further resets.

The initial coin multiplier is `1 + prestige_count`. Defaults award ×1 before
prestige, ×2 after the first wipe and ×3 after the second. XP rates default to
×1 for both permanent and prestige XP. Fractional rewards round down and
integer counters cannot exceed the native signed 32-bit range.

Default discounts follow the local singleplayer interval curve: 20% at level 10,
25% at 15, 30% at 20, 35% at 25, 40% at 30, 45% at 35, and 50% from 40 onward.
Set `DISCOUNT_MODE=legacy`, `DISCOUNT_INTERVAL=10`, and `DISCOUNT_MAXIMUM=0.30`
to select the earlier 10/20/30 milestone curve and its diminishing gap to 30%.
Paid packs retain a minimum price of one coin; genuinely free packs remain free.
Server policy preview does not claim that multiplayer clients apply this tuning.

Singleplayer retains its separate local balance policy and save domain.
