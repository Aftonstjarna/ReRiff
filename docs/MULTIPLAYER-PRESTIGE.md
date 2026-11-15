# Multiplayer prestige candidate

This candidate adds opt-in progression for trusted test servers. It reuses the
native isolated-save hooks, original save serializer, and manual Credits
confirmation/reset flow. It needs **the matching multiplayer prestige test client** and the
`ghcr.io/aftonstjarna/reriff:experimental` server image. The published
1.4.0-beta.4 client does not enable multiplayer prestige.

Each server account has an immutable, randomly generated UUID. The server UUID
and account UUID determine its save domain. Username, display name, Steam ID,
launcher, and credential changes do not determine ownership. There is no Steam
account requirement. The client discovers the server UUID and automatically obtains its account UUID
on first connection. It creates a random credential and remembers it locally;
players do not enter IDs or tokens. The legacy Parse login/session stub does not
authorize progression.

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
```

When updating an existing test server, retain its Compose project and volume,
set the image above, and add `RERIFF_PROGRESSION_MODE: trusted` under the server
service’s `environment` (or use the updated Compose file with that setting in
`.env`). Pulling an image alone does not enable progression. Recreate the service
with `docker compose up -d server`; do not remove its volume.

Leave `RERIFF_SERVER_PASSWORD` blank for immediate connections. Set it in the
server environment to require a password when joining; the client prompts and
keeps the password in memory for that connection. Existing accounts must also
supply the current password. Password changes take effect after recreation.
This protects the progression APIs; discovery and the original song/Parse APIs
remain public. The password is not an account identifier or recovery credential.

The default port binding is localhost. Configure the existing proxy or
`RERIFF_BIND_ADDRESS` for a remote test client, and set `RERIFF_PUBLIC_URL` when
using a public origin. HTTPS is required for non-loopback credentials unless
`multiplayer_allow_http=1` is explicitly set for a trusted LAN test.

## Client setup

Install the matching test client and select the server through the existing
server controls or its `server.ini`:

```ini
[server]
external_url = http://192.168.0.211:5144
external_only = true
networking_enabled = true

[multiplayer_progression]
multiplayer_allow_http = 1
```

The HTTP opt-in is only needed for a trusted LAN without HTTPS. On an open server,
connection needs no additional prompt, account IDs, tokens or enabling flag.
A password-locked server prompts at startup; Cancel closes the game without
opening a multiplayer save. Incorrect passwords can be retried. The game has no
server-account password control available before Parse initialization and native
save access, so this startup flow reuses the Windows credential dialog with its
masked entry, DPI layout, and Enter/Escape handling. No in-game menu is added.

Credentials are stored atomically in `localserver/accounts/<endpoint-hash>.json`.
Back up that private directory to preserve accounts across reinstalls or devices;
losing it loses access to an automatically created account. Do not share it.
Each exact server URL gets a separate credential: changing to an alias does not
forward the original secret automatically. The server UUID is pinned on first
connection; an unexpected change stops startup instead of switching profiles.
The first credential is persisted before registration, making lost-response
retries return the same account. Corrupt cache files or revoked existing
credentials stop connection rather than silently replacing an account.

Existing manually configured credentials are imported when their URL and server
identity match. `multiplayer_enabled=0` explicitly disables automatic progression;
legacy servers without discovery retain their existing behavior. If a server
with an existing cached account loses discovery, the client stops to protect its
save. Credentials are never forwarded to redirects or fallback URLs.

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

Update the corresponding private account-cache token and restart (or the legacy
INI token if it has not yet been imported). Normally no manual provisioning or
rotation is needed. The `create-account` operator command remains available for
recovery and controlled tests. To disable the feature, set
`RERIFF_PROGRESSION_MODE=disabled` and recreate the server. Stored progression is
retained. No automatic migration, season reset or account deletion is performed.

## Validation boundary

Automated tests cover discovery, automatic first connection and retry, password
checks, the protocol, credentials and rotation, immutable account
identity, atomic resets, conflicting sessions, bounded requests, policy mismatch,
retry/restart recovery and original singleplayer checkpoints. Native regression
fixtures cover save routing and server/account isolation. A live in-game trial
of first-profile creation, earning, Credits confirmation and reconnect is still
required before treating this experimental feature as ready for general use.

## Admission limits

`/reriff/progression/info` is public and reports password requirements without
secrets. `/reriff/progression/connect` accepts a 4 KiB request and a client-created
256-bit random credential. Creation is atomic and idempotent for that credential;
known-account reconnects never recreate a revoked account. Each process allows
600 connect attempts per minute, including wrong passwords, with HTTP 429 on
excess attempts. Client-side retries never change the pending credential.
