# Build and publish the server on GitHub

The workflow in `.github/workflows/container.yml` builds and tests the Docker
image on GitHub-hosted runners. No original game files, client DLLs, .NET SDK, or
local build artifacts are needed for the server build.

## Repository setup

1. Push this project to the chosen GitHub repository.
2. Check its **Actions** tab. The initial push triggers a build and container
   integration test, including score/ghost persistence across restart.
3. Pushes to the repository's default branch publish the tested image to
   `ghcr.io/<owner>/<repository>:latest` and a commit-specific `sha-...` tag.
4. Version tags beginning with `v` publish matching image tags, such as `v0.1.1`.
5. Pull requests and non-default branches build/test without publishing.

You can also run the workflow manually from the default branch using
**Actions → Build and publish server container → Run workflow**.

The workflow uses the repository's automatic `GITHUB_TOKEN` with package write
permission; it does not need a Docker Hub account or an additional registry
password. Organization policies may need to allow workflow package publishing.

GitHub Container Registry package visibility is configured separately from the
repository. Make the package public in its settings if you want anonymous pulls;
otherwise authenticate Docker to GitHub Container Registry before pulling.

## Use a remotely built image

Replace the owner and repository with the actual lowercase image path:

```sh
export RERIFF_IMAGE=ghcr.io/owner/repository:latest
docker compose pull server
docker compose up -d --no-build
```

Set `RERIFF_IMAGE` in `.env` to retain the choice for future Compose commands.
Use a version tag or digest instead of `latest` when you want a fixed build.
The existing named volume and localhost port mapping are retained.

No container is deployed to your computer or another server automatically.
GitHub builds/publishes the image; your host pulls and starts it when requested.

The workflow currently targets `linux/amd64`, matching the locally tested image.
ARM support can be added separately with platform-specific build/test coverage.
