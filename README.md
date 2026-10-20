# RiffApi Local Extended

## 🔖 Foreword — this is a fork

`riffapi-local-extended` is a fork of [rebane2001/riffapi-local](https://github.com/rebane2001/riffapi-local), the basic local server that restored online play for **Riff Racer** after the official servers were shut down. This fork extends it with a persistent Parse-compatible backend so the game's online features actually work: **ghost upload/download**, scores & leaderboards, overthrow notifications, user login, track & playlist persistence, and a web admin GUI.

### 🔧 Required: patch `ParseOctane.dll` (EXE/DLL patching)

The game client ships with `ParseOctane.dll` pointing at the dead official endpoint `https://parseapi.back4app.com`. Patch it so the game talks to `http://127.000000000.0.1:5144` instead:

- Find: `https://parseapi.back4app.com`
- Replace with: `http://127.000000000.0.1:5144` *(same length as the original; safe in any hex editor; the odd-looking IP is still 127.0.0.1)*

Command line (Git Bash / WSL / Linux):

```bash
sed -i "s/https:\\/\\/parseapi.back4app.com/http:\\/\\/127.000000000.0.1:5144/g" ParseOctane.dll
```

`ParseOctane.dll` sits in the Riff Racer game folder next to the executable. Without this patch the game cannot reach your local server.

### 🏃‍♂️ Condensed run instructions

1. Patch `ParseOctane.dll` as shown above.
2. Install Python 3.8+ and Flask: `pip install Flask`
3. Start the server: `python riffapi-local.py`
4. Launch the game; watch the console / `data/requests.log`.
5. Open <http://127.0.0.1:5144/admin> to manage stored data.

Docker alternative: `docker compose up --build`

> `data/` (ghosts, scores, logs) is gitignored and never published — back it up by copying the folder.

---

## 📖 Original README (rebane2001/riffapi-local)

The section below is the original `README.md` from [rebane2001/riffapi-local](https://github.com/rebane2001/riffapi-local), included for reference and attribution.

# RiffApi Local

This project is a basic local server for the [Riff Racer](https://store.steampowered.com/app/351990) game. Since the servers of the game have been shut down, it is now impossible to create and play new tracks even if you wish to play in single player - thus a server implementation is necessary.

## Requirements

- Python 3.6+
- Flask

(Or Docker)

## Usage

### Patching ParseOctane.dll

Patch `ParseOctane.dll` to use RiffApi Local changing "https://parseapi.back4app.com" with "http://127.000000000.0.1:5144".

You can do so with sed (`sed -i "s/https:\\/\\/parseapi.back4app.com/http:\\/\\/127.000000000.0.1:5144/g" ParseOctane.dll`), a hex editor, or by simply using the [website](https://riffapi.hobune.stream/).

### Running the API

There are multiple ways to run the server:

- Run with Python (recommended)
  1. Install [Python 3.6+](https://www.python.org/downloads/).
  2. Install [Flask](https://pypi.org/project/Flask/) with `pip install Flask`.
  3. Run `riffapi-local.py`.

- Run with Docker.
  1. Install Docker.
  2. Run with `docker compose up --build`.

**Important**: keep the command prompt open until you exit the game.

## riffapi.hobune.stream

A more advanced online server with historic data is available at [riffapi.hobune.stream](https://riffapi.hobune.stream/). This server implementation here is for those wishing to run their own local server or in case the hobune server ever goes down.

---

## 🚀 Extended fork — AI-written documentation (significant changes)

A local server for the 2015 music racing game [Riff Racer](https://store.steampowered.com/app/351990) that restores the online features lost when the official servers were shut down - most importantly **ghost upload/download**, so your best runs become rivals again.

This is an extended fork of [rebane2001/riffapi-local](https://github.com/rebane2001/riffapi-local), which made the game playable again after the shutdown. That project faked the minimum API responses needed to boot the game; this fork adds a persistent Parse-compatible backend so the game's online features actually work.

### What works

- **Ghost sync** - after finishing a race the game uploads your run's ghost binary and metadata; racing a track again offers your saved runs as rivals.
- **Scores & leaderboards** - per-track `Score` objects with owner, alias and finish counters.
- **Overthrow notifications** - beating a stored score creates a `Notification` object, just like the live service did.
- **User login** - the game's Parse login is answered with auto-registered per-Steam-account users.
- **Track & playlist persistence** - shared track metadata, playlists and profile changes survive restarts.
- **Web admin GUI** - browse/edit/delete everything the game stores at <http://127.0.0.1:5144/admin>.

### Requirements

- Python 3.8+ (or Docker)
- Flask (`pip install Flask`)

### Quick start

1. Patch `ParseOctane.dll` in the game folder (see the **EXE / DLL patching** section in the Foreword above), replacing

       https://parseapi.back4app.com

   with

       http://127.000000000.0.1:5144

2. Start the server:

       pip install Flask
       python riffapi-local.py

3. Launch the game. Watch the console (or `data/requests.log`) to see it connect, log in and sync.

4. Open <http://127.0.0.1:5144/admin> to manage stored data.

#### Docker

    docker compose up --build

### Where data lives

    data/
      classes/<ClassName>/<objectId>.json   # every stored Parse object
      files/<name>                          # uploaded binaries (ghosts, tracks)
      requests.log                          # full log of game API traffic

Back up or move your ghosts/scores by copying the `data/` folder, or export/import individual objects through the admin GUI.  
**Note:** `data/` is gitignored and never published — it contains your personal ghosts and scores.

### How it works

The game talks the standard [Parse REST API](https://docs.parseplatform.org/rest/guide/) through its bundled .NET SDK (`Parse.dll`, patched `ParseOctane.dll`). The server implements the subset the game uses:

| Endpoint | Purpose |
|---|---|
| `POST /files/<name>` / `GET /files/<name>` | store and serve binaries (ghosts ~40 KB each) |
| `GET/POST /classes/<Class>` | query (where/order/limit/skip/count/keys/include) and create objects |
| `GET/PUT/DELETE /classes/<Class>/<id>` | fetch/update/delete objects |
| `PUT` with `{"__op": ...}` | atomic Increment/Add/AddUnique/Remove/Delete operations |
| `POST /batch` | batched operations |
| `GET /login`, `GET /users/me`, `POST /logout` | Parse user auth (auto-registers players) |
| `POST /events/AppOpened`, `_Installation` | telemetry the game expects |

Classes observed from the live game: `ghost_v18` (ghost metadata), `FileData` (ghost file records), `Score`, `Notification`, `Track`, `PlaylistTracks`, `Profile_Steam`, `_User`, `_Installation`.

Want the full reverse-engineering story — how the API surface was recovered from the game binaries, including all analysis tools and tests? See the companion project **riffapi-re-tutorial**.

### Credits

- [rebane2001](https://github.com/rebane2001) - original **riffapi-local** server; this project would not exist without it.
- ox-alpha (AI assistant) - persistence layer, auth, admin GUI, documentation.

License: The Unlicense (public domain) - see LICENSE.

