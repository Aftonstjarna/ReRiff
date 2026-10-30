# dotnet452 and the original generator

Investigated 2026-10-30. The Mono crash is a reason to test Microsoft's .NET
Framework, not evidence that the original generator cannot run under Wine.

## Findings

- Original Game.exe has CLR metadata version `v4.0.30319` and contains both IL
  and x86 native code. This establishes the CLR family, not an exact minimum
  Framework release such as 4.5.2.
- The previous reflection probe ran in the new isolated
  `build/song-reference/wine-prefix`, which had Mono, not Microsoft .NET.
  It failed in native CRT initialization before `gaNodeManager.create` ran.
  It therefore did not exercise the earlier dotnet452 recommendation.
- The Steam prefix at
  `/var/home/afton/.local/share/Steam/steamapps/compatdata/351990/pfx` contains
  a historical installer log named
  `drive_c/users/steamuser/AppData/Local/Temp/Microsoft .NET Framework 4.5.2 Setup_20251202_145309629.html`.
  Its final result reports success on 2025-12-02. That historical success does
  not establish that Microsoft .NET is still present or selected.
- Currently neither the x86 nor x64 Framework v4 directory in that Steam prefix
  contains `clr.dll`. Its x86 `mscoree.dll` resolves to Proton 11's Wine DLL.
  Registry values report Framework 4.7.03190, but registry values alone are not
  proof of the loaded runtime. Current evidence does not show a usable native
  Microsoft CLR in that prefix; the reason its state changed is undetermined.
- Both the installed `/usr/bin/winetricks` and upstream's `load_dotnet452`
  explicitly remove Mono, install the dotnet40 prerequisite, install the
  Microsoft 4.5.2 package, and select native `mscoree`.
- Linux .NET 8 used for ILSpy is a separate runtime and does not install the
  Windows .NET Framework into a Wine prefix.

## Controlled test: passed

The follow-up installed Microsoft .NET 4.5.2 through Winetricks in isolated test
prefixes. A normal **win64 prefix works**; the game and probe remain x86 processes.
No change to the installed game, its Steam prefix or saved data was necessary.

| Test | Result |
| --- | --- |
| Proton 11.0-2 family, initial win32 diagnostic | Microsoft CLR verified; direct EXE library load rejected with `0x80131019` |
| Installed Proton 4.11-13, normal win64 prefix | .NET installer raised `0xc06d007e`; probe not reached; this does not test gameplay compatibility |
| Proton 11.0-2 family, normal win64 prefix | Microsoft CLR verified; original entity manager, node allocator and eight-node chain passed |

The fallback runner's exact version file is `1791559474 proton-11.0-2e-x86_64`.
The successful prefix is `build/song-reference/dotnet452-proton1102-win64`.
It reports Framework `4.5.51209`. Runtime verification reported:

```text
MONO_RUNTIME=False
CLR_VERSION=4.0.30319.34209
POINTER_BYTES=4
LOADED_CLR=C:\windows\Microsoft.NET\Framework\v4.0.30319\clr.dll
```

Direct `Assembly.LoadFile(Game.exe)` is not a viable host for this mixed-mode EXE:
Microsoft CLR rejects it as an unverifiable EXE with fixups. The diagnostic uses
a disposable `Game.reference.dll`, with the DLL header bit set and both native
and managed process entry points cleared. Original method bodies are unchanged.
This is an isolated reference host, not a converted game or a full-game test.
`scripts/prepare-original-reference.py` reproduces the copy and records the exact
header changes and hashes; its output is restricted to ignored `build/`.

Proton's packaged `libvkd3d-1.dll`, `libvkd3d-shader-1.dll` and
`libvkd3d-utils-1.dll` were added to the isolated prefix's matching architecture
directories. These dependencies are needed by Wine's D3D import chain when using
its Wine binary directly rather than the full Steam/Proton launcher. No renderer
is initialized by the probe.

The first successful library load exposed a missing host prerequisite:
`gaNodeManager` registers an entity listener, so `gaEntityManager.create` must run
first. After adding that original initialization call, both constructors returned
and original `gaNodeManager.makeNodes(8,20)` produced exactly `(0,0,20*i)` for
nodes 0–7. The process exited successfully. No server connection was needed.

Evidence is retained in `analysis/native-port/dotnet452-runtime-test.json` and
`analysis/native-port/proton1102-win64-nodes.log`. Probe source is
`reference/OriginalRuntimeProbe.cs`; compiled host and installer logs stay in
ignored `build/song-reference/`. The installed Game.exe hash was rechecked and
remains the supported original hash.

This clears the runtime blocker for reference execution. The subsequent offline
song capture also succeeded: original `gaTrackGen.initFromJson` returned all 635
Rolling On nodes and 132 segments. Widths, walls, flags and visibility match exactly;
maximum position error is 0.0079461 world units against a 0.01 tolerance. See
[the capture procedure and scope](SONG-TRACKS.md).
Because Game.exe targets CLR v4, no legacy-v2 activation switch was added.

## ProtonDB reports checked

All 25 reports on the linked app page were retrieved from its public report
listing on 2026-10-30. A local snapshot is retained at
`analysis/native-port/protondb-351990-reports.json` (ProtonDB data is ODbL licensed).

- [2019-11-27, Proton 4.11-8](https://www.protondb.com/app/351990#hGhpF_cNe):
  explicitly reports a normal 64-bit prefix with dotnet452. A 32-bit-prefix
  conversion is not a prerequisite.
- [2020-03-07, Proton 4.11-13](https://www.protondb.com/app/351990#7HB9lQ5F3):
  reports smooth operation after installing dotnet452.
- [2024-06-11, GE-Proton9-5](https://www.protondb.com/app/351990#KivETiSDyC):
  distinguishes startup without dotnet452 from crashes on server responses,
  then reports success using dotnet452 plus the replacement-server patch.
  Since both were applied, this is not a controlled causal test of either fix.
- [2025-02-06, Proton 9.0-4](https://www.protondb.com/app/351990#EMX0CWDgRW):
  points to a local replacement server and describes a separate post-race freeze.

These are reports about complete gameplay under their specific configurations.
They support trying a standard 64-bit prefix and separating server-response
problems from CLR initialization; they do not certify the isolated reference host
or establish a universal .NET requirement for every Proton release.

## Primary references

- [Winetricks source, load_dotnet452](https://github.com/Winetricks/winetricks/blob/master/src/winetricks)
- [Mono C++/CLI and mixed-mode limitations](https://www.mono-project.com/docs/about-mono/languages/cplusplus/)
- [Microsoft CLR startup and legacy mixed-mode activation](https://learn.microsoft.com/en-us/dotnet/framework/configure-apps/file-schema/startup/startup-element)

Mono documents experimental mixed-mode support through Windows Mono/Wine and
specific CRT limitations; this is not a blanket claim that every mixed-mode
assembly necessarily fails under Wine Mono. The observed failure is specific to
the tested original executable and probe/runtime combination.
