---
id: TASK-4
title: Build Pantera locally and check the branding
priority: high
labels: [phase-0, build]
depends_on: []
created: 2026-10-27
---

## Goal

A first ad-hoc signed build, to find build breakage early and confirm the branding works in the
running app.

## Acceptance criteria

- [ ] About 150 GB free disk and the README's prerequisites installed
- [ ] `MACOS_AD_HOC_SIGNING=1 ./build.sh` finishes with a `.dmg` in `build/`
- [ ] The app is "Pantera Browser" with the panther icon in light and dark mode
- [ ] Profile in `~/Library/Application Support/Pantera Browser`, keychain item "Pantera Safe Storage"
- [ ] The UI says Pantera (About page, menus) in English and at least one other language
- [ ] Every fix needed to get here is committed on its own

## Notes

Blocked on disk space: 63 GB free on 2026-10-27 (M1 Max, 10 cores, 32 GB RAM, Xcode 27.1 with
the macOS 27.0 SDK; use `build.sh -s` if Chromium 156 rejects that SDK).

Prerequisites on 2026-10-27: node, readline and the Metal toolchain are installed, binutils isn't
(nothing to unlink). Missing: python@3.13, ninja, coreutils, esbuild, typescript, and the PySocks
and httplib2 Python modules.

GitHub-hosted runners don't fit a first build (see TASK-12), so this one runs locally.
