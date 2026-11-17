# Security alert triage — 2026-11-16

Read-only review of Aftonstjarna/ReRiff GitHub security alerts: 63 open
Dependabot alerts (5 critical, 37 high, 21 medium), all in build dependencies.
No alerts were dismissed or marked fixed by this review.

| Priority | Findings | Exposure and action |
| --- | --- | --- |
| First | Pillow: 62 alerts, including all 5 critical findings | Image/font build tooling. Upgrade 12.0.0 to 13.0.0 in the requirements file and experimental workflows. |
| Next | fontTools: 1 medium alert, GHSA-768j-98cg-p3fv | Font build tooling. Upgrade 4.60.1 to the security backport 4.60.2. |
| Coverage gap | Code scanning returned HTTP 404, “no analysis found” | No clean code-scan result can be claimed. The CLI also printed a token-scope hint; a separate code-quality endpoint returned 404. Confirm analysis availability/settings before treating this as a complete code audit. |
| Checked | Secret scanning returned zero open alerts | This is the returned alert count, not proof that all files/history are secret-free. |

## Exposure

These requirements are used to generate artwork and font packs, not shipped as
Python dependencies in the installed client. Most image-format findings involve
formats outside the normal PNG/font build path. That reduces routine exposure,
but does not justify retaining vulnerable native parsers in build environments.

The fontTools advisory affects its varLib command-line designspace processing;
the inspected font builder uses TTFont for a checksum-pinned font's character
map instead. Upgrade anyway rather than dismissing the alert as unreachable.
No runtime/server dependency alert was returned in this snapshot. This is not a
full audit of application code or third-party native binaries.

## Patch and validation

- Pin Pillow 13.0.0 and fontTools 4.60.2, covering the vulnerable ranges in this
  snapshot. Also replace the three direct Pillow pins in experimental workflows.
- In an isolated environment with the patched packages, pass CJK regional atlas,
  high-resolution atlas and Latin glyph/metrics tests on main and experimental
  sources. Experimental UI asset and prestige-artwork tests also pass.
- Default-branch alerts remain open until the main patch is merged and GitHub
  refreshes dependency analysis. A branch patch or draft PR is not remediation
  of the default branch. No new application release is produced by this patch.

Sources: [repository alerts](https://github.com/Aftonstjarna/ReRiff/security/dependabot),
[Pillow 13.0.0 release notes](https://pillow.readthedocs.io/en/stable/releasenotes/13.0.0.html),
[fontTools advisory](https://github.com/fonttools/fonttools/security/advisories/GHSA-768j-98cg-p3fv).
