# ReRiff server container

For GitHub-hosted builds and pulling ready-made images, see [GitHub setup](GITHUB.md).

From the project root, build and start the container:

```sh
docker compose up -d --build
docker compose logs -f server
```

The API listens at http://127.0.0.1:5144 and the admin panel at
http://127.0.0.1:5144/admin. Stop any existing local backend using port 5144 before
starting the container. When the container is already running, the game launcher
recognizes it and reuses it instead of starting the Python backend in the game
folder. Set the client's fallback URL to `http://127.0.0.1:5144`.

The container binds to all interfaces internally, while Compose publishes only to
the host's loopback address by default. The API/admin UI has no hardened access
control. Keep the default for a local server; restrict any remote deployment to
trusted clients using your own network/access controls.

## Data and logs

The `reriff-data` named volume stores everything under `/data`:

- `classes/`: profiles, tracks, scores, and other Parse records.
- `files/`: uploaded ghosts and track binaries.
- `traces/`: structured, credential-redacted request/response logs.

`docker compose down` stops/removes the container but keeps the volume. Avoid
`docker compose down -v` unless you intend to delete the saved data. No game
executables, assets, DLL patches, or local player data are included in the image.
The server retains the backend limitations documented in RELEASE-NOTES.md.

The image uses a non-root user (UID/GID 10001), one Waitress process with eight threads,
and a healthcheck. Compose gives it a read-only root filesystem with writable data
and temporary storage. Do not run multiple independent backend processes against
the same data directory: the backend storage lock is process-local.

## Copy existing data into the volume

Stop the old backend before copying. First create the Compose volume/container:

```sh
docker compose create server
docker compose cp "/path/to/Riff Racer/localserver/server/data/." server:/data/
docker compose run --rm --no-deps --user 0 --entrypoint sh server -c 'chown -R 10001:10001 /data'
docker compose up -d server
```

This copies data; it does not remove the original. Back up the source directory
first. Uploaded file URLs can refer to the old server address, so keep the same
client-visible address when migrating or update those records through the admin
UI. The image never mounts the host game directory automatically.

## Different host port or remote address

Environment variables for Compose can be set in a project `.env` file:

```ini
RERIFF_PORT=5144
RERIFF_BIND_ADDRESS=127.0.0.1
# Optional origin to put in uploaded file URLs, for example behind an HTTPS proxy:
# RERIFF_PUBLIC_URL=https://riff.example.org
```

Use `RERIFF_BIND_ADDRESS=0.0.0.0` only when you intend to allow connections from
other machines. Configure the game's external URL with that host's actual IP or
hostname, not `0.0.0.0`. If a reverse proxy changes scheme/host, set
`RERIFF_PUBLIC_URL` to the client-visible HTTP(S) origin. The public URL setting
does not configure TLS or a reverse proxy by itself.

If you change the host port, match it in the client INI. The launcher's automatic
local-server check currently uses 5144; custom ports are for manually managed or
remote servers.

## Without Compose / Podman

```sh
docker build -t reriff-server:local .
docker run -d --name reriff-server --restart unless-stopped \
  -p 127.0.0.1:5144:5144 -v reriff-data:/data reriff-server:local
```

The Dockerfile also builds with Podman. Substitute `podman` for `docker` in those
commands. Existing host data supplied as a bind mount must be writable by
container UID 10001; named volumes avoid most host permission issues.

## Verify the image

```sh
python3 docker/smoke_test.py --image reriff-server:local
```

The test uses an isolated host port and disposable named volume. It checks admin
loading, health, persistent scores/ghosts across restart, credential redaction, and
public file URL configuration, then removes its test container and volume. Use
`--engine podman --image localhost/reriff-server:local` for Podman builds.
