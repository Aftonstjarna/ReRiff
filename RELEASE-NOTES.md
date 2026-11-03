# ReRiff 1.2.4-beta.1 — Display and input testing

## In-game summary

- Test corrected windowed clicks and fullscreen mode selection.
- Save prompts keep input focus while settings stay inactive.
- High-rate FLAC analysis uses the original 44.1 kHz resampler.

## Full release notes

Experimental test build. Display transitions, Alt-Tab, confirmation controls
and windowed pointer alignment need in-game testing on Linux and Windows.

- Restore the selected windowed client dimensions when leaving borderless.
- Map absolute windowed pointer coordinates to the actual render dimensions;
  retain native relative and normalized input paths.
- Remove overlapping fullscreen-label directional activation shortcuts.
- Keep added settings controls and scrolling inactive beneath native overlays.
- Normalize native activation messages and release capture on deactivation.
- Publish persistent server IDs and remember alternate server addresses.
- Add an explicit, backed-up one-time LAN-to-public file-link migration.
- Highlight each fullscreen arrow individually and keep its hitbox separate.
- Resample non-44.1 kHz FLAC analysis PCM with the original Ogg resampler.
  Source audio/playback remain unchanged; regenerate affected cached tracks.
