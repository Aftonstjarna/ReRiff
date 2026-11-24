# Injected native client

Scope: `ExperimentalMenu/` and the native hooks it installs into `Game.exe` and
the launcher's shared headers. Managed helpers live in `ClientConfig/`.

## Extend the original game first

- Prefer the game's existing native solution. Inspect its original functions,
  controls and behavior before adding a replacement implementation.
- Copy, reuse or extend a suitable original function instead of creating a
  separate system for the same job. Preserve its native input handling,
  navigation, repeat behavior, artwork and lifecycle where applicable.
- Add a new implementation only when no suitable original solution exists;
  document that gap and keep the integration consistent with the game.
- For every new menu or menu addition, retain native spacing and control
  styling. Check the available space at supported resolutions and UI scales;
  reuse the native scrolling solution when entries overflow. Do not compress
  entries or allow controls to crowd, overlap, or become unreachable.

## Native patches and provenance

- Consult the RiffRacerDecomp recovery project for reverse-engineering work and
  contribute verified new findings there. Preserve original hashes, exact bytes,
  metadata tokens and provenance; keep intentional ReRiff patches separate from
  byte-for-byte original reconstruction and do not claim matching without
  comparison.
- Keep patch sites, fallback behavior and validation in the code and in the
  matching wiki Research page. Do not describe a patch as original behavior.
- Preserve the original executable and assembly contracts. A patch that changes
  an IL body, method RVA or metadata token needs the affected callers and
  unload/error paths checked.

## Build and test

```sh
python scripts/build-windows-launcher.py --zig /path/to/zig --component client --output-dir build/client
```

Use the pinned Zig from `packaging/fetch_release_tools.py` or the repository's
release-tools directory. The command above names the compiler explicitly.

Compile and run the native regression that covers the changed header. The
authoritative list is in `.github/workflows/client-checks.yml`; a typical local
check is:

```sh
zig cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror \
  ExperimentalMenu/test_<area>.c -o build/client/test_<area>.exe \
  -lcomctl32 -lcomdlg32 -lwininet -lshell32 -luser32 -lgdi32 -ladvapi32 -static
```

Run the executable under Wine or on Windows. Also build and run
`NativeLauncher/test_lastfm.c` when the change touches a shared launcher header.
Do not skip a test and report only a caveat.

## UI wording

- Name controls by their function. Button, checkbox, row and menu text is the
  action or setting name, not a sentence explaining it. Tooltips give the
  setting's scope, values and restart requirements; explanations belong on the
  wiki.

## Where documentation goes

- Graphics, culling, free look and photo mode: wiki
  [Graphics](https://github.com/Aftonstjarna/ReRiff/wiki/Graphics).
- Original-game reconstruction and patch findings: wiki
  [Original game research](https://github.com/Aftonstjarna/ReRiff/wiki/Original-game-research)
  and the [Research notes](https://github.com/Aftonstjarna/ReRiff/wiki/Research).
- In-game settings and controls: wiki
  [Launcher and in-game settings](https://github.com/Aftonstjarna/ReRiff/wiki/Launcher-and-in-game-settings).
- Search, music and progression UI: the matching wiki page.

Do not add a repository Markdown file for a feature. Update the wiki page or the
existing internal note. Shared or cross-component work follows
`docs/agents/README.md`.
