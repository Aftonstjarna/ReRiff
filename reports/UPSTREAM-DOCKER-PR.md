# Fix Docker reachability and preserve data across container recreation

The current image starts the Flask development server bound to `127.0.0.1`
inside the container, so publishing port 5144 does not make it reachable through
the container's external interface. The current Compose file also leaves saved
records and uploads in the container's writable layer, where recreation loses
them unless the user has arranged a separate mount.

This patch serves the existing Flask app using one threaded Waitress process on
`0.0.0.0:5144` inside the image and stores data in a named volume at `/data`.
Direct `python riffapi-local.py` use retains the existing localhost binding and
adjacent `data/` directory unless `RIFFAPI_DATA_DIR` is explicitly set.

The container runs as UID/GID 10001 with a healthcheck. Compose adds a read-only
root filesystem, writable temporary storage, dropped capabilities, restart
policy, and a localhost-only published port by default. The README documents LAN
exposure and migration of an old container's `/app/data`. Existing data is not
automatically copied into a new volume.

The existing API/auth/operation tests gain an optional `RIFFAPI_TEST_URL` so they
can run against a disposable, automatically allocated host port. A new integration
test checks published-port reachability, admin/health access, non-root writes,
binary content and generated URLs, and scores/files after removing and recreating
the container using the same volume. A GitHub Actions workflow runs the test with
Docker; it does not publish or deploy an image.

Validation completed locally with Podman in Docker-compatible image format:

- Existing API suite: **28 passed, 0 failed**.
- Existing authentication and operation suites: **all passed**.
- New container integration: **passed**, including recreation with the same volume.
- Diff whitespace check: **passed**.

Commands:

```sh
docker build -t riffapi-local:test .
python3 docs/tests/test_container.py --image riffapi-local:test
```

Local validation used `--engine` with a Podman bridge. The branch also includes a
GitHub Actions job to verify the same integration test with Docker.

GitHub Docker CI also passed: https://github.com/Aftonstjarna/riffapi-local-extended/actions/runs/41171388969

This is a focused server/container contribution based on upstream
`d6b98d2845bfbc8ee2888f805f8f0dc36bb73ede`. It contains no ReRiff launcher,
game DLLs or patches, installers, client updater, song catalog, or release
workflows. Application authentication behavior is unchanged.
