# Experimental server image

The dedicated `codex/server-experimental` branch publishes the tested server
image as `ghcr.io/aftonstjarna/reriff:experimental` and a commit-specific SHA tag.
It does not move `:latest`; the default branch retains that publishing policy.
Both publication paths run the real image smoke test and inherited API suites.

For a trial, change the existing deployment's image tag from `:latest` to
`:experimental`, pull it, and recreate the server while retaining its existing
data volume. The running server does not change merely because an image tag
is published. Return to `:latest` and recreate it to roll back the image.

This image replaces the fixed RiffApiLocal News placeholder with a pinned
ReRiff status entry while preserving normal stored announcements. A client
reporting an older ReRiff release receives an exit-and-update notice. Equal or
newer clients get neutral status; absent/invalid client versions get an explicit
unknown status. Version reporting uses `X-ReRiff-Client-Version`; the original
game protocol's `Ver:100` remains unchanged.

The server release comes from the image's VERSION file, or an explicit
`RERIFF_SERVER_VERSION` override. The version policy is a separate native C++
library, preserving the existing song identity engine and player data.
