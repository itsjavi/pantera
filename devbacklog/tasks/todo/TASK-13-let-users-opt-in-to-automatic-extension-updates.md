---
id: TASK-13
title: Let users opt in to automatic extension updates
priority: medium
labels: [phase-0, extensions, privacy, ui]
depends_on: [TASK-3]
created: 2026-10-27
---

## Goal

A setting to turn on automatic extension updates (off by default), with a clear hint while it's
on, per DECISION-5.

## Acceptance criteria

- [ ] A per-profile preference, off by default, that turns on the automatic-checks switch added
      in TASK-3 (startup and periodic checks, and the other automatic paths)
- [ ] A toggle in chrome://extensions, next to the Update button area
- [ ] While it's on, chrome://extensions shows a hint: extensions update automatically, and each
      check sends the IP address and extension list to Google
- [ ] Turning it on or off takes effect without a restart
- [ ] README updated

## Notes

The chrome://extensions page talks to the browser through the `developerPrivate` API
(`getProfileConfiguration` / `updateProfileConfiguration`), so the setting likely needs a field
there, the toolbar UI in `chrome/browser/resources/extensions/`, and new strings. Verifying needs
a build (TASK-4).
