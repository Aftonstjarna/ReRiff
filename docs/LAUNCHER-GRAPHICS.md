# Launcher graphics settings

Open **Graphics**, immediately after Servers. Choose settings and click **Save
settings**, then restart the game.

- **Resolution:** resolutions reported by Windows or Proton, plus the saved
  resolution. **Current game resolution** keeps the native startup resolution.
- **Display mode:** Windowed, Borderless or Exclusive fullscreen. **Current game
  mode** keeps the native saved fullscreen choice.
- **Anti-aliasing:** the same setting as the game's Graphics menu.
- **UI scale:** Automatic, or the game's 25–100 scale in steps of five.
- **Recent Tracks limit:** All tracks (default), 20 (original), 50 or 100. This
  controls the recent-song list across its sort tabs; bundled tracks are
  unaffected. It changes the displayed list, not saved song history.

The in-game **Graphics** menu also has a **Recent Tracks** row after UI scale.
Use its native arrows and **Apply**; restart the game to apply the limit.

Hover over a launcher control for its explanation. Settings share the existing
`client.ini` `[graphics]` keys: `render_width`, `render_height`,
`fullscreen`, `antialiasing`, `ui_scale` and
`recent_tracks_limit`. A limit of 0 means All tracks. Supported finite limits
are 20, 50 and 100; unsupported values fall back to All tracks.

`fullscreen=0` selects Windowed, `fullscreen=1` Borderless, and
`fullscreen=2` Exclusive fullscreen. Existing `borderless` and
`exclusive_fullscreen` values remain readable for compatibility; saving removes
those old keys. With no fullscreen override, the native game setting applies.
