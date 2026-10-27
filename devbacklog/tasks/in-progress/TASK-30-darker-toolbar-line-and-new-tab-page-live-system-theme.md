---
id: TASK-30
title: Darker toolbar line and New Tab page, live system theme
priority: medium
labels: [phase-0, branding, ui]
depends_on: []
created: 2026-10-27
---

## Goal

Dark mode stays dark everywhere in Pantera's themes, and the New Tab page follows the system
when the browser is set to Device. Asked for by the user on 2026-10-27 with a screenshot of the
toolbar.

## Acceptance criteria

- [x] The line under the toolbar is dimmer in dark mode
- [x] The New Tab page background is darker in dark mode in Pantera's themes
- [ ] An open New Tab page switches light and dark with the system, without reloading
- [x] Checked in a build

## Notes

- The line is `kColorToolbarContentAreaSeparator` = `kColorSysSurfaceVariant`, neutral-variant
  tone 30 in dark mode (`#474747`). Charcoal mode darkened only the neutral tones;
  `default-theme-palette.patch` now maps neutral-variant tones 10, 15, 20 and 30 the same way
  (tone 30 becomes about 21.7, `#363636`). The later `chromium-theme-color.patch` was
  regenerated on top so the series still applies in order.
- `patches/pantera/ntp/darker-dark-background.patch`: `kColorNewTabPageBackground` is the page
  surface (`#111111`) instead of the toolbar (`kColorSysBase`, `#212121`) in Pantera's Default
  and Grey themes in dark mode; the Chromium color and others keep the toolbar's color.
- `patches/pantera/ntp/follow-the-system-theme.patch`: `NewTabPageThirdPartyHandler`
  implemented `OnNativeThemeUpdated()` but never observed the native theme (an upstream bug), so
  a system light/dark switch reached the page only after a reload. It now observes
  `ui::NativeTheme::GetInstanceForNativeUi()`, like `NewTabPageHandler`.
- Switching macOS's appearance is a system setting, so the live switch needs the user to check;
  changing Pantera's own Mode setting goes through the ThemeService observer instead.

### Checked (2026-10-27)

In the build, dark mode: `--color-toolbar-content-area-separator` `#343434` (was `#474747`)
against the `#212121` toolbar; New Tab background `#111111` (was `#212121`); focus ring
`#C48300`. With a New Tab page open in one window, switching Pantera's Mode in another (Light,
Dark, Device) changed the page's background and gradient in place, without a reload (the
ThemeService path). Not checked: a macOS appearance switch, the native theme path this task
fixes, which needs the user to change the system setting. Leave in progress until they confirm.
