> Historical upstream tutorial retained for attribution and reference.
> Current ReRiff setup and usage are maintained in the [main README](../README.md).

# Rebuilding Riff Racer's online backend - a reverse-engineering tutorial

This repository documents, step by step, how the online API of the 2015 Steam game
**Riff Racer - Race Your Music** was reverse-engineered from its binaries after the
official servers were shut down, and how a fully working local replacement backend
(ghost uploads, leaderboards, logins, notifications) was rebuilt on top of that
knowledge.

The finished, production-ready server lives in the companion repository
**riffapi-local-extended**. This repository contains the same server plus:

- the **analysis tools** used to recover the API surface from the game files,
- the **test suites** that verify the server speaks Parse correctly,
- the **incremental patches** that show how the server evolved,
- and this **tutorial** explaining every step and every discovery.

> All player names, identifiers and scores shown in examples are **sanitized
> placeholders** (PlayerOne/PlayerTwo etc.), not real accounts.

## Credits & provenance

- [rebane2001](https://github.com/rebane2001) wrote the original
  [riffapi-local](https://github.com/rebane2001/riffapi-local) server, which is the
  **starting point of this work**: it patched the game's `ParseOctane.dll` to talk to
  `127.0.0.1:5144` and faked the handful of Parse responses needed to boot the game.
  Everything in here builds on top of that foundation.
- The reverse engineering, the persistence/auth/admin implementation, the tools and
  this tutorial were done by **ox-alpha** (an AI assistant) working interactively
  with the game's community.

## Table of contents

1. [Background: the game and its backend](#1-background)
2. [The starting point: what riffapi-local did](#2-starting-point)
3. [Surveying the installation](#3-survey)
4. [Identifying Game.exe (PE/.NET forensics)](#4-gameexe)
5. [Mining strings: recovering the API surface](#5-strings)
6. [The Parse SDK: endpoints and headers](#6-sdk)
7. [Gap analysis: what the fake server was missing](#7-gaps)
8. [Designing the storage layer](#8-storage)
9. [The query engine](#9-query)
10. [Files, URLs and the 127.000000000.0.1 trick](#10-files)
11. [Atomic operations and batch](#11-ops)
12. [Live test #1: the silent game](#12-test1)
13. [Live test #2: the login wall](#13-test2)
14. [Implementing Parse auth](#14-auth)
15. [Live test #3: victory - reading real traffic](#15-victory)
16. [The admin GUI](#16-admin)
17. [Testing strategy](#17-testing)
18. [Repository layout and tool usage](#18-layout)

---

## 1. Background

Riff Racer generates a racetrack from any music file and lets you race it. Online,
it used [Parse](https://parseplatform.org/) (later back4app) as its backend:

- after finishing a track, the game **uploads a ghost** of your run so other players
  (and your future self) can race against it;
- it keeps **per-track leaderboards** and notifies you when someone beats your score;
- it stores **profiles, shared tracks and playlists**.

The servers died years ago. The game still *works* offline, but every online feature
silently fails - and, crucially, some single-player flows (like creating new tracks)
also expect the backend to answer.

## 2. Starting point

rebane2001's riffapi-local is a ~40-line Flask app. Its key tricks:

1. **Patch the DLL.** The game's Parse wrapper (`ParseOctane.dll`) contains the
   string `https://parseapi.back4app.com`. Replacing it byte-for-byte with
   `http://127.000000000.0.1:5144` (same length!) redirects all API traffic to
   localhost. The weird `127.000000000.0.1` is still a valid way to write `127.0.0.1`
   - the padding exists purely to keep the string length identical.

2. **Fake the boot responses.** The game needs a `Configuration` object (coin
   prices), a `News` entry, a `Version` object and a `Profile_Steam` object before it
   will start. The original server returns hardcoded JSON for those.

3. **Swallow everything else.** `POST /classes/<X>` returns a random objectId but
   stores nothing; `GET /classes/<X>` returns an empty result list; `POST /files/...`
   returns a fake URL pointing nowhere.

That is enough to *play*, but nothing persists and no ghost ever comes back. Our
goal: make the game believe the servers are alive again.

## 3. Survey

The game folder looks like this:

    Game.exe              8.6 MB
    Parse.dll             205 KB    <- Parse .NET SDK
    Parse.NetFx45.dll      30 KB    <- SDK platform shim
    ParseOctane.dll        74 KB    <- game's C++/CLI Parse wrapper (patched)
    steam_api.dll, steam_appid.txt
    vfs.dat, vfs.zip                <- game assets in a virtual file system
    audio/, ...

Two immediate questions: **where is the game logic** (what does it send?), and
**what exactly does the SDK put on the wire?**

## 4. Identifying Game.exe

`tools/03_identify_exe.py` parses the PE header by hand (no dependencies):

- `Machine: 0x14c` - 32-bit x86;
- the .NET data directory entry is non-zero - it **is** a .NET assembly;
- but `ilspycmd -l c` lists only four `<CrtImplementationDetails>` classes and the
  metadata is full of `std::` symbols like `gaTrackManager`, `elNetworkParse::object`.

Conclusion: Game.exe is a **mixed-mode C++/CLI assembly** - the whole engine is
C++ compiled to IL. A clean C# decompilation is impossible; the useful information
is in **string literals** (class names, field names, format strings) and in the
behavior we can observe on the wire. That shaped the whole methodology:

> Don't decompile - **mine strings, then watch the traffic.**

Also interesting: the binary contains `s_parseAppKeyPC`, `s_parseClientKeyPC`,
`s_parseWindowsKeyPC` - the Parse credentials are compiled in, and the SDK sends
them as `X-Parse-Application-Id` / `X-Parse-Windows-Key` headers on every request.
A local server can simply ignore them.

---

## 5. Mining strings

`tools/01_mine_strings.py` extracts ASCII and UTF-16LE strings from the binaries and
filters them by keywords (`ghost`, `score`, `parse`, `objectId`, ...).
`tools/02_string_context.py` then prints the *neighbors* of interesting strings -
in C++ binaries, related literals sit close together in .rdata, so context is gold.

### The Parse class catalog (from Game.exe)

Around the literal `"PlaylistTracks"` we found the whole class list:

    News, Configuration, Version, _Installation,
    Track, PlaylistTracks, Playlists, Notification,
    Profile_Steam, Ghost, Score, ...

### Field names, learned the same way

Context around `"Ghost"` / `"Score"` / `"Uploader"`:

    Ghost-ish fields : Score, PlayerAlias, Owner, SongID, Mode, PlayerID, Pack,
                       Uploader, UploaderID
    Track fields     : Artist, Title, Genre, userName, song, pack, Plays,
                       PlaysDaily, PlaysWeekly, Finishes, Platform, trackViewUrl
    Notification     : RecipientID, SongName, ArtistName, PreviousScore,
                       NewScore, NewLeaderName, FriendNotification
    Profile_Steam    : SystemID, Username, Platform, DriverLevel, FavouriteCar,
                       FavouriteSong, FavouriteGenre, TracksCreated, MostRaced

### Local file formats

More context mining revealed the save-game layout in
`%APPDATA%\Octane\DAT`:

    save.bin, cache.bin, online.bin, library.bin,
    pendingGhostUploads.bin      <- outbox for ghost uploads (with retry!)
    <objectId>_v18               <- cached generated-track binaries ("0GNS" magic)
    "%s-%s.ghost"                <- ghost file naming pattern

`online.bin` turned out to hold 32-byte leaderboard records (12-char id, name,
score, timestamp) - the game caches what the server tells it.

### UI strings that describe behavior

    "Uploading", "Error - upload failed", "Press enter to confirm upload of:",
    "DownloadingGhost", "GHOST_Checking", "GHOST_WorldLeader", "GHOST_Friend",
    "Writing song metadata to parse", "Writing playlist entry to parse",
    "Failed to communicate with parse server to delete playlist."

These confirm the flows: ghost upload with user confirmation and a retry queue,
ghost download before a race, playlist sync.

## 6. The Parse SDK

`tools/05_parse_sdk_endpoints.py` pulls UTF-16 strings out of `Parse.dll`. The SDK
is the standard open-source Parse .NET SDK (1.7), which tells us the exact wire
format without any guessing:

    classes/{0}            POST   create object
    classes/{0}/{1}        GET/PUT/DELETE
    classes/{0}?{1}        GET    query  (where, order, limit, skip, count, keys, include)
    files/{0}              POST   upload file (raw body) -> {"name":..., "url":...}
    batch                  POST   {"requests":[{method, path, body}, ...]}
    login?{0}              GET    user login
    events/AppOpened       POST   telemetry

    Headers: X-Parse-Application-Id, X-Parse-Windows-Key,
             X-Parse-Installation-Id, X-Parse-Session-Token, ...

Two details matter later:

- **File upload** is a raw-body POST; the response `{"name","url"}` is stored by the
  SDK as a `{"__type":"File","name","url"}` value inside objects. Downloads are plain
  `GET` on that `url` - so the URL we return must be *reachable by the game*.
- **Updates** can carry atomic operations: `{"__op":"Increment","amount":1}`,
  `Add`, `AddUnique`, `Remove`, `Delete`.

## 7. Gap analysis

Comparing the SDK's needs with riffapi-local's answers:

| Game needs | riffapi-local did | Consequence |
|---|---|---|
| store uploaded ghost files | returned fake URL | ghost data lost |
| persist Ghost/Score objects | returned random objectId, stored nothing | nothing survives |
| answer queries (leaderboards) | always `{"results":[]}` | no rivals, ever |
| `PUT` with `__op` counters | ignored body | counters corrupt |
| `POST /batch` | 404 | multi-saves fail |
| `GET /login` | 404 | **game never gets online** (discovered later!) |
| serve files back by URL | `GET /files/` returned JSON | downloads impossible |

## 8. Storage layer

Design goals: zero dependencies beyond Flask, human-inspectable data, crash-safe
writes, trivially backuppable.

    data/
      classes/<Class>/<objectId>.json   # one file per object
      files/<name>                      # raw uploaded bytes
      requests.log                      # append-only traffic log

Key implementation choices (see `server/riffapi-local.py`):

- **objectId** = 10 random alphanumerics, exactly like Parse; uniqueness enforced
  against the class directory.
- **Timestamps** in Parse's ISO format `YYYY-MM-DDTHH:MM:SS.mmmZ`.
- **Atomic writes**: write to `<file>.json.tmp`, then `os.replace()` - a crash can
  never leave a half-written object.
- **A global `RLock`** around storage mutations; the Flask dev server runs
  threaded, and the game does fire parallel requests.
- **`safe_name()`** sanitizes class/file names into path components (the game
  controls these strings - never trust them on the filesystem).
- **Canned responses preserved**: `Configuration`, `News`, `Version` (and the
  `Profile_Steam` fallback while no real profile exists) keep the original server's
  exact bytes, because the game's boot depends on those values.

## 9. Query engine

The game's leaderboard queries look like:

    GET /classes/Score?where={"SongID":"<track>","Mode":0}&order=-Score&limit=1
    GET /classes/ghost_v18?where={"song":{"$in":["<id1>","<id2>"]}}&order=-score

So the server implements a compact but real subset of Parse queries:

- **where**: equality for scalars plus `$in $nin $ne $lt $lte $gt $gte $exists
  $regex $all`;
- **typed values**: `Pointer` compares by objectId, `File` by name, `Date` by iso
  string (`scalarize()` normalizes all three);
- **order**: comma-separated keys, `-` prefix = descending, stable multi-key sort,
  missing values sort first (Parse treats undefined as smallest);
- **paging**: `limit` (default 100), `skip`, and `count=1` returning the total
  match count alongside an empty page;
- **keys/include**: field projection and one-level Pointer expansion.

Everything is file-system backed: a query is "load all JSON files of the class,
filter in memory". For a local game server (hundreds of objects) this is instant
and infinitely simpler than embedding a database.

## 10. Files, URLs and the host trick

`POST /files/<name>` stores the raw body and answers:

    {"url": "http://<host>/files/<name>", "name": "<name>"}

The `<host>` is taken from the request's `Host` header - because the game reaches
us as `127.000000000.0.1:5144`, the returned download URL points back at exactly
the same address, which the game's HTTP stack already knows how to resolve.
`GET /files/<name>` then serves the bytes with a guessed MIME type
(`application/octet-stream` for `.ghost`).

The original server's biggest bug is now obvious: it answered **every** `/files/*`
request - including the later GET downloads - with the same fake JSON.

## 11. Atomic operations and batch

`merge_update()` applies Parse's REST operations to stored objects:

    {"__op": "Increment", "amount": 5}   -> field += 5 (default 1)
    {"__op": "Add"/"AddUnique"/"Remove"} -> list operations
    {"__op": "Delete"}                   -> field removed

This matters because the game increments `Plays`/`Finishes` counters and the
`_Installation.opens` counter with these ops. `POST /batch` maps each
`{method, path, body}` entry onto the same internal helpers and returns per-item
`{"success": ...}` / `{"error": ...}` results.

One Parse nicety we reproduce: `_Installation` objects are **upserted** by their
`installationId` instead of duplicated on every game launch.

---

## 12. Live test #1: the silent game

With the storage server running, the game was played and a track finished...
and **nothing happened**. `data/requests.log` contained only our own curl tests.

Lesson one of working with a black box: *verify your assumptions about the
transport*. A quick scan of `ParseOctane.dll` showed it still contained
`https://parseapi.back4app.com` - **the DLL had never been patched** on this
machine. The game had been talking to a long-dead server the whole time.
After patching the DLL (same sed one-liner as the original project), the log
came alive.

## 13. Live test #2: the login wall

Now the game connected - and got stuck. The log showed a perfect heartbeat:

    [..] POST /events/AppOpened
    [..] POST /classes/_Installation        -> 200 (stored)
    [..] GET  /login?username=<login-hash>&password=<password-hash>
    [..] GET  /classes/Version              -> 200
    [..] GET  /login?...                    (404)
    ...                                     (repeat every ~5 s, forever)

The game performs a **Parse user login** before anything else, and retries every
`s_loginRetryTime` seconds when it fails. Neither the original riffapi-local nor
our first version implemented `/login`, so the game sat on "Waiting to connect to
Parse" and never reached ghost upload at all.

(The login username/password are 12-character tokens derived from the Steam ID -
the same values we had seen earlier while hex-dumping `online.bin`. The server
does not need to understand them; it just needs to answer.)

## 14. Implementing Parse auth

Parse's `GET /login?username=U&password=P` must return the user object:

    {
      "objectId": "...", "createdAt": "...", "updatedAt": "...",
      "username": "U", "sessionToken": "r:...",
      ...any custom fields...
    }

Our local twist: **auto-register** unknown users. Any credentials create a
persistent `_User` object; repeat logins return the same objectId (so the player
keeps their identity across sessions), each login mints a fresh `sessionToken`,
and the password is never echoed back. Two Steam accounts therefore become two
stable `_User` objects - exactly what cross-account testing needs.

Also added: `GET /users/me` (token lookup) and `POST /logout`.

## 15. Victory: reading real traffic

With auth in place the game flowed through the entire online lifecycle. Here is
the sanitized shape of what one two-player session stored:

    classes/ghost_v18/<id>.json     <- ghost metadata (the real class name!)
    {
      "user": "<playerId>", "userName": "PlayerOne",
      "song": "<trackId>", "score": 5424,
      "car": "alf", "pack": 0
    }

    classes/FileData/<id>.json      <- ghost file record
    {
      "name": "<trackId>-<playerId>.ghost", "username": "...",
      "data": {"__type": "File", "name": "...", "url": "http://127.0.0.1:5144/files/..."}
    }

    classes/Score/<id>.json         <- leaderboard entry
    { "Score": 10444, "Owner": "<playerId>", "SongID": "<trackId>",
      "Mode": 0, "Finishes": 3, "Version": 1, "Platform": "PC",
      "PlayerAlias": "PlayerTwo" }

    classes/Notification/<id>.json  <- created when PlayerTwo beat PlayerOne
    { "SongID": "...", "RecipientID": "<playerOne>", "SongName": "...",
      "ArtistName": "...", "PreviousScore": 1921, "NewScore": 4106,
      "NewLeaderName": "PlayerTwo", "Status": "Active",
      "FriendNotification": false }

    files/<trackId>-<playerId>.ghost   <- ~40 KB binary per run

Note how reality differed from the string-mining guesses in two fun ways:
the live ghost metadata class is called **`ghost_v18`** (versioned!), and ghost
binaries are recorded through a separate **`FileData`** class. Generic storage
made both non-events - no code change was needed to absorb discoveries like
these, which is exactly why the server implements Parse generically instead of
hardcoding guessed schemas.

Racing the same track again now downloads the stored ghost binary and puts your
previous run on the road. Switching Steam accounts shows both players on each
other's leaderboards, complete with an overthrow notification.

## 16. The admin GUI

Managing JSON files by hand gets old fast, so the server gained a web UI at
`/admin` (`server/admin.html`, vanilla JS, no dependencies):

- **Overview** - class/object/file counts, storage size;
- **Classes** - browse objects, view/edit full JSON, create (optionally with a
  chosen objectId - handy for importing a friend's ghost), delete;
- **Files** - list/download/upload/delete binaries;
- **Request log** - live tail of everything the game sends.

Admin endpoints live under `/admin/api/*` and reuse the same storage helpers;
destructive actions go through the same validation as game traffic.

## 17. Testing strategy

Three suites under `tests/`, all stdlib-only (`urllib`), all runnable against a
live server on 127.0.0.1:5144:

- **test_api.py** (28 checks) - canned boot responses still byte-identical; file
  upload/download round-trip byte-for-byte; Ghost create/query/order/$in/count/
  get/update/batch/delete; Profile_Steam fallback.
- **test_ops.py** - atomic operations: Increment (explicit + default amount),
  Add/AddUnique/Remove on arrays, Delete removing a field.
- **test_auth.py** - login creates a user, re-login returns the same objectId,
  second account gets a different user, `/users/me` resolves tokens, bad token
  is a 404.

Run them in order after any change:

    python tests/test_api.py && python tests/test_ops.py && python tests/test_auth.py

## 18. Layout and tool usage

    riffapi-re-tutorial/
      README.md                 <- this tutorial
      server/riffapi-local.py   <- the finished server (heavily commented)
      server/admin.html         <- the admin GUI
      tools/
        01_mine_strings.py          # keyword string extraction from binaries
        02_string_context.py        # neighbors of interesting strings
        03_identify_exe.py          # PE/.NET header forensics for Game.exe
        04_inspect_vfs.py           # list the game's asset archives
        05_parse_sdk_endpoints.py   # recover REST surface from Parse.dll
      tests/                    <- the three suites described above
      patches/
        patch_ops.py            # evolution step: __op support + merge_update
        patch_auth.py           # evolution step: /login, /users/me, /logout
        patch_admin.py          # evolution step: admin GUI endpoints

Tools assume the game at `C:\Games\Riff Racer` - adjust the paths at the top
of each script. They are intentionally dependency-free (pure `re`/`struct`) so
they keep working on any machine.

### Reproducing the analysis yourself

1. Run `03_identify_exe.py` on Game.exe - confirm mixed-mode C++/CLI.
2. Run `01_mine_strings.py` - skim the hits; then aim `02_string_context.py`
   at anchors like `Ghost`, `Uploader`, `PlaylistTracks`.
3. Run `05_parse_sdk_endpoints.py` on Parse.dll - write down the endpoint table.
4. Patch the DLL, start the server, play, and read `data/requests.log`:
   the game itself will teach you the rest (that is how `ghost_v18`,
   `FileData` and the login wall were found).

## Final remarks

The biggest lessons of this exercise:

1. **Watch the wire before you trust static analysis** - the login wall was
   invisible in strings but obvious in the first real log.
2. **Implement the protocol generically** - when the game used `ghost_v18`
   instead of the guessed `Ghost` class, zero code changed.
3. **Keep the original project's contract** - the canned boot responses from
   riffapi-local still work byte-for-byte; everything else is additive.

Have fun racing your own ghosts!
