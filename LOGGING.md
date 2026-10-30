# Server investigation logging

Restart `~/.local/share/ReRiff/start-server.sh` to enable tracing. Keep only one
server running on port 5144. Launch the game through Steam as usual.

Each server start creates a separate session file in
`~/.local/share/ReRiff/server/data/traces/*.jsonl`.
Records include request IDs, timestamps, routes, query parameters, JSON request
and response bodies, headers, status, duration, and exception stack frames.
Binary data is represented by size and SHA-256; uploaded files remain in the
server's normal file store. Passwords, tokens, keys, and cookies are redacted.
Player names, IDs, scores, and other gameplay data remain visible.

The wrapper replaces upstream credential-bearing request logging. Existing
`data/requests.log` files are retained and may contain credentials from earlier
sessions. Requests made directly to the upstream script bypass this wrapper.

To summarize a session:

```sh
~/.local/share/ReRiff/venv/bin/python ~/.local/share/ReRiff/trace-server.py --report /absolute/path/to/session.jsonl
```

Play one feature at a time and note its time: profile loading, song conversion,
finishing a race, leaderboard browsing, ghost selection, playlists, and restart.
Send the trace with the action sequence and visible failures for investigation.

Flags identify investigation candidates, not proof of missing functionality.
Empty queries can be normal for a fresh database. Successful HTTP responses can
still be semantically incompatible. The logger cannot detect requests that never
reach this server or reproduce historical service behavior without evidence.
Logs grow for the duration of each session; archive or remove old traces as needed.

The setup script also installs this wrapper on future runs. No upstream server
source, saved records, or game binaries are changed by enabling tracing.
