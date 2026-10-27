---
id: TASK-17
title: Keep the Chromium Authors credit in translated copyright lines
priority: medium
labels: [phase-0, branding, l10n]
depends_on: []
created: 2026-10-27
---

## Goal

Translated copyright lines credit the Chromium Authors, as the English one does (DECISION-3).
The first build (TASK-4) shows "Copyright 2026 Los creadores de Pantera" in Spanish, on the
About page and in the app's `NSHumanReadableCopyright`, while English keeps "The Chromium
Authors".

## Acceptance criteria

- [ ] `IDS_ABOUT_VERSION_COPYRIGHT` and the Info.plist copyright keep the localized "Chromium
      Authors" in every language
- [ ] Every translation ID that matched a message before the rename still matches one after
- [ ] Checked in a build: the About page in Spanish credits "Los creadores de Chromium"

## Notes

Cause: `rebrand_translations()` in `scripts/rebrand_strings.py` runs the rename regex over every
translation. The regex only protects the English " Authors", so localized forms such as "Los
creadores de Chromium" are renamed.

Likely fix: only rebrand a translation whose English message changed (its ID was rekeyed). The
copyright message has no renamed "Chromium" in English, so its translations would stay as they
are. Check that no message both renames "Chromium" and contains the authors credit.

Can be tested without a build: run the script on the original `.grd`/`.xtb` files (AGENTS.md,
Validation) or on `git -C build/src show HEAD:<path>` copies.
