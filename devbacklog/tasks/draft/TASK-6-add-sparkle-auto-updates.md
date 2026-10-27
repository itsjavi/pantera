---
id: TASK-6
title: Add Sparkle auto-updates
priority: medium
labels: [phase-1, updates]
depends_on: [TASK-5]
created: 2026-10-27
---

## Goal

Pantera updates itself promptly after each Chromium security release.

## Acceptance criteria

- [ ] To be defined: update feed hosting, signing keys, how often it checks, and what the check
      sends

## Notes

ungoogled-chromium builds with `enable_updater=false`. Brave uses Sparkle on macOS.
