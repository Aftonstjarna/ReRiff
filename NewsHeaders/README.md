# Server-version notices in the original News screen

Friends is replaced by Servers on the experimental menu branch. News retains
its original screen and action. The backend now serves stored normal News
entries through its ordinary Parse query handling rather than returning only
the fixed RiffApiLocal announcement.

The native policy in `ServerSync/news.hpp` removes the old `RiffApiLocalNews`
placeholder and supplies one pinned `ReRiffServerStatus` entry. When the
server's ReRiff version is numerically greater than the client's version, its
title is **Exit to update ReRiff**, and its message asks the player to exit,
update, and reconnect. It does not force an exit, download or install anything.
Equal/newer clients receive a neutral status. Missing/invalid versions receive
an explicit unknown status, with no invented update recommendation.

Other announcements, translations, object IDs and content are preserved.
The status entry has ID 0 so the original game's ascending ID/ten-entry query
includes it before ordinary positive-ID announcements. Its English fallback
text is supplied for all six language fields expected by the game. News still
supports normal filtering, ordering, pagination, projection and counts.
The legacy `/classes/Version` field `Ver:100` is unchanged: it is the original
game protocol value, not the ReRiff software release number.

## Version sources and transport

- Server version: `RERIFF_SERVER_VERSION`, otherwise `VERSION` beside the server,
  otherwise repository `VERSION` during development. Docker and release payloads
  now include the server's own VERSION file.
- Client version: `version` in its installed `localserver/install-state.json`,
  falling back to `localserver/client-version.txt` when no installation record
  exists. This follows subsequent normal client updates rather than retaining a
  stale version file. The server's version is read independently.
- The thin .NET Framework adapter sends `X-ReRiff-Client-Version` on the existing
  Parse transport's requests. It validates the version's header syntax, omits
  unreadable/missing/invalid metadata, and preserves the request's existing
  headers, destination and offline guard. Comparison and generated News content
  are implemented in C++, in a separate native News library. The existing song
  identity library is preserved.

Use three numeric components, each at most four digits. Development/prerelease
labels are deliberately treated as unknown rather than guessed as ordered releases.
Only a backend running this News policy can produce the notice. An unmodified
remote server continues returning its own News responses; the client does not
replace another server's announcements.

## Build and deployment boundary

The full client build (`scripts/build-client-config.py`) now also builds the
header adapter and applies its extra transport call after the existing offline
patch. To extend a copy of an existing patched transport without rebuilding its
other DLLs, use `scripts/build-news-headers.py --framework-dir ... --input ...`.
The helper is written to `build/ReRiff.NewsHeaders.dll`; the extended transport
is isolated in `build/news-headers/Parse.NetFx45.dll`.

Build server libraries with the existing `scripts/build-song-catalog.py`. The
News library is `libreriff-news.so` or `reriff-news.dll`. Deploy it with
`news_adapter.py`, the News query hook and the correct server VERSION, then restart
the backend. Existing player data and the song identity library are preserved.
The original menu/EXE requires no additional patch for this News response.

The coordinated client/menu setup is `ExperimentalMenu/setup_client.py`. It
works from vanilla or existing ReRiff DLLs and installs the new helper, extended
transport and accurate client-version file together. It also starts the engine
menu independently of the popup setting. It has been installed locally.

`ExperimentalMenu/setup_news.py --game-dir ...` adds the backend extension with
verified backups, preserving unrelated backend code and the existing song library.
It has also been installed locally; the running backend needs a restart to load
the new code. Both tools support `--restore` and `--dry-run`. All active backups
and restore records are under the game's **bak** folder. No remote server was
deployed or restarted by this work.

Existing update-only installers deliberately do not rewrite Parse DLLs and pin
the supported SDK hash. They cannot introduce this new transport call into an
older client, and an experimental SDK replacement would fail their current
prerequisite check. A full supported client installation/migration is needed
before relying on version reporting; the new coordinated setup provides that
local migration, while copying only the helper is insufficient.
This branch does not publish a release or loosen those installer checks.

## Verification

Inside development, six real Flask/native-policy tests cover newer/equal/newer-
client and invalid versions, numeric ordering, placeholder removal, ordinary
news preservation, Parse filtering/counts and unchanged protocol Version.
Eight existing song-alias tests pass with the rebuilt native library.

The .NET tests exercise the actual extended Parse transport over loopback HTTP,
checking the reported version and preserved custom/cache headers, invalid/missing
metadata, and the original offline guard. They do not establish live Game.exe/
Proton News rendering; that remains a game trial after coordinated deployment.

Matching reconstruction remains 64,760 / 8,635,392 bytes (0.749937%). These mods
receive no matching byte credit.
