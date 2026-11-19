# ListenBrainz scrobbling (1.4.1-beta.1)

ReRiff 1.4.1-beta.1 can submit Now Playing and completed listens to
ListenBrainz independently of Last.fm. Either provider or both can be enabled.
This feature is not in stable 1.4.0.

## Setup

1. Open the launcher's **Scrobbling → ListenBrainz** subtab and choose **Get user token**.
2. Sign in to ListenBrainz and copy your user token from its settings page.
3. Paste it into the masked **User token** field, check **Enable ListenBrainz
   scrobbling**, choose the listening percentage, and click **Save settings**.
4. Restart Riff Racer. No Last.fm application key or shared secret is required.

Save checks the token's format, not its server-side validity. Connection or
rejected-token messages appear under the game's diagnostic logging. Disable the
checkbox to stop submissions; disable it and clear the token to remove the
saved credential. Restart after changing the token or listening percentage.

## Listening policy

The native race playback observer is shared with Last.fm. Each provider has its
own percentage, defaulting to 80%. A completed race can qualify earlier, but
actual listening must reach half the track or four minutes, whichever is less.
Pauses, seeks and skipped time do not count. Each play qualifies at most once.
The shared ReRiff policy excludes tracks of 30 seconds or less and requires
usable artist/title metadata; it does not invent tags from filenames.

Now Playing has no listen timestamp. Completed listens retain the original
playback-start timestamp and include artist, title and duration. Album is sent
only when available. The native observer currently supplies artist and title,
not album or MusicBrainz IDs. Audio files and local paths are never uploaded.

## Storage and retries

Settings live in `[listenbrainz]` in `client.ini`: `enabled`, `user_token` and
`scrobble_percent`. The launcher encrypts the token with user-scoped Windows
DPAPI and refuses to overwrite unreadable protected credentials. This is tied
to the Windows profile or Proton prefix, not protection from programs running
as the same user. Keep credentials and the profile/prefix private.

`listenbrainz-queue.json` retains up to 500 pending listens, bound to a hash of
the token. It contains listening history but no token. Another token cannot
replay that queue. Last.fm has its own queue. The oldest pending entry is dropped
if the queue is full. Abrupt termination before the worker persists a new listen
can lose it; loss of a server acknowledgement can cause a retry of the same listen.

Requests use HTTPS, no redirects, a contact User-Agent, at most one request per
second, rate-limit headers and exponential retry backoff. Invalid credentials
stop submissions until corrected and the game restarted; malformed listens are
removed so later listens can proceed. Network work never runs on the game thread.

## Validation

Automated checks cover payloads, timestamps, independent configuration, playback
thresholds, durable queues, account separation, rate limits, transient/permanent
failures, launcher page visibility and encrypted token handoff to the client.
Live account submission and in-game testing still need verification. Tests use
synthetic tracks and mocked submissions rather than writing to a real account.

API references: [submission endpoint](https://listenbrainz.readthedocs.io/en/latest/users/api/core.html#post--1-submit-listens),
[JSON format](https://listenbrainz.readthedocs.io/en/latest/users/json.html), and
[authentication and rate limits](https://listenbrainz.readthedocs.io/en/latest/users/api/).
