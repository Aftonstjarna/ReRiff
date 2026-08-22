# RiffApi Local Extended

A local server for the 2015 music racing game [Riff Racer](https://store.steampowered.com/app/351990) that restores the online features lost when the official servers were shut down - most importantly **ghost upload/download**, so your best runs become rivals again.

This is an extended fork of [rebane2001/riffapi-local](https://github.com/rebane2001/riffapi-local), which made the game playable again after the shutdown. That project faked the minimum API responses needed to boot the game; this fork adds a persistent Parse-compatible backend so the game's online features actually work.

## What works

- **Ghost sync** - after finishing a race the game uploads your run's ghost binary and metadata; racing a track again offers your saved runs as rivals.
- **Scores & leaderboards** - per-track `Score` objects with owner, alias and finish counters.
- **Overthrow notifications** - beating a stored score creates a `Notification` object, just like the live service did.
- **User login** - the game's Parse login is answered with auto-registered per-Steam-account users.
- **Track & playlist persistence** - shared track metadata, playlists and profile changes survive restarts.
- **Web admin GUI** - browse/edit/delete everything the game stores at <http://127.0.0.1:5144/admin>.

## Requirements

- Python 3.8+ (or Docker)
- Flask (`pip install Flask`)

## Quick start

1. Patch `ParseOctane.dll` in the game folder, replacing

       https://parseapi.back4app.com

   with

       http://127.000000000.0.1:5144

   (same length on purpose - it can be done in any hex editor; the odd-looking IP is still 127.0.0.1).

2. Start the server:

       pip install Flask
       python riffapi-local.py

3. Launch the game. Watch the console (or `data/requests.log`) to see it connect, log in and sync.

4. Open <http://127.0.0.1:5144/admin> to manage stored data.

### Docker

    docker compose up --build

## Where data lives

    data/
      classes/<ClassName>/<objectId>.json   # every stored Parse object
      files/<name>                          # uploaded binaries (ghosts, tracks)
      requests.log                          # full log of game API traffic

Back up or move your ghosts/scores by copying the `data/` folder, or export/import individual objects through the admin GUI.

## How it works

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

Want the full reverse-engineering story - how the API surface was recovered from the game binaries, including all analysis tools and tests? See the companion project **riffapi-re-tutorial**.

## Credits

- [rebane2001](https://github.com/rebane2001) - original **riffapi-local** server; this project would not exist without it.
- ox-alpha (AI assistant) - persistence layer, auth, admin GUI, documentation.

License: The Unlicense (public domain) - see LICENSE.
