---
id: TASK-7
title: Add a per-site privacy panel
priority: medium
labels: [phase-1, shields, ui]
depends_on: [TASK-8]
created: 2026-10-27
---

## Goal

A toolbar panel like Brave Shields: a per-site on/off switch plus scripts, third-party cookies
and HTTPS upgrades, built on Chromium's existing per-site settings.

## Acceptance criteria

- [ ] To be defined, including the panel design

## Notes

Port Brave's logic where useful, not its UI, which is tied to Brave's WebUI and design system.

Since 2026-10-28 it depends on TASK-8: the panel shows the per-tab blocked count and switches
shields and the ad-block mode (DECISION-16) for the site, as well as Chromium's own settings.
