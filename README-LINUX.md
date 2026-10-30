# ReRiff — Linux installation and use

## Install

1. Download the Linux installer (`ReRiff-<version>-linux.run`). Close Riff Racer
   and its launcher before installing.

2. Open a terminal in the folder containing the download and run the installer,
   replacing `<version>` with the downloaded release number:

   ```bash
   sh ReRiff-<version>-linux.run
   ```

   The installer searches common Steam libraries. If it cannot find the game,
   specify the folder containing `Game.exe`:

   ```bash
   sh ReRiff-<version>-linux.run --game-dir "/path/to/steamapps/common/Riff Racer"
   ```

   Python 3.10 or newer is required. Do not use `sudo`. Existing settings and
   saved server data are preserved.

## Configure Steam

1. In Steam, open **Riff Racer → Properties → Installed Files → Browse** to find
   the game's installation folder.

2. Open **Properties → General → Launch Options** and enter the following,
   replacing `/path/to/Riff Racer` with that installation folder. Keep the
   quotation marks, `--`, and `%command%`:

   ```text
   "/path/to/Riff Racer/localserver/steam-launcher.sh" -- %command%
   ```

3. Press **Play** in Steam. The Windows launcher runs under the game's selected
   Proton version and automatically starts or reuses the native Linux server.

4. For local play, click **Use local server**, then **Save and play**.

Keep the game's existing working Proton version and .NET setup. ReRiff does not
replace those dependencies or make the game Linux-native.

## Skip the settings window

After saving your settings, use these Steam launch options to start the server
and game without opening the launcher GUI:

```text
"/path/to/Riff Racer/localserver/steam-launcher.sh" --skip-launcher -- %command%
```

Do not put `ReRiff-Launcher.exe` directly before `%command%`: Linux must start it
through the Steam/Proton bridge above.

## Settings and server data

- Client settings are in `server.ini` beside `Game.exe`. Restart the game after
  editing them. Both server addresses blank disables the game's Parse networking.
- The local server and its saved records, ghosts, and logs are under
  `localserver/`. Closing the game leaves the server running.
- While the server is running, its admin panel is available at
  [http://127.0.0.1:5144/admin](http://127.0.0.1:5144/admin).
- Original DLL backups are kept outside the game folder in the ReRiff project
  backup directory. The installer reports the location. Use `--backup-dir` to
  choose another project backup folder.

## Restore original DLLs

Close the game and run the installer with `--restore`:

```bash
sh ReRiff-<version>-linux.run --restore --game-dir "/path/to/steamapps/common/Riff Racer"
```

Restore mode retains the INI, local server, and saved server data. Keep the backup
folder so the installer can restore the original DLLs.

## Troubleshooting

- If installation cannot find the game, supply `--game-dir` using the folder
  opened by Steam's **Browse** button.
- If the launcher does not open, check the quoted bridge path in Steam launch
  options and retain `%command%`.
- If the server does not start, check `localserver/server-console.log` and
  `ReRiff-launcher.log` beside `Game.exe`.
- If the notification is hidden in exclusive fullscreen, try borderless or
  windowed mode. It disappears automatically and does not require interaction.
