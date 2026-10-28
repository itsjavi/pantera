---
id: DECISION-18
title: Shields panel: toolbar button with a count, Brave-style rows, reload on change
date: 2026-10-28
tasks: [TASK-7]
replaced_by: DECISION-19
---

## Context

With request blocking in place (TASK-8), users need a way to turn shields off on a site that
breaks, or to block harder on one that doesn't. Brave puts this in a panel behind a toolbar
button, with advanced options below the main switch. Pantera can't reuse Brave's panel, which is
built on Brave's WebUI. The user chose the layout on 2026-10-28.

## Decision

- A shield button right of the address bar. Its badge shows the number of requests blocked on
  the page.
- The panel shows:
  - the site, the shields switch for it, and the blocked count;
  - advanced controls, like Brave's: trackers and ads blocking (standard or aggressive, as in
    DECISION-17), blocking scripts and blocking third-party cookies on the site;
  - a list of the requests blocked on the page.
- Scripts and third-party cookies use Chromium's own per-site settings.
- Rows only appear for protections Pantera has. Fingerprinting (TASK-10) and "forget me"
  (TASK-9) join the panel when they exist.
- Changing a setting reloads the page right away, as in Brave.
- The panel is native Views code, and its strings are translatable resources.

## Consequences

- Turning shields off is the panel's way to allow all ads on a site, since ad blocking is the only
  shield for now. Brave's separate "allow all" choice can come back when there are other shields
  to keep on.
- The scripts and cookies rows act whatever the shields switch says, because they are Chromium's
  settings, not Pantera's.
