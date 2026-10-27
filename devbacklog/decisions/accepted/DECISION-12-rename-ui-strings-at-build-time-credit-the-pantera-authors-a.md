---
id: DECISION-12
title: Rename UI strings at build time, credit the Pantera Authors and link the Chromium project
date: 2026-10-27
tasks: [TASK-2, TASK-27]
---

## Context

Chromium's branded strings mention "Chromium" about 800 times, translated into 81 languages.
Translations are keyed by a fingerprint of the English text, so a patch that changes the English
would silently drop every translation, and a patch that big would conflict on every upgrade.

DECISION-3 kept "The Chromium Authors" in the copyright line. Once Pantera chose its own license
(DECISION-11), the user asked for the About page to read like Brave's: Pantera's own authors in
the copyright line and the license under it (2026-10-27). This replaces DECISION-3.

## Decision

Rename with `scripts/rebrand_strings.py`, run by `build.sh` after patching. It renames the
strings and rekeys their translations using the source tree's own grit.

The copyright line and company name say "The Pantera Authors", in every language: "Copyright
2026 The Pantera Authors. All rights reserved." on the About page, chrome://version and the app's
Get Info copyright.

"Chromium" stays where it names the open source project rather than the browser: the linked
project name in the About page credit ("Pantera is made possible by the Chromium open source
project…"), and Chromium OS strings.

## Consequences

- Chromium's authors are credited through that linked line and the credits page, not the
  copyright line. `LICENSE` keeps both copyright lines.
- Inflected forms in 11 languages stay as they are until TASK-11.
- New Chromium versions need no string work unless the branded catalogs move.
