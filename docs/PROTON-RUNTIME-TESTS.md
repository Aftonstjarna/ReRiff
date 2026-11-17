# Proton runtime tests

Tested on November 16, 2026. **An additional Microsoft .NET 4.5.2 install was
not needed for the successful stock Proton Experimental test.**

## Results

| Proton build | Prefix and runtime | Result |
| --- | --- | --- |
| Proton 10.0-4b | Fresh prefix; bundled Wine Mono 10.4.1 | Reached menus and connected to the local server, but stalled before song generation finished. |
| GE-Proton11-6 | Clone of the working Steam 351990 prefix; loaded Wine Mono 11.2.0 | User confirmed successful generation and arrival at the race screen. |
| Stock Proton Experimental `experimental-11.0-20261105c-x86_64` | Fresh prefix; loaded Wine Mono 11.3.0; Steam-declared prerequisite commands | User confirmed the same track reached the race screen. |

The test track was Adele’s “Someone Like You”. Server logs for the successful
runs show the FileData upload completing beyond the point where the Proton 10
run stalled. These are generation tests, not long-session stability tests.

## What Steam installs

Steam app 351990 declares shared prerequisites for Visual C++ 2013, DirectX
June 2010 and .NET Framework 4.5.2 (depots 228985, 228990 and 229004).
In the fresh Experimental prefix, Proton already set the .NET 4.5.2
`HasRunKey` completion marker. Steam's prerequisite logic therefore considers
that installer complete; the game actually loaded Wine Mono 11.3.0.

We reproduced the declared Visual C++ x86/x64 and DirectX installer commands;
all returned exit code 0. We honored the existing .NET completion marker and
did not force the Microsoft installer or use Winetricks. **The Steam client’s
first-run dispatcher itself was not exercised.**

A .NET version or prerequisite marker in the registry alone does not prove
Microsoft .NET is installed. The working-prefix audit found Wine's builtin
`mscoree.dll` and loaded Mono 11.2.0, with no native Microsoft CLR 4 runtime.

## Game and server scope

The game was copied from the original Windows depot 351991, build 1687655,
manifest 8951408038967536554. File comparisons confirmed that only
`ParseOctane.dll` differed: a same-length server-address replacement directed
the original client to an unchanged [Rebane local server](https://github.com/rebane2001/riffapi-local).
No ReRiff client features were used in this comparison. Disposable prefixes
kept the installed game and its working prefix unchanged.

This demonstrates that the original client's tested generation/server path can
work without an extra Microsoft .NET install. It does not test every ReRiff
feature or restore the original defunct online service.

## Limits and interpretation

- These results do not establish the minimum working Proton or Mono version.
- The GE comparison changed both Proton and inherited prefix settings; it does
  not isolate Mono as the cause. The fresh Experimental success avoids relying
  on that working-prefix clone.
- A stream-seeking exception appeared in the failing Mono log, including during
  successful startup requests; it was not proven to cause the stall.
- Attempts to install Microsoft .NET into separate disposable prefixes failed
  during setup. There was no successful native-.NET A/B comparison.
- Reports of older setups needing `dotnet452` are not disproven for those builds.
  They do not establish a requirement for the Experimental build tested here.

Keep an already working Proton setup. For a new setup or the observed generation
stall, try stock Proton Experimental before adding .NET manually.
