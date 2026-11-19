# Libre.fm scrobbling (1.4.1-beta.1)

Open **Scrobbling → Libre.fm** in the launcher. If Libre.fm issued you an API key,
paste it into the masked **API key** field and **Save settings** first. Choose **Link account**, and approve
ReRiff in your browser. Keep this subtab open; it checks for approval automatically.
Enable scrobbling, choose the listening percentage, and select **Save settings**.
Restart the game after changing settings. The API key is encrypted when saved. Changing it clears the old account link;
link again before enabling scrobbling. No password is entered in the launcher.

Libre.fm, Last.fm and ListenBrainz can run independently or together. The existing
race playback observer supplies tagged artist/title metadata and counts actual
listening, excluding pauses and seeks. A completed race or the chosen percentage
(default 80%) qualifies only after the minimum listening time: half the song or
four minutes, whichever is shorter. Songs must be longer than 30 seconds.

The `[librefm]` section of `client.ini` stores `enabled`, `scrobble_percent`,
`username`, an optional `api_key`, and a user-scoped DPAPI-protected `session_key`. The fixed client
identifier is not a private application credential. Keep your Windows profile or
Proton prefix private. Disconnect clears the local session and disables this
provider without affecting the others.

`librefm-queue.json` persists pending listens with their original playback time.
Temporary failures back off and retry. Invalid sessions stop submission without
removing queued listens. Queues are scoped to the linked session; a newly linked
session does not submit the previous session's pending history. Now Playing is
transient and does not replace a completed scrobble.

Uses the HTTPS Last.fm-compatible API at `https://libre.fm/2.0/`, with
`auth.getToken` / `auth.getSession` browser authorization, `track.updateNowPlaying`
and `track.scrobble`. The implementation follows [GNU FM's API source](https://github.com/libre-fm/gnu-fm/blob/master/nixtape/2.0/index.php)
and shares ReRiff's existing HTTPS transport, encrypted credential store, playback
policy and durable queue.

## Validation and current limitation

Automated tests cover independent provider configuration, encrypted native session
storage, queue routing, original listen timestamps, linking polling, settings
preservation and subtab visibility/layout. Live account linking and submission
still need testing: the public authorization endpoint returned HTTP 500 during
verification on November 18, 2026. No account listens were submitted by these tests.
