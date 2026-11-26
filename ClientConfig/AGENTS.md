# Managed client helper

Scope: `ClientConfig/`, `Managed/`, `Shared/` and the managed parts of
`Patcher/` and `NewsHeaders/`. The injected native client is in
`ExperimentalMenu/`.

## Boundaries

- `ClientConfig` is a helper loaded beside the original game assembly. It must
  not reference `Game.exe` at build time. Bind original methods by the supported
  assembly identity, metadata token or verified hash, and keep the documented
  fallback behavior.
- `Shared/` is compiled into more than one consumer. A change there must build
  and test both the client and server sides that include it.
- `Progression/` is server balance policy. The client implementation lives in
  `ClientConfig/` and `ExperimentalMenu/`; coordinate policy changes with both.
- Do not add a new dependency for a single call site. Reuse the helper,
  serialization and logging utilities already in the tree.

## Contracts

- Preserve `client.ini` key names and semantics, save envelopes, ghost formats,
  archive/cache formats, receiver-account identifiers and installer receipts
  unless the task explicitly coordinates every consumer.
- Preserve the supported `Game.exe` identity checks and their failure paths.
  A mismatched assembly must produce the documented fallback, not a crash or a
  silent write.
- Never patch a car package in place (`strings/`, `customs_unlocks.txt` or
  configs). If composition fails, fix the composer or send the format change to
  RiffRacer-Customs.

## Build and test

Use the pinned SDK route:

```sh
python scripts/build-client-config.py --dotnet /path/to/dotnet \
  --framework-dir /path/to/net40 --original-dir /path/to/original
```

For a narrower check, build the affected project:

```sh
dotnet build ClientConfig/ClientConfig.csproj -c Release -o build/client-check
dotnet build ClientConfig/Tests/<Area>Smoke.csproj -c Release -o build/client-check
dotnet build/client-check/<Area>Smoke.dll
```

The authoritative matrix is `.github/workflows/client-checks.yml`. Run the
affected smoke projects and the protocol tests when a wire or checkpoint format
changes. Do not substitute a caveat for a runnable check.

## Where documentation goes

- Server, singleplayer and progression settings: wiki
  [Servers and singleplayer](https://github.com/Aftonstjarna/ReRiff/wiki/Servers-and-singleplayer)
  and [Progression and balance](https://github.com/Aftonstjarna/ReRiff/wiki/Progression-and-balance).
- Leaderboards and ghosts: wiki
  [Leaderboards and ghosts](https://github.com/Aftonstjarna/ReRiff/wiki/Leaderboards-and-ghosts).
- Last.fm, ListenBrainz and Libre.fm: wiki [Last.fm](https://github.com/Aftonstjarna/ReRiff/wiki/Last.fm).
- Search and library behavior: wiki
  [Music search and beta features](https://github.com/Aftonstjarna/ReRiff/wiki/Music-search-and-beta-features).

Do not create a feature Markdown file. Update the matching wiki page or the
existing internal note. Shared or cross-component work follows
`docs/agents/README.md`.
