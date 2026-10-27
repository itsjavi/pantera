---
id: TASK-2
title: Rename Chromium to Pantera in the profile, keychain and UI strings
priority: high
labels: [phase-0, branding, l10n]
depends_on: [TASK-1]
created: 2026-10-27
completed: 2026-10-27
---

## Goal

Pantera shouldn't share Chromium's profile or keychain item, and its UI should say Pantera.

## Acceptance criteria

- [x] Profile in `~/Library/Application Support/Pantera Browser` (`set-profile-directory.patch`)
- [x] Keychain item "Pantera Safe Storage" (`set-keychain-item-name.patch`)
- [x] UI strings and their translations renamed (`scripts/rebrand_strings.py`)

## Notes

Checked against Chromium 156 sources only: both patches dry-run cleanly, and the string rename
updated 765 English mentions and 46,251 translations with none losing their message. The
running app is checked in TASK-4. Inflected forms in 11 languages are TASK-11.
