---
id: TASK-29
title: Point the About page's Chromium link at chromium.org
priority: low
labels: [phase-0, branding]
depends_on: []
created: 2026-10-27
---

## Goal

The "Chromium" link in "Pantera is made possible by the Chromium open source project" opens the
Chromium project. Today it can't: domain substitution turns `kChromiumProjectURL` in
`chrome/common/url_constants.h` into `https://www.ch40m1um.qjz9zk/`, which doesn't resolve.
Found while doing TASK-27 (2026-10-27).

## Acceptance criteria

- [ ] The About page's "Chromium" link opens `https://www.chromium.org/`
- [ ] chrome://version's link, which uses the same constant, does too
- [ ] Checked in a build

## Notes

Options: a patch passing a Pantera-owned constant to `IDS_VERSION_UI_LICENSE` in the two
callers, or excluding `url_constants.h` from domain substitution. The second would need checking
every other Google URL in that file (AGENTS.md), so the patch is likely smaller.
