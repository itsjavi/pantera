---
id: TASK-5
title: Ship a signed and notarized release
priority: medium
labels: [phase-0, release]
depends_on: [TASK-4]
created: 2026-10-27
---

## Goal

A Developer ID signed and notarized `.dmg` that opens on any Mac without Gatekeeper warnings.

## Acceptance criteria

- [ ] Signing and notarization variables set up (see the README's Building section)
- [ ] `spctl -a -vvv -t install "Pantera Browser.app"` reports "Notarized Developer ID"
- [ ] The release process is written down

## Notes

The user has an Apple Developer ID.
