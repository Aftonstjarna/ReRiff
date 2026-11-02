# Shared scores and ghosts across cached song IDs

Two clients can assign different song IDs to exactly the same source audio.
Previously, both clients could upload scores successfully but see separate
leaderboards. This extension links registered IDs when the source-file SHA-256
matches. Existing clients and unregistered IDs keep their original behavior.

It does not merge databases, discover other servers, transfer client caches,
recover lost records, or use title/artist matching to guess identity. It includes
no game files or client modifications.

## Enable registration

The Docker build includes the catalog library. Direct Python installs retain
all existing routes without it, but registration returns HTTP 503 until built.
On Debian/Ubuntu:

```sh
sudo apt-get install g++ make nlohmann-json3-dev
make -C native
python riffapi-local.py
```

The registry uses C++17 and the nlohmann JSON library; Python is its HTTP/storage
adapter. On Windows, build `native/song_catalog.cpp` as `reriff-catalog.dll` with
a C++17 compiler and nlohmann headers. Place the library beside the server, or
set `RIFFAPI_CATALOG_LIBRARY` to its absolute path (`RERIFF_CATALOG_LIBRARY` is
also accepted for compatibility). The native source retains its MIT license in
`native/LICENSE.ReRiff`; upstream server licensing is unchanged.

`RIFFAPI_DATA_DIR` optionally selects the data folder; the default remains the
adjacent `data/`. Keep **all** data, including `song-catalog.json`, when migrating
or restoring backups. The existing Docker storage/exposure configuration is
otherwise unchanged by this contribution.

## Client handshake

A compatible client or registration utility sends the original cached ID and
SHA-256 of the complete source file before querying shared scores/ghosts:

```http
POST /reriff/songs/register
Content-Type: application/json

{"songs":[{"song_id":"cached-first","sha256":"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef","title":"Example","artist":"Example"}]}
```

Send each client's own ID with the same hash for identical audio. Maximum request
size is 8 MiB and at most 10,000 registrations per request. Titles and artists
are display metadata only; local paths and audio bytes are never required.
The server atomically writes the registry under the existing storage lock.
Invalid/conflicting registration does not change the registry. An ID previously
registered to different audio returns HTTP 409; invalid request structure returns
400. The endpoint follows the server's existing access model; hashes are client
claims, **not** verified audio fingerprints, authentication or anti-cheat.
Do not expose this server publicly without suitable access controls.

Different encodings or changed file tags produce different hashes and are not
automatically linked. A stock client does not send this handshake; merely
installing the server patch cannot infer missing source identities. ReRiff is one
compatible client; other tools can implement the same small request format.

## Read behavior and retained history

- Score, versioned ghost and FileData queries resolve registered equivalent IDs.
  Owner/friends filters, sorting, pagination and counts still operate normally.
- Leaderboards count a player's best result once per canonical song/mode/version.
  Stored duplicate records remain available by their original object IDs.
- Upload filenames, original song IDs, object IDs and file URLs are retained.
  Ghost/track file lookup can fall back to an equivalent registered ID.
- Other classes and the existing canned News/Configuration/Version responses
  remain unchanged.

An optional `song-aliases.json` supports explicitly verified ID groups:

```json
[["cached-first", "cached-second"]]
```

Groups must be disjoint, contain at least two unique IDs, and use only letters,
digits, `_` and `-`. Removing this file removes those manual equivalences; it
does not remove independently registered source identities or rewrite records.

## Checks

```sh
python -m unittest discover -s docs/tests -p test_song_sync.py
python docs/tests/run_api_suites.py
```

The HTTP runner uses a temporary data folder and an automatically allocated
localhost port. It does not write to a running user's data directory. The new
suite covers shared sorting/counts/friends filters, retained records and updates,
manual mapping reload/validation, exact-source registration, different-source
isolation, conflict rollback, original ghost bytes and duplicate ranking.
