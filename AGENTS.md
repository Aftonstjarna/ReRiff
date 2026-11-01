# Extending the original game

- Prefer the game's existing native solution. Inspect its original functions,
  controls and behavior before adding a replacement implementation.
- Copy, reuse or extend a suitable original game function instead of creating
  a separate system for the same job. Preserve its native input handling,
  navigation, repeat behavior, artwork and lifecycle where applicable.
- Add a new implementation only when no suitable original solution exists;
  document that gap and keep the integration consistent with the game.
