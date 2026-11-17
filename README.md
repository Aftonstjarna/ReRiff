# ReRiff

Play Riff Racer on Windows or Linux/Proton with singleplayer, shared
leaderboards and ghosts.

[Download stable](https://github.com/Aftonstjarna/ReRiff/releases/latest)
· [Try the beta](https://github.com/Aftonstjarna/ReRiff/releases/tag/v1.4.0-beta.10)
· [Wiki and guides](https://github.com/Aftonstjarna/ReRiff/wiki)
· [Get help](https://github.com/Aftonstjarna/ReRiff/issues)

- Play solo offline or connect to a server for shared leaderboards and ghosts.
- Start a local server from the launcher to share with other PCs on your LAN.
- Add Chinese, Japanese, Korean and Latin/European characters with optional fonts.
- Adjust fullscreen mode, resolution, antialiasing and UI scale in-game.
- Get stable updates by default, with optional beta releases.

![ReRiff launcher with server settings](docs/images/reriff-launcher.png)

Beta.12 launcher shown. Hover over checkboxes for explanations.

## Install

You need an installed copy of **Riff Racer**. Close the game and launcher first.
Your settings and saved data are kept. Keep the game's `.bak` folder for restoring
original files.

**Windows:** download and extract `ReRiff-<version>-windows.zip`, run
**ReRiff-Setup.exe**, and choose the game's **Game.exe**. Keep the extracted files
together until setup finishes. You do not need to install Python manually.

**Linux/Proton:** download `ReRiff-<version>-linux.run`.

1. Right-click the downloaded file and open **Properties**.
2. Under **Permissions**, enable **Allow executing file as program** (the wording
   varies by desktop), then close Properties.
3. Double-click the file and choose **Run** if prompted.
4. Choose the folder containing **Game.exe**, then click **Install**. Leave
   **Link music folder** enabled to make your Linux music folder available in-game.

In Steam, right-click **Riff Racer → Properties → General → Launch Options** and enter:

```text
"/path/to/Riff Racer/ReRiff-Launcher.sh" -- %command%
```

Replace `/path/to/Riff Racer` with your actual game folder, keeping the quotes
and `-- %command%`.

Keep your working Proton configuration. Stock Proton Experimental passed our
song-generation test without a manual .NET install; see the
[Proton test findings](https://github.com/Aftonstjarna/ReRiff/wiki/Proton-runtime-tests).

[Detailed installation guide](https://github.com/Aftonstjarna/ReRiff/wiki/Installation)
· [Launch through Steam](https://github.com/Aftonstjarna/ReRiff/wiki/Steam-launch-options)

## Play and update

Open **ReRiff-Launcher.exe** beside the game, or use the Steam launch instructions
above. Set your preferred server in the launcher and click **Play**.

**Force singleplayer** skips servers. With it unchecked, ReRiff still uses
singleplayer automatically if no configured server is available.

To update, close the game and click **Update from GitHub**. Stable is the default;
check **Use experimental releases** to try betas.

[Server and singleplayer guide](https://github.com/Aftonstjarna/ReRiff/wiki/Servers-and-singleplayer)
· [Graphics settings](https://github.com/Aftonstjarna/ReRiff/wiki/Graphics)
· [Language fonts](https://github.com/Aftonstjarna/ReRiff/wiki/Language-fonts)

## Current beta

[1.4.0-beta.10](https://github.com/Aftonstjarna/ReRiff/releases/tag/v1.4.0-beta.10)
includes whole-library music search, editable server addresses, experimental
prestige progression and Last.fm scrobbling with protected credentials.

In the music picker, press **Ctrl+F** or **Xbox Y** to search, including music you
haven't imported. **Esc/B** clears search before leaving the picker.

![Music search in the picker](docs/images/reriff-music-search.png)

Steam virtual-keyboard input remains unresolved in the tested Proton setup;
use a physical keyboard with Steam's keyboard closed. M4A playback is not supported.

[Music search guide](https://github.com/Aftonstjarna/ReRiff/wiki/Music-search-and-beta-features)
· [Set up Last.fm](https://github.com/Aftonstjarna/ReRiff/wiki/Last.fm)
· [Full release notes](https://github.com/Aftonstjarna/ReRiff/releases)

## Help

For update failures, open **Diagnostics → View update details**. For other issues,
enable diagnostic logging, reproduce the issue, and review the logs before sharing.
Never share your `client.ini`, which can contain account credentials.

[Troubleshooting](https://github.com/Aftonstjarna/ReRiff/wiki/Troubleshooting)
· [Restore original files](https://github.com/Aftonstjarna/ReRiff/wiki/Restoring-original-files)
· [Report an issue](https://github.com/Aftonstjarna/ReRiff/issues)

Hosting a server or contributing code? See the wiki's
[server administration](https://github.com/Aftonstjarna/ReRiff/wiki/Server-administration),
[build and release](https://github.com/Aftonstjarna/ReRiff/wiki/Building-and-releasing),
and [original-game research](https://github.com/Aftonstjarna/ReRiff/wiki/Original-game-research)
guides.

ReRiff is MIT licensed. Bundled dependencies retain their own licenses; see
[LICENSE](LICENSE) and the [build guide](https://github.com/Aftonstjarna/ReRiff/wiki/Building-and-releasing#repository-history)
for backend attribution.
