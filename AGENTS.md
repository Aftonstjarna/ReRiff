# ReRiff agent guide

ReRiff extends Riff Racer with a Windows launcher, an injected native client, a
managed client helper, a compatible server and release packaging.

## Read only what the task needs

Load the guides covering the files you change, plus any nested `AGENTS.md` in
those directories. Do not load every guide by default.

- [`docs/agents/README.md`](docs/agents/README.md) — wiki routing, joint-project
  rules, validation matrix and worktree/artifact policy.
- [`ExperimentalMenu/AGENTS.md`](ExperimentalMenu/AGENTS.md) — injected client,
  camera/culling, in-game UI and native game patches.
- [`ClientConfig/AGENTS.md`](ClientConfig/AGENTS.md) — managed helper, saves,
  progression, transports and search.
- [`NativeLauncher/AGENTS.md`](NativeLauncher/AGENTS.md) — launcher, installer,
  updater and Linux runtime.
- [`ServerSync/AGENTS.md`](ServerSync/AGENTS.md) — server compatibility, channel
  promotion and upstream restoration.
- [`packaging/AGENTS.md`](packaging/AGENTS.md) — release metadata, changelog,
  packaging and publication.
- [`reports/AGENTS.md`](reports/AGENTS.md) — decomp research handoff.
- [Developer guides](https://github.com/Aftonstjarna/ReRiff/wiki/Developers) —
  architecture, port, decomp and internal implementation notes.

User-facing behavior, setup and shared/cross-component project documentation
belong in the [ReRiff wiki](https://github.com/Aftonstjarna/ReRiff/wiki), not in
new repository Markdown. Component `AGENTS.md` files and `docs/agents/` are for
agent instructions and validation policy only. Internal developer documentation
lives in the wiki [Developer guides](https://github.com/Aftonstjarna/ReRiff/wiki/Developers).
Before creating a Markdown file, read the routing table in
`docs/agents/README.md`.

## Working rules

- Prefer the game's existing native solution. Inspect original functions,
  controls and behavior before adding a replacement.
- Trace the affected execution path before editing. Reuse existing utilities and
  conventions.
- Address demonstrated failures rather than speculative ones. Make the smallest
  complete change; keep unrelated refactors and formatting out.
- New abstractions or dependencies need a concrete consumer.
- Preserve user changes and unrelated files. Keep experiments, backups, logs,
  downloaded tools and build output outside the tracked tree. Check free space
  before large builds or captures.
- Use the repository's pinned toolchain and scripts. Do not modify global
  installs.
- Update affected callers when an interface changes. Preserve `client.ini`, wire
  protocol, save/ghost/archive, installer-receipt and
  `packaging/vanilla-reference.json` contracts unless the task coordinates the
  change.
- Comments explain constraints or non-obvious decisions, not task history.
- Documentation stays concise and factual. Update the existing relevant section
  instead of adding a new feature page. New documents need a durable purpose
  beyond recording the current task; link to authoritative code and
  configuration instead of duplicating them.
- Run the checks that validate the change and report what actually ran. Do not
  skip a check and add a caveat, and do not present inferred correctness or
  performance as measured.

## Contracts and publication

- Do not commit, push, tag, publish packages, move release channels or post
  GitHub comments unless asked.
- Use ReRiff's imperative commit style (`Add …`, `Fix …`, `Move …`) without an
  extended body.
- Preserve licensing and author attribution. Do not invent co-author credits.
- Ask the user to write the PR description. Do not reply to reviewers yourself.
- Keep this file short. Put task-specific detail in the component guides or the
  wiki and update it when the implementation changes.
