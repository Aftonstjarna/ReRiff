# Automatic song identity

The original game caches server-assigned song IDs locally and can reuse them on
another server without uploading track metadata. Scores alone carry no source
identity, so a server cannot infer which old IDs refer to the same audio.

ReRiff 0.1.2 registers cached IDs with the source file's SHA256 when the native
launcher starts and again after the game closes. The server stores the registry
in `/data/song-catalog.json`, alongside existing scores and ghosts. Launch each
updated client once, with its original source audio accessible. New imports made
during gameplay register after exit. The registration sends IDs, hashes, title
and artist, never audio bytes, local paths, or Steam credentials.

`POST /reriff/songs/register` accepts:

```json
{"songs":[{"song_id":"cached-id","sha256":"64 hex characters","title":"Title","artist":"Artist"}]}
```

The C++ registry validates each manifest and groups IDs by exact file SHA256.
Conflicting reuse of an ID returns HTTP 409 and leaves the catalog unchanged.
Matching titles or metadata alone do not join tracks; different encodings or
files with edited tags have different hashes. Deleted or inaccessible source
files are skipped and recorded in `ReRiff-launcher.log`. Unsupported cache
versions are skipped without modifying the cache. The current reader supports
cache format 19 / analysis version 18, verified against the original writer.

All existing score and ghost records remain intact. Song-filtered score queries,
friend filters, ranking counts, ghost queries, FileData names and read-only file
lookups resolve equivalent IDs. Duplicate player records rank the best result
once per song/mode/version. Administrative queries without a song filter still
expose the underlying records. Original object IDs, filenames and URLs are
preserved for updates and downloads. Missing historical Track metadata is not
fabricated from the cache: this catalog recovers song identity, not lost assets.

The old optional `/data/song-aliases.json` equivalence groups still work for
migration. They are no longer needed once both clients register matching hashes.
Back up and remove an old manual mapping after verifying automatic registration
if you want the server to rely exclusively on file identity.

Update both the Docker image and each client's launcher. A container restart
alone cannot teach an old server or old launcher this protocol. Preserve the
existing data volume when recreating the server. The x64 local-server identity
libraries are also included in the Linux and Windows update installers.

## Verification

The native engine lives in `ServerSync/catalog.cpp`. The existing Python server
handles HTTP/storage through `song_catalog.py`; identity validation/grouping is
native C++. The Docker build compiles the engine in a separate build stage.
The standalone release build uses `scripts/build-song-catalog.py`.

In the development container:

```sh
RERIFF_CATALOG_LIBRARY="$PWD/build/server-sync/libreriff-catalog.so" \
  build/server-test-venv/bin/python scripts/test_song_aliases.py
```

`docker/smoke_test.py` verifies automatic mapping and shared scores across a
container restart with isolated storage. `scripts/test_song_sync_wine.py` tests
the real Windows cache reader, SHA256 and HTTP handshake in a disposable prefix.
