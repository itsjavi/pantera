---
id: TASK-34
title: Add a Shields settings page for filter lists and custom filters
priority: medium
labels: [phase-2, shields, ui]
depends_on: [TASK-32]
created: 2026-10-28
---

## Goal

A settings page where users choose the default blocking mode, turn filter lists on and off, add
their own filters and subscribe to lists by URL.

## Acceptance criteria

- [ ] The page sets the default mode (standard or aggressive, DECISION-16)
- [ ] It lists the catalog's filter lists with on/off switches and when each was last updated,
      plus "Update now" and the setting that turns updates off
- [ ] Users can add their own filters and subscribe to filter lists by URL
- [ ] Its strings are translatable resources
- [ ] Checked in the build

## Notes

- Brave's page is built on its own WebUI; Pantera's lives in Chromium's settings WebUI.
