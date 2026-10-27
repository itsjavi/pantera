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

Blocked on disk space: 69 GB free on 2026-10-27. GNU coreutils, a listed prerequisite, isn't
installed yet.
