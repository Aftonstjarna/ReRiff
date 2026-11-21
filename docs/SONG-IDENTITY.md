# Song identity and catalog matching

Song identity is **artist/title metadata**, not file bytes. Content hashes are
recorded as evidence and used only where metadata cannot decide.

## Registration

The launcher posts each cached song as `{song_id, sha256, title, artist}` to
`POST /reriff/songs/register`. The catalog stores:

```json
{
  "records": {
    "cached-song-id": {
      "title": "Hurt",
      "artist": "Nine Inch Nails",
      "hashes": ["<sha256 of this copy>", "<sha256 of another copy>"]
    }
  },
  "aliases": {"cached-song-id": "canonical-song-id"}
}
```

- Artist and title are trimmed, ASCII case-folded and internal whitespace is
  collapsed before grouping.
- Every `song_id` with the same normalized artist/title maps to one canonical
  ID (the lexicographically smallest registered ID). Scores and ghosts are
  resolved through that alias, so a FLAC original and an MP3 transcode share a
  leaderboard.
- A `song_id` may be re-registered with a new hash (re-encode) and keeps its
  metadata; the hash is appended to `hashes`.
- A `song_id` may **not** change artist/title. That would silently move an
  existing cache ID to another leaderboard and is refused.
- If the title or artist is empty after normalization, identity falls back to
  exact-source matching by hash. Unlabelled audio is never merged by a generic
  or missing title.
- Legacy records that store a single `sha256` are migrated to `hashes` on the
  next registration; the alias output is unchanged.

## Consequences

- Transcoded or differently mastered files with identical artist/title merge.
  This is intentional and is what makes a rolling Navidrome cache safe to use
  with shared leaderboards.
- Two genuinely different recordings that share exactly the same artist and
  title will also merge. Including album or duration in registration would
  reduce that risk; neither is sent today.
- The client still hashes files for its own cache performance, but a changed
  hash no longer creates a separate song identity.
