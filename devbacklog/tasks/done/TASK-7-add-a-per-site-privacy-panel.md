---
id: TASK-7
title: Add a per-site privacy panel
priority: high
labels: [phase-1, shields, ui]
depends_on: [TASK-8]
created: 2026-10-27
completed: 2026-10-28
---

## Goal

A toolbar panel like Brave Shields (DECISION-19): turn shields off on a site that breaks, block
harder on one that doesn't, and see what was blocked.

## Acceptance criteria

- [x] A shield button right of the address bar shows the number of requests blocked on the page,
      and looks off when shields are down
- [x] Its panel shows the site, a shields switch for it and the blocked count
- [x] Advanced controls: trackers and ads blocking (standard or aggressive), block scripts and
      block third-party cookies, for the site
- [x] Brave's other rows are shown disabled until their tasks, and Chromium's cookie controls
      icon is hidden
- [x] An expandable list shows the requests blocked on the page
- [x] Changing a setting reloads the page
- [x] The strings are translatable resources
- [x] Checked in the build, with screenshots in light and dark mode

## Notes

Port Brave's logic where useful, not its UI, which is tied to Brave's WebUI and design system.

Since 2026-10-28 it depends on TASK-8: the panel shows the per-tab blocked count and switches
shields and the ad-block mode (DECISION-17) for the site, as well as Chromium's own settings.

Fingerprinting (TASK-10), "forget me" (TASK-9), per-site HTTPS upgrades (TASK-36), "allow all
trackers & ads" (TASK-37) and the footer's buttons (TASK-34) are disabled rows until then.

2026-10-28, built and checked on edition.cnn.com with screenshots in light and dark mode
(`--pantera-show-shields-panel`, `scripts/capture-app-windows.swift`): the button's count and
shields-down look, the panel's site, switch and count, the advanced rows with their current
settings, the blocked list, and no cookie controls icon in the address bar. The shield uses the
toolbar's own icon colors and the count badge Pantera's accent; the rows' icons line up with page
info's metrics. The site settings behind the controls are covered by `pantera_unittests`.

2026-10-28, the user clicked through the panel's controls in the build: the page reloads with
the new setting.
