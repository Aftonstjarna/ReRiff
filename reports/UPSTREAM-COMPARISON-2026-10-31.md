# ReRiff fixes and additions compared with riffapi-local-extended

**Prepared:** 31 October 2026  
**Project:** Aftonstjarna/ReRiff  
**Scope:** published server, client integration, launcher, installers, deployment, and documentation through the v0.1.6 release series.

## Comparison baseline

This report compares upstream [`rumbadebu/riffapi-local-extended` at `d6b98d2`](https://github.com/rumbadebu/riffapi-local-extended/tree/d6b98d2845bfbc8ee2888f805f8f0dc36bb73ede) with [ReRiff at `01cf5c6`](https://github.com/Aftonstjarna/ReRiff/tree/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0).

The upstream revision is both the version recorded in ReRiff's vendored provenance and upstream's current `main` revision when checked. Unpublished local reconstruction and native-port changes are excluded.

**Overall contribution:** ReRiff builds a configurable Windows/Proton client and deployment system around the inherited Parse-compatible backend. Its principal server correctness addition is recognizing identical source songs across different cached IDs, allowing existing scores and ghosts to be shared. It also adds diagnostics, native launcher controls, transactional installers, and automatic client updates.

## 1. What upstream already provided

The following are inherited foundations, not new ReRiff implementations:

- Persistent Parse class records and uploaded files.
- Score and ghost storage, track/playlist records, and notification objects.
- Queries, sorting, pagination, counts, pointer inclusion, atomic field operations, and batch requests.
- Permissive user-login handling and canned game configuration responses.
- The browser-based admin interface.
- Basic console/file logging, API tests, reverse-engineering tools, and Docker/Compose files.

Upstream itself builds on rebane2001/riffapi-local. Its README's feature claims are treated as attribution, not as proof that every feature is complete in ReRiff's live deployment. [Upstream README](https://github.com/rumbadebu/riffapi-local-extended/blob/d6b98d2845bfbc8ee2888f805f8f0dc36bb73ede/README.md).

ReRiff's vendored `admin.html` and upstream `LICENSE` remain byte-for-byte identical to that baseline. The admin GUI therefore should not be presented as newly authored by ReRiff. The Unlicense is retained; ReRiff's own code uses MIT, with separate third-party licenses.

## 2. Server fixes and extensions

| Area | Upstream behavior | ReRiff contribution |
|---|---|---|
| Song identity | Scores and ghost queries use the supplied song ID directly. | Registers cached IDs against source-file SHA256 and resolves equivalent IDs through a persistent catalog. |
| Shared leaderboards | Different IDs split the same song into different result sets. | Joins verified song identities for score queries, friend filtering, ordering, pagination, and rank counts. |
| Duplicate history | Separate records can survive under different cached IDs. | Ranks a player's best result once per song/mode/version while retaining stored records. |
| Ghost retrieval | Exact metadata names and filenames are expected. | Resolves ghost metadata, FileData queries, and equivalent ghost/track filenames without renaming stored uploads. |
| Diagnostics | Human-readable request logs can include credentials. | Adds redacted JSONL sessions, request IDs, timings, response details, and investigation flags. |
| Admin response tracing | No ReRiff trace wrapper exists. | Handles streamed/direct-passthrough responses without consuming their bodies or crashing admin responses. |
| Container operation | Runs the Flask script, which binds to container loopback. | Uses Waitress listening on all container interfaces, with explicit host-side exposure and persistent `/data`. |

### Automatic shared song identity

The underlying problem was not that scores failed to upload. Clients could retain different IDs for the same audio, then upload valid scores under those unrelated IDs. The server had no source identity in the score request from which to infer equivalence.

ReRiff adds the missing registration path:

1. The native launcher reads the supported existing song cache.
2. It hashes accessible source audio files and sends cached IDs, SHA256, title, and artist.
3. `POST /reriff/songs/register` validates the manifest using a native C++ registry engine.
4. The server atomically persists `song-catalog.json` in its data directory.
5. Score and ghost reads resolve equivalent IDs while preserving original record IDs and upload names.

Registration runs before the game launches and after it closes, covering new imports saved during a session. Conflicting reuse of a cached ID with a different hash is rejected without changing the catalog. The earlier explicit alias-file workaround remains optional compatibility support; automatic registration does not require per-song hardcoding.

This is **exact-file identity**, not acoustic fingerprinting: changed tags or a different encoding can produce a different hash. Registration sends neither audio bytes nor local paths. Hashes are reported by clients; this is not a server-side anti-cheat or identity-attestation system.

Sources: [native cache reader and hashing](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/NativeLauncher/song_sync.h), [C++ registry](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/ServerSync/catalog.cpp), [HTTP/storage adapter](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/vendor/riffapi-local-extended/song_catalog.py), [server query/file integration](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/vendor/riffapi-local-extended/riffapi-local.py).

### Investigation logging and admin compatibility

ReRiff's tracing wrapper records session/request IDs, timestamps, routes, decoded queries, JSON request/response information, status, duration, and exception frames. Binary bodies are represented by size and SHA256 instead of being duplicated into logs. Credential-like fields are redacted; player names, IDs, and gameplay data remain visible.

Flags identify unsupported operations, unmatched routes, HTTP/Parse/batch errors, empty results, and canned/fallback responses. A report command summarizes a captured session. These flags identify investigation candidates, not automatic proof of missing functionality.

The admin-panel crash fix belongs to this wrapper: it avoids reading bodies from streamed/direct-passthrough responses. It is not a replacement or redesign of upstream's unchanged admin frontend. Running the upstream script directly bypasses the wrapper's redaction.

Source: [trace-server.py](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/scripts/trace-server.py).

## 3. Client routing and native launcher additions

Upstream documents replacing the original endpoint string inside `ParseOctane.dll` with a fixed localhost address. ReRiff adds a client-side configuration helper and patch-building tools so destination selection can come from `server.ini` beside `Game.exe`.

Added behavior includes:

- Preferred external and fallback addresses, a configurable timeout, and an external-server-only mode.
- A startup probe of `/classes/Version`, with selection held for the game session.
- Disabled Parse networking for blank, missing, unreadable, or invalid configuration, enforced through the SDK transport gate.
- Endpoint-selection logs. Failed uploads do not silently switch databases or retry against a different server.
- A native Win32 launcher with settings editing, save-and-play, local-server controls, and `--skip-launcher`.
- A Steam/Proton bridge that starts native Linux Python while keeping the Windows launcher and game in Steam's selected prefix.
- A server notification that fades automatically without taking input focus or requiring a controller-disrupting confirmation.
- A window title showing the running launcher's compiled version.

Stopping a backend verifies ownership rather than indiscriminately killing whatever uses port 5144. Linux matches the runner and server-directory arguments and checks process identity; Windows uses a recorded PID and creation time. The Proton fix uses an atomic completion marker instead of trusting Wine `start.exe` to report the Unix helper's result. An already-stopped server succeeds; unrelated processes and Docker services remain untouched.

Sources: [client helper](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/ClientConfig/ClientConfig.cs), [patcher](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/Patcher/Program.cs), [launcher](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/NativeLauncher/launcher.c), [notification](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/NativeLauncher/notification.c), [server control](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/scripts/server-control.py).

## 4. Installers and automatic updates

ReRiff adds Windows setup packages and Linux self-extracting installers, with explicit game-directory selection and Steam-library discovery. Installation validates package contents and supported DLL hashes, preserves settings/data, records backup locations outside the game folder, and rolls back changed files on an installation failure.

Current public releases are **update-only**. They require an already supported ReRiff installation and do not distribute original game binaries or game-derived DLL patches. Separate local patch-building/restoration tools exist; a current public update is not a fresh-game installer or an original-DLL restore package.

The launcher update button now:

1. Finds the latest stable GitHub release for the platform.
2. Downloads the package and published SHA256 checksums.
3. Verifies the download and performs installer preflight before closing.
4. Uses a temporary native updater that waits for the old launcher to exit.
5. Confirms owned-backend shutdown, applies the transactional installer, and reopens the launcher.

Windows applies from a private copy of portable Python so that the installed runtime can itself be replaced. Proton uses native Linux Python and explicit completion markers for the Wine bridge. Docker updates remain separate from client updates.

Sources: [installer](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/packaging/installer.py), [native downloader/preflight](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/NativeLauncher/auto_update.h), [native handoff](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/NativeLauncher/updater.c), [apply tool](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/scripts/apply-client-update.py).

## 5. Docker, publishing, and documentation improvements

Docker support is an **extension of existing upstream support**, not a first introduction. Upstream supplies a small Python/Flask image and port mapping. ReRiff adds:

- A separate native-registry build stage and a Waitress runtime with a non-root default user.
- Pinned Python dependencies, a healthcheck, graceful shutdown handling, and a single backend process with worker threads.
- Persistent `/data` storage and a configurable client-visible file URL through `RERIFF_PUBLIC_URL`.
- Local-only default Compose exposure, optional LAN configuration, and host-network/bind-mount instructions.
- Read-only root filesystem, temporary storage, dropped capabilities, and no-new-privileges in the supplied Compose configuration.
- GitHub container builds, integration checks, and publication to `ghcr.io/aftonstjarna/reriff`.
- Automated Windows/Linux release builds, checksums, source archives, installer checks, and version-specific release notes.
- One maintained main README for user instructions, with old guides redirected and standalone README release attachments removed.

The container settings improve deployment behavior; they do not turn the inherited permissive application API into an authenticated public service. Restarting an old container also does not install a newly pulled image: recreation must retain the existing data mount.

Sources: [upstream Dockerfile](https://github.com/rumbadebu/riffapi-local-extended/blob/d6b98d2845bfbc8ee2888f805f8f0dc36bb73ede/Dockerfile), [ReRiff Dockerfile](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/Dockerfile), [Compose](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/compose.yaml), [workflows](https://github.com/Aftonstjarna/ReRiff/tree/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/.github/workflows), [main README](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/README.md).

## 6. Evidence and validation limits

| Evidence | What it establishes |
|---|---|
| Eight isolated song-identity regression tests | Cross-ID scores/ghosts, counts, filename lookup, conflict rejection, preservation of records, and distinct-file separation. |
| Six updater tests | Platform extraction, checksum/path checks, preflight, shutdown failure handling, preserved data, and rollback. |
| Six installer tests | Installation/restore behavior, dry runs, unsupported modifications, corrupt packages, and failed-write rollback. |
| Native Wine fixtures | Cache reading, source hashing, HTTP registration, parent-exit handoff, Linux/Windows apply-and-relaunch, and owned/already-stopped server handling. |
| Actual GitHub download test | Native HTTPS download, published checksum verification, and installer preflight in a disposable fixture. |
| Docker integration checks | Non-root/read-only operation, health/admin access, score/ghost persistence and catalog behavior across restart, tracing, and file URLs. |
| User confirmation | Automatic update success reported on two actual clients. |

These are additional ReRiff checks; upstream already included its own API/auth/operation tests. The user-confirmed updater results should not be expanded into a claim that every multiplayer, profile, or leaderboard feature has been verified end to end.

Representative test sources: [song identity](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/scripts/test_song_aliases.py), [updater](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/scripts/test_client_update.py), [installer](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/packaging/test_installer.py), [Docker](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/docker/smoke_test.py), [Wine stop regression](https://github.com/Aftonstjarna/ReRiff/blob/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0/scripts/test_stop_server_wine.py).

## 7. Issues identified but not fixed in this comparison

- **Fallback profile persistence:** the inherited canned `RiffApiLocalProfile` can be returned without a corresponding stored object, so updates to it can return 404. The logs identified this; the compared server still contains that path.
- **Canned News formatting:** the inherited manually assembled response can contain unescaped newlines and invalid JSON. This was identified during tracing and is not claimed as fixed.
- **Authentication/access control:** login remains permissive and admin endpoints are not hardened for public exposure. Log redaction is not an authentication or privacy audit.
- **Historical recovery:** lost original-server records and assets cannot be reconstructed from score uploads alone. Registration requires accessible source files and supported clients; different databases are not automatically synchronized.
- **Runtime/platform limits:** the game still needs its existing Windows/.NET environment and Proton on Linux. The native launcher does not make the game itself Linux-native. Notification visibility can vary in exclusive fullscreen.

The report does not claim complete recovery of the original service, a complete Game.exe source reconstruction, removal of the game's .NET dependency, or authorship of the inherited Parse backend, admin GUI, or vendored JSON library.

## 8. Fix and addition commit index

These links identify commits in the public ReRiff history. Several earlier changes
were published together in the initial source import; those rows point to that
import rather than implying a separate public commit exists for every fix.

| Fix or addition | Change represented | Commit |
|---|---|---|
| Configurable client routing and offline gate | Game-folder INI selection, external/fallback handling, SDK gating, and client patch tools. | [`8845da8`](https://github.com/Aftonstjarna/ReRiff/commit/8845da8e94581f9fd2b78e193320d80fd1954f95) — grouped initial import |
| Native launcher, Proton bridge, and notification | Native launcher/setup, `--skip-launcher`, native Linux backend bridge, and non-interactive notification. | [`8845da8`](https://github.com/Aftonstjarna/ReRiff/commit/8845da8e94581f9fd2b78e193320d80fd1954f95) — grouped initial import |
| Redacted tracing and admin-response compatibility | Structured traces, investigation flags, report tooling, and streamed-response handling. | [`8845da8`](https://github.com/Aftonstjarna/ReRiff/commit/8845da8e94581f9fd2b78e193320d80fd1954f95) — grouped initial import |
| Installer and Docker deployment foundation | Installer backup/rollback tools, persistent container storage, Waitress integration, smoke tests, and GHCR workflow. | [`8845da8`](https://github.com/Aftonstjarna/ReRiff/commit/8845da8e94581f9fd2b78e193320d80fd1954f95) — grouped initial import |
| Local-server controls and external-only mode | Ownership-aware stop control, external-only client behavior, and the original GitHub release-check button. | [`6bb7a6c`](https://github.com/Aftonstjarna/ReRiff/commit/6bb7a6c8f1f5765d15d10087df1f114c43147b5c) |
| Public update-only release pipeline | Automated client builds, pinned build-tool downloads, required-existing-DLL checks, and update-only packaging. | [`22cba5a`](https://github.com/Aftonstjarna/ReRiff/commit/22cba5ad82bf3167d411e58c73b17017f6b9efa1) |
| Shared leaderboard and ghost identity fix | Automatic exact-source registration, persistent C++ catalog, equivalent-ID queries/file reads, and best-score ranking. | [`a8da5a8`](https://github.com/Aftonstjarna/ReRiff/commit/a8da5a8d1367ed3536eff1f6fb3321ed68435455) |
| Actual automatic client updating | Verified release downloads, installer preflight, native exit/install/relaunch handoff, copied Windows runtime, and Wine completion markers. | [`c997667`](https://github.com/Aftonstjarna/ReRiff/commit/c99766720797d147646408643c68c08c071d8c16) |
| Running version in launcher title | Displays the executable's compiled version in its window title. | [`88372b6`](https://github.com/Aftonstjarna/ReRiff/commit/88372b6984453595cd81ec552e3c92c979223669) |
| Proton server-stop/update failure | Replaces misleading Wine stop status with explicit completion; confirms Linux shutdown in the apply tool; supports already-stopped backends. | [`bc24f35`](https://github.com/Aftonstjarna/ReRiff/commit/bc24f35521cc8fa7d7fbcc2161c3f4bca3513aef) |
| Per-version release notes | Extracts only the matching changelog section for release bodies, note assets, and bundles; corrects the latest published notes. | [`0a671bf`](https://github.com/Aftonstjarna/ReRiff/commit/0a671bf6ca92b11eae706acc8ebe1b13a7095954) |
| Updater verification release | Advances to v0.1.6 so v0.1.5 clients can exercise the real updater. This is a verification/version change, not another behavioral fix. | [`abbbe91`](https://github.com/Aftonstjarna/ReRiff/commit/abbbe91dce876224f9b4dc47dab3a04644790ecb) |
| Centralized current documentation | Consolidates the main README, redirects old guides, removes the obsolete Linux README, and removes standalone README release assets. | [`01cf5c6`](https://github.com/Aftonstjarna/ReRiff/commit/01cf5c6aef07dd1a303497aca7ec05fa6f3ebde0) |

The profile persistence, canned News formatting, and access-control issues listed
above have no fix commits in this snapshot and are intentionally excluded from
the fixed-work table.

## Suggested contribution description

> ReRiff extends riffapi-local-extended with configurable Windows/Proton client routing, exact-source song identity recovery for shared scores and ghosts, structured redacted tracing, a native launcher with safe local-server controls, transactional installers, automatic verified GitHub updates, and persistent tested container deployment. It retains and credits upstream's Parse-compatible persistence, API operations, authentication scaffolding, and admin interface, while documenting the remaining service and runtime limitations.
