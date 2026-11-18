# Launcher startup and update performance (development)

The launcher retains the pre-launch song identity handshake, but caches audio
SHA-256 values in `%APPDATA%/Octane/DAT/ReRiff-song-hashes.bin`. Each use opens
the file and compares its identity, size, creation time and last-write time.
Changed, replaced or missing files do not reuse the entry. Entries expire after
24 hours, so edits that preserve all checked metadata are eventually rehashed.
This is a performance cache, not tamper-proof content verification. The first
launch and expired entries still read the complete audio file.

The bounded, versioned sidecar holds at most 10,000 entries. It stores path
hashes, not filenames. Malformed/truncated caches are ignored; failed writes
leave normal hashing available. Writes use a temporary file and atomic replace.
Unchanged caches are not rewritten. The original game cache and saves are not
modified by the launcher cache.

Successful checks finding no update are reused for **15 minutes** on automatic
startup checks. The cache identity includes installed/requested channels,
version and launcher executable size/time. Manual Update from GitHub and forced
reinstall always bypass it. Failed checks and pending updates never create a
successful-check entry. Future timestamps and expired/corrupt entries are ignored.
Windows private Python setup is deferred until an update actually needs install.

Diagnostic launcher logs now show elapsed milliseconds for song hashing (with
reuse/read counts), complete sync, local-server readiness, Game.exe spawn,
update requests, checksum verification and installer preflight. The updater log
adds verification, extraction/preparation and install totals. Existing logging
switches still apply; these messages do not expose keys or local music paths.

Package verification streams fixed-size chunks instead of loading the complete
archive into memory. Checksum verification, transactional installation and
rollback are retained. No download-speed gain or measured real-game startup
speedup is claimed yet; the stage timings support the next live comparison.

Validation: native performance-cache regressions, cold/warm HTTP registration,
15 updater tests, and a development launcher build. Changes are not released.

## Diagnostic timestamps

New human-readable diagnostic lines use UTC with millisecond precision:
`[2026-11-17T14:30:05.123Z] [launcher] message`. Component tags remain where
available. Multiline messages, exceptions and captured setup/update output get
one timestamp per line. Existing logs are retained unchanged. JSONL traces keep
their existing structured `time` field, and machine-readable result/configuration
files are not prefixed. Logging preferences and grouped file names are retained.

## Search work and HTTP connection reuse

Partial library publications reuse normalized literal/Kana text from the prior
snapshot when the full search text is identical. Each snapshot retains its own
source indices, so reordering, duplicate names and changed metadata preserve
selection identity. Reuse is confined to one open picker session; filesystem
rescans still detect additions/removals when reopening.

One search worker serves each session. New query/index revisions supersede older
work, which checks cancellation every 256 entries. Clearing or closing search
stops pending scans; native picker ownership, input and restoration are unchanged.

The song identity probe and registration share one WinINet session and request
persistent connections, allowing HTTP/TLS connection reuse when supported by the
server. No server-health cache or registration skip was introduced: primary and
fallback selection, timeouts and registration-before-game ordering are retained.

Offline comparison on the same machine (100,000 synthetic entries): four partial
index builds fell from 1,217 ms to 554 ms. At 10,000 entries they fell from 117 ms
to 54 ms. Individual query time was essentially unchanged. These are index-work
measurements, not end-to-end startup or frame-rate claims. Network connection
reuse has passed registration regression testing but has no measured live startup
gain yet.

Validation: 224 picker checks, 24 bridge/lifecycle checks (native UI mocked),
Wine HTTP registration against an isolated server, and development launcher and
client builds. Tests include changed/reordered/duplicate labels, invalid UTF-16,
cancellation and 100 rapid query revisions. Live game validation remains last.

## Bounded extraction and single-pass traversal

Update preparation now finds the Linux payload marker using 64 KiB chunks and
opens the gzip tar payload as a stream. ZIP and tar members are copied in at most
1 MiB chunks, with declared-length checks. Package checksums, path/link rejection,
512 MiB expanded-size limit, metadata binding, installer preflight and rollback
remain in place. This bounds extraction buffers; it does not claim all installer
validation or runtime-copy allocations are bounded to that size.

Library scanning now uses one `DirectoryInfo.EnumerateFileSystemInfos` pass per
folder and the attributes supplied by enumeration. Explicit linked roots remain
supported; nested directory links/junctions remain excluded. Cancellation, limits,
metadata identity, overlapping-root deduplication and final sorting are retained.
Filesystem contents may still change during any scan; failures are reported as
unreadable entries/folders rather than aborting the whole library.

Validation: 19 updater regressions, 228 picker checks (including an external
linked-folder fixture under Wine), and 24 mocked-native bridge checks passed.
The managed development build passed. Production preparation with a 32 MiB
fixture used peak Python allocations of 2.4 MiB (stored ZIP) and 4.5 MiB (Linux),
with installer validation mocked only for that allocation measurement. Separate
regressions exercise real preflight, apply and rollback. A read-only music scan
found 6,620 tracks in 448 ms without unreadable folders; library changes prevent
an exact comparison against earlier runs. No live game check was run.

## Diagnostic identity headers

New diagnostic logs start with a timestamped identity header. Existing logs retain
their history: each new writer session appends its header, with another header
when the native/managed writer notices an identity change. The header includes
player display name when available, ReRiff account UUID and server UUID, client
installation UUID, installed client version, platform and logging process. Native
writers also identify their compiled release/development build. Unknown fields
are explicitly `unavailable`; launchers/helpers label cached accounts `last-known`.

After a successful ReRiff account connection, the game writes only the public
account/server identifiers and its process identity to
`localserver/diagnostic-context.ini`. Process creation time prevents a recycled
PID from making an old snapshot look current. The already initialized Steam
client can supply the display name; it is only a readable label, not an account
key or proof of ownership. Clients without that display-name source still log
ReRiff UUIDs. No account credential, password, API key or session ticket is copied.

`localserver/diagnostic-client-id.txt` holds a random per-installation UUID shared
by native, managed and Python writers. It is diagnostic only, not an authentication
credential. Copying a whole installation also copies this ID. Normal updates
retain it. Read-only/preflight header inspection does not create it; actual log
writers may create it. Logging-off behavior and machine-readable output remain
unchanged. Metadata lookups in native/managed hot paths are cached for five seconds.

Steam account reuse across installations is a separate authentication feature.
It is not enabled by these headers. The requested automatic flow needs server-side
verification of a Steam authentication ticket before associating a Steam identity
with a ReRiff UUID. A client-supplied SteamID or this diagnostic snapshot must
never authorize recovery or account linking. No verifier is currently configured;
account ownership and saved-game selection remain unchanged. Steam's reference:
https://partner.steamgames.com/doc/webapi/ISteamUserAuth#AuthenticateUserTicket
