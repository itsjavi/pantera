---
id: DECISION-19
title: Shields panel: Brave's rows with unbuilt ones disabled, a corner count badge, no cookie eye
date: 2026-10-28
tasks: [TASK-7]
---

## Context

DECISION-18 showed only the rows for protections Pantera has. Looking at Brave's panel, the user
chose on 2026-10-28 to build the essentials now and show the rest disabled, each planned in its
own task. Chromium's cookie controls icon in the address bar (the eye) does the same job as the
panel's cookies row, and the user chose how the toolbar button looks.

## Decision

- A shield button right of the address bar, in the toolbar's own icon colors like the buttons
  next to it, and in its disabled color when shields are down. A badge in Pantera's accent color
  sits on its bottom-right corner with the number of requests blocked on the page, without
  shrinking or moving the shield. No badge when nothing was blocked or shields are down.
- The panel shows:
  - the site, the shields switch for it, and the blocked count, which opens the list of requests
    blocked on the page;
  - advanced options in Brave's order: trackers and ads (aggressive or standard, as in
    DECISION-17), HTTPS upgrades, block scripts, fingerprinting, cookies (block third-party,
    block all or allow all), forget me;
  - Brave's footer, with filter lists and global settings.
- Rows and choices Pantera doesn't have yet are shown disabled, with "Coming soon":
  "allow all trackers & ads" (TASK-37), HTTPS upgrades (TASK-36, on for every site meanwhile),
  fingerprinting (TASK-10), forget me (TASK-9), and the footer's buttons (TASK-34).
- Scripts and cookies use Chromium's own per-site settings.
- Changing a setting reloads the page right away, as in Brave.
- The panel is native Views code, its rows laid out like page info's, and its strings are
  translatable resources.
- Chromium's cookie controls icon is hidden from the address bar; the panel's cookies row
  replaces it.

## Consequences

- Turning shields off is the way to allow all ads on a site until another shield exists.
- The scripts and cookies rows act whatever the shields switch says, because they are Chromium's
  settings, not Pantera's.
- Each disabled row is a promise: its task turns it on, or removes it if the feature is dropped.
- Cookie exceptions made from Chromium's eye icon before still apply; the cookies row and
  Chromium's settings page show and change them.
