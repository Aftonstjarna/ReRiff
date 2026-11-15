# Multiplayer prestige candidate

This candidate adds opt-in progression for trusted test servers. It reuses the
native isolated-save hooks, original save serializer, and manual Credits
confirmation/reset flow. It needs **the matching multiplayer prestige test client** and the
`ghcr.io/aftonstjarna/reriff:experimental` server image. The published
1.4.0-beta.4 client does not enable multiplayer prestige.

Each server account has an immutable, randomly generated UUID. The server UUID
and account UUID determine its save domain. Username, display name, Steam ID,
launcher, and credential changes do not determine ownership. There is no Steam
account requirement. Credentials are provisioned by the server operator; the
legacy Parse login/session stub is not used to authorize progression.

## Test-server setup

Use a separate Compose project and port with the repository's image-only Compose
file. Its named volume is project-scoped; do not reuse a production data volume.
Put these settings in `.env` beside that Compose file:

```dotenv
RERIFF_IMAGE=ghcr.io/aftonstjarna/reriff:experimental
RERIFF_PORT=5145
RERIFF_PROGRESSION_MODE=trusted
```

```sh
docker compose -p reriff-prestige-test pull server
docker compose -p reriff-prestige-test up -d server
docker compose -p reriff-prestige-test exec server cat /data/server-id
docker compose -p reriff-prestige-test exec server python /app/Progression/multiplayer.py create-account
```

When updating an existing test server, retain its Compose project and volume,
set the image above, and add `RERIFF_PROGRESSION_MODE: trusted` under the server
service’s `environment` (or use the updated Compose file with that setting in
`.env`). Pulling an image alone does not enable progression. Recreate the service
with `docker compose up -d server`; do not remove its volume.

The last command prints `multiplayer_account_id` and `multiplayer_token`.
Store the credential privately. The database stores its hash, not the credential.
Create a different account for each player. Moving a player's credential to
another device keeps the same account; simultaneous saves use generation checks
so a stale session cannot overwrite newer progress.

The default port binding is localhost. Configure the existing proxy or
`RERIFF_BIND_ADDRESS` for a remote test client, and set `RERIFF_PUBLIC_URL` when
using a public origin. HTTPS is required for non-loopback credentials unless
`multiplayer_allow_http=1` is explicitly set for a trusted LAN test.

## Client setup

Use a separate candidate client installation and add to its `server.ini`:

```ini
[server]
external_url = http://127.0.0.1:5145
external_only = true
networking_enabled = true

[multiplayer_progression]
multiplayer_enabled = 1
multiplayer_url = http://127.0.0.1:5145
multiplayer_server_id = SERVER_UUID_FROM_COMMAND_ABOVE
multiplayer_account_id = ACCOUNT_UUID_FROM_COMMAND_ABOVE
multiplayer_token = CREDENTIAL_FROM_COMMAND_ABOVE
```

The configured URL must match the selected endpoint exactly. Credentials are
never forwarded to redirects or fallback endpoints. This section remains in
`server.ini`; singleplayer settings remain in `singleplayer.ini`.

A new account starts a fresh native profile. Original and singleplayer saves are
not imported or reset. Existing server progress is downloaded before native save
access. The original game handles serialization, rewards and UI, using the
server's supported balance values. Original race/song/ghost storage is retained.

Earn all 38 achievements displayed by the original profile. Open Credits and
confirm the native prestige prompt. Cancel/Back changes nothing. Confirm saves
current changes, checks the server again, resets through the original native
methods, and submits the reset checkpoint. The server preserves main XP, adds
one prestige, and requires zero cycle XP/coins, cleared cycle achievements,
First Prestige, and only the equipped car/paint ownership. No Steam achievement
clear operation is called.

## Persistence and policy

The server atomically stores the opaque native save, validated observations,
permanent XP, cycle XP, prestige count and balance revision in
`/data/progression/profiles.sqlite3`. This is outside the public Parse classes
and files APIs. The local multiplayer save is a working cache, not authority.
Back up the entire data volume including the server UUID and database.

Requests authenticate with the provisioned account credential. Ordinary saves
cannot decrease cycle XP, clear achievements, or supply a prestige count.
Prestige eligibility is checked against the last committed checkpoint. A
request ID and generation protect retries and competing sessions. A lost
acknowledgement can be retried without a second reset. After a failed save, the
game stops; reconnect restores the server's last complete generation. Gameplay
since the last acknowledged checkpoint may be lost on a disconnect.

Balance remains environment-controlled (`docker/server.env.example`). Trusted
mode currently requires native XP rates, native car unlock requirements,
`native_all` prestige achievements and `ANTI_CHEAT=disabled`. Unsupported policy
returns an error instead of silently using different rules. Reward rates,
prestige growth/cap, paint discounts, main-level settings and maximum prestige
count are supported. A balance change requires reconnecting and does not reset
progress.

The server trusts gameplay observations from authenticated test clients. It
cannot verify the opaque native save or prove that achievements were earned.
Capabilities therefore report `trustedClientProgression: true`,
`serverCheckpoints: true`, `prestigeReset: true`, but
`progressionEnforced: false` and `antiCheatVerified: false`. Do not interpret a
stored credential or validated reset transition as anticheat verification.

To rotate a lost credential without changing account identity or progress:

```sh
docker compose -p reriff-prestige-test exec server python /app/Progression/multiplayer.py rotate-token --account-id ACCOUNT_UUID
```

Update the client token and restart. To disable the feature, set
`RERIFF_PROGRESSION_MODE=disabled` and recreate the server. Stored progression is
retained. No automatic migration, season reset or account deletion is performed.

## Validation boundary

Automated tests cover the protocol, credentials and rotation, immutable account
identity, atomic resets, conflicting sessions, bounded requests, policy mismatch,
retry/restart recovery and original singleplayer checkpoints. Native regression
fixtures cover save routing and server/account isolation. A live in-game trial
of first-profile creation, earning, Credits confirmation and reconnect is still
required before treating this experimental feature as ready for general use.
