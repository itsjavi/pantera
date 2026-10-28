---
id: TASK-36
title: Choose HTTPS upgrades per site in the Shields panel
priority: low
labels: [phase-2, shields, privacy]
depends_on: [TASK-7]
created: 2026-10-28
---

## Goal

Let the Shields panel's "Upgrade connections to HTTPS" row (DECISION-19) turn HTTPS upgrades off
for a site that breaks with them, and back on.

## Acceptance criteria

- [ ] To be defined

## Notes

The row shows on and disabled for now: Chromium upgrades navigations to HTTPS for every site
already. Look at Chromium's HTTPS-Upgrades and HTTPS-First Mode allowlists, and at what Brave's
per-site setting adds (strict mode, which blocks a site that has no HTTPS).
