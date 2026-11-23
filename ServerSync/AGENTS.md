# Server compatibility and promotion

Scope: `ServerSync/`, `docker/`, `vendor/riffapi-local-extended/`,
`Progression/` balance policy and the server-facing parts of `Shared/`.

## Contracts

- Preserve the wire protocol, authentication flow, discovery behavior, object
  identifiers, checkpoint/ghost formats and container channel names unless the
  task coordinates every client and server consumer.
- A protocol change is a joint project. Update the client and server in the same
  change and follow `docs/agents/README.md`.
- Keep server data, logs and local credentials out of the tracked tree.

## Server channel promotion

- Server experiments are developed on `reriff/experimental-server`. A push there
  builds and publishes the `experimental` container image and does not move
  `latest`.
- When a stable client update depends on experimental server changes, promote
  both together: merge or fast-forward the tested stable branch into
  `reriff/main` and push. The `reriff/main` push publishes the `latest` server
  image, and the release workflow publishes the stable client. Do not promote
  server experiments to `reriff/main` ahead of a stable client release without
  explicit approval.
- After promotion, merge `reriff/main` back into `reriff/experimental-server` so
  the next experiment starts from the released server image. Keep
  `reriff/experimental-client` based on `reriff/main` for the next client beta.
- `latest` and `reriff/main` describe the same released server. `experimental` is
  the optional, movable preview channel. Tags are produced by
  `.github/workflows/container.yml`; do not publish these channels by hand.

## Upstream server restoration after stable releases

- After publishing a stable release, identify changes in that release that
  restore original server functionality. Isolate those changes on the existing
  `Server-Restoration` branch of `Aftonstjarna/ReRiff` for contribution to
  `rumbadebu/riffapi-local-extended`.
- Base the contribution on current upstream code and retain the existing focused
  restoration work. Do not merge the full ReRiff reriff/main history into this
  branch. Exclude launcher, installer, client/game patches and unrelated ReRiff
  features; include server dependencies, tests and documentation needed by the
  restoration.
- Run focused restoration regressions and upstream compatibility checks before
  submitting. Explain protocol/client prerequisites and any new dependencies.
- Split changes that restore different functions or fix unrelated problems into
  separate draft upstream PRs. Group only tightly related changes, their
  required dependencies, documentation and tests. Each PR should explain one
  concrete restoration and its validation.
- Keep `Server-Restoration` as the restoration integration branch. Each separate
  PR needs its own focused branch, such as `Server-Restoration-<topic>`, based on
  upstream so its diff excludes unrelated restoration work. Do not rename or
  delete an existing PR's source branch to create these separate contributions.
- Update an open PR only for changes to that same restoration, retaining its
  existing review state. For a different restoration, or after its previous PR
  has closed or merged, create a separate draft PR. Do not mark a new draft ready
  or merge it without user instruction.
- This standing instruction authorizes pushing the isolated branch and creating
  or updating its upstream PR as part of a stable release; do not ask again.
  Beta-only work waits until it reaches stable. If a release contains no original
  server restoration changes, skip the upstream contribution.

## Build and test

Use the container workflow and the protocol tests. Local checks include:

```sh
python -m pip install -r packaging/requirements.txt
python scripts/test_multiplayer_transport.py --client-command <client smoke exe>
python scripts/test_progression_discovery.py --client-command <client smoke exe>
```

The authoritative matrix is `.github/workflows/container.yml` and the server
steps in `.github/workflows/client-checks.yml`. Do not claim compatibility
without running the consumer that exercises the wire format.

## Where documentation goes

- Server administration, Docker, LAN and host networking: wiki
  [Server administration](https://github.com/Aftonstjarna/ReRiff/wiki/Server-administration).
- Server settings, singleplayer and progression transport: wiki
  [Servers and singleplayer](https://github.com/Aftonstjarna/ReRiff/wiki/Servers-and-singleplayer).
- Progression policy and balance: wiki
  [Progression and balance](https://github.com/Aftonstjarna/ReRiff/wiki/Progression-and-balance).
- Release channels and repository history: wiki
  [Building and releasing](https://github.com/Aftonstjarna/ReRiff/wiki/Building-and-releasing).

Do not create a server feature Markdown file. Update the matching wiki page.
