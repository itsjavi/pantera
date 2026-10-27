---
id: DECISION-3
title: Rename UI strings at build time and keep the Chromium credit
date: 2026-10-27
status: accepted
tasks: [TASK-2]
---

## Context

Chromium's branded strings mention "Chromium" about 800 times, translated into 81 languages.
Translations are keyed by a fingerprint of the English text, so a patch that changes the English
would silently drop every translation, and a patch that big would conflict on every upgrade.

## Decision

Rename with `scripts/rebrand_strings.py`, run by `build.sh` after patching. It renames the
strings and rekeys their translations using the source tree's own grit.

"Chromium" stays where it names the open source project rather than the browser: "The Chromium
Authors", the linked project name in the About page credit, and Chromium OS strings.

## Consequences

- Inflected forms in 11 languages stay as they are until TASK-11.
- New Chromium versions need no string work unless the branded catalogs move.
