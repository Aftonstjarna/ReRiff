# Client-side server selection

The patched ParseOctane.dll calls ReRiff.ClientConfig.dll during network
initialization. It reads server.ini beside Game.exe, independently of the working
directory. The Python backend no longer chooses the destination in default mode.

```ini
[server]
external_url = https://your-compatible-server.example
fallback_url = http://127.0.0.1:5144
timeout_seconds = 5
show_server_popup = true
external_only = false
```

Restart the game after editing. Client settings do not require a server restart.
The notification shows the selected endpoint once per game process, or disabled
status. It appears over the game window for six seconds and fades automatically.
It has no buttons, does not activate or accept focus, and is click-through so
keyboard, mouse, and controller input stays with the game. It hides when the game
loses focus or is minimized. Set `show_server_popup = false` to turn it off.
Selection does not guarantee a later login or upload will succeed. This is a
non-activating Win32 overlay, not a hook into the game's renderer. Visibility in
exclusive fullscreen depends on Proton/compositor behavior; windowed or borderless
mode is the reliable option. There is no modal-dialog fallback if it cannot show.
The client probes the external /classes/Version endpoint once at initialization.
HTTP 200 with a JSON-looking results response selects external; otherwise it uses
the configured fallback. The probe is unauthenticated and does not follow
redirects. Backend selection stays fixed until game restart. Failed writes are
never retried against another backend. Independent databases are not synchronized.

Both addresses blank, missing/unreadable INI, or invalid settings disable Parse
network requests. Missing keys are blank. There is no hidden localhost default.
The patched Parse.NetFx45.dll transport guard returns a failed network task before
request creation when disabled, including SDK calls before normal initialization.
This does not disable unrelated Steam traffic.

Set `external_only = true` to use only the external URL. The launcher then skips
local backend startup and the client ignores the fallback address. If the external
server is unavailable or blank, Parse networking is disabled for that session.

Both settings accept HTTP(S) hosts, IPs, custom ports, and optional base paths.
Credentials, query strings, and fragments in base URLs are rejected. The selected
server receives game traffic including login headers and bodies. If the fallback
is localhost, start the local server before launching the game.

ReRiff-client.log beside Game.exe records endpoint selection and reasons. Detailed
local server traces only see requests actually directed to the local server.
External traffic goes directly to the external server. The local admin panel
continues to manage local data. External file URLs must be reachable from clients;
there is no relay rewriting with the direct client patch.

## Install and restore

The supported DLL hashes are checked and originals saved as
original DLL files in the ReRiff project backups/client folder. Existing server.ini settings are preserved.

```sh
python3 scripts/install-client-config.py --game-dir '/path/to/Riff Racer'
python3 scripts/install-client-config.py --game-dir '/path/to/Riff Racer' --restore
```

Restoring returns both DLLs to their original versions. Steam verification may
replace the patches; rerun installation afterward. Setup also installs the patch.

To rebuild, supply a .NET 8 SDK, .NET Framework reference assemblies (mscorlib.dll
and System.dll), and an unmodified copy of ParseOctane.dll and Parse.NetFx45.dll:

```sh
python3 scripts/build-client-config.py --dotnet /path/to/dotnet --framework-dir /path/to/references --original-dir /path/to/originals
```

The patcher uses dnlib NativeWrite for the mixed C++/CLI DLL. The helper targets
.NET Framework. The modified Parse platform assembly retains its original assembly
identity but cannot be re-signed with Parse's private key. Runtimes enforcing
strong-name verification may reject it; this still needs game-runtime validation.

Verified: endpoint selection; empty/invalid/missing config; offline gating through
the actual patched SDK on .NET 8; native DLL architecture, exports, and export
jump instructions. Full Game.exe/Proton behavior requires a restart and live test.
Legacy relay mode remains available explicitly with trace-server.py --relay for
installations without the client patch. Do not combine it with this client patch.
