---
id: TASK-28
title: Keep Chromium's default theme as a choice and soften the amber focus ring
priority: medium
labels: [phase-0, branding, ui]
depends_on: []
created: 2026-10-27
completed: 2026-10-27
---

## Goal

Chromium's own default theme stays available after Pantera's two (Default and Grey), and the
amber focus ring in Pantera's themes doesn't glare. Asked for by the user on 2026-10-27, after
TASK-22 replaced Chromium's default palette.

## Acceptance criteria

- [x] The theme color picker shows Pantera's Default, Pantera's Grey, then "Chromium"
- [x] Choosing "Chromium" gives exactly Chromium's default theme, not a generated palette
- [x] Pantera's themes use a deeper amber focus ring in dark mode
- [x] Checked in a build

## Notes

- `patches/pantera/ui/chromium-theme-color.patch`: `ui/color/chromium_theme.h` defines the seed
  (`#0B57D0`, Chromium's primary) and `IsChromiumTheme(key)`. With that seed as the user color,
  `AddRefColorMixer()` uses Chromium's `AddBaselinePalette()` (kept in the source, unused
  since TASK-22) and `AddSysColorMixer()` applies no overrides, as for Chromium's default. The
  chip is first in `kDynamicCustomizeChromeColors` (id 15, `IDS_NTP_COLORS_CHROMIUM`, English
  only), and the picker handler shows Chromium's default swatch for it. A custom color of
  exactly `#0B57D0` also gets Chromium's default, which is fine.
- `default-theme-focus-ring.patch` now also sets the focus ring in Pantera's Default and Grey
  themes to primary tone 60 in dark mode (`#C48300`, was tone 80 `#FFB94D`) and tone 40 in
  light (`#815500`).
- Verified on 2026-10-27 in the build (dark mode): picker labels "Default color", "Grey default
  color", "Chromium", "Blue"...; choosing Chromium gave primary `#A8C7FA`, tab strip
  `#1F2020`, toolbar `#3C3C3C`, pages `#1F1F1F` (Chromium's dark default) with the chip
  selected; Default brought amber back; focus ring `#C48300`.
- Still blue: the avatar picker's selected check mark (TASK-25).
