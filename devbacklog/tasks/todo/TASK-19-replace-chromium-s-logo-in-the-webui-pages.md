---
id: TASK-19
title: Replace Chromium's logo in the WebUI pages
priority: low
labels: [phase-0, branding, ui]
depends_on: []
created: 2026-10-27
---

## Goal

Settings and other WebUI pages show the panther instead of Chromium's logo. In the first build
(TASK-4), in dark mode, the Settings toolbar logo and the "About Pantera" menu icon are still
Chromium's.

## Acceptance criteria

- [ ] The Settings toolbar shows the panther in light and dark mode
- [ ] The "About Pantera" item in the Settings menu shows a Pantera icon
- [ ] Other WebUI pages that use the same icons are checked (History, Downloads, Extensions)

## Notes

Sources in Chromium 156:

- Toolbar: `ui/webui/resources/cr_elements/cr_toolbar/cr_toolbar.html.ts` uses
  `chrome://theme/current-channel-logo` (the product logo PNG, already Pantera's through
  `branding/overlay/`) in light mode, but `//resources/images/chrome_logo_dark.svg` in dark mode.
- Menu icon: `cr:chrome-product` in `ui/webui/resources/cr_elements/icons.html.ts`, a vector
  icon (two definitions, lines ~97 and ~320).

Replacing whole files fits `branding/overlay/` (an SVG); the icon set needs a small patch or a
monochrome panther path. Check the result on a light background too.
