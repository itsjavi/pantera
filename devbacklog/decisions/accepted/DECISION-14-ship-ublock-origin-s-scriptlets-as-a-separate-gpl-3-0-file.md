---
id: DECISION-14
title: Ship uBlock Origin's scriptlets as a separate GPL-3.0 file
date: 2026-10-28
tasks: [TASK-32, TASK-33]
---

## Context

`+js()` scriptlets and `$redirect` resources come from uBlock Origin, which is GPL-3.0. Brave
builds them into a `resources.json` offline and ships it through its component updater, which
Pantera can't use (DECISION-6). Extracting the scriptlets from uBlock Origin's sources needs a
JavaScript runtime. Without them, `+js()` rules (YouTube and Twitch ads, anti-adblock walls) do
nothing and `$redirect` rules become plain blocks. The user chose to bundle them on 2026-10-28.

## Decision

- At build time, Pantera generates `resources.json` from a pinned uBlock Origin release (its
  scriptlets and redirect resources) and Brave's MPL-2.0 `adblock-resources`.
- It ships as its own file in the app bundle, next to its GPL-3.0 license, and is never compiled
  into the binary.
- The pins are raised with Pantera releases.

## Consequences

- The GPL-3.0 applies to that file only. Its license ships beside it and its source is the pinned
  upstream release; the rest of Pantera stays BSD-3-Clause.
- A list that needs a scriptlet newer than the pinned release doesn't get it until the next
  Pantera release.
- The build needs Node, which it already uses for Chromium's WebUI.
