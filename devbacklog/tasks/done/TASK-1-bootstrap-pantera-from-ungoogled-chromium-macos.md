---
id: TASK-1
title: Bootstrap Pantera from ungoogled-chromium-macos
priority: high
labels: [phase-0, branding]
depends_on: []
created: 2026-10-27
completed: 2026-10-27
---

## Goal

Start the repository from an unmodified copy of ungoogled-chromium-macos and brand the build as
Pantera Browser, with its own icon.

## Acceptance criteria

- [x] Upstream imported unmodified (cb57ca4), ungoogled-chromium pinned as a submodule
- [x] Branding overlay: Pantera Browser, bundle ID com.itsjavi.pantera
- [x] App icon for light and dark mode, plus Chromium's product logos
- [x] README and AGENTS.md

## Notes

Commits 8782006 to 85ff637. The icon is a black panther in profile traced from a reference photo;
`scripts/build-branding.sh` regenerates every asset.
