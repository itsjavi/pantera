---
id: TASK-25
title: Replace the remaining hard-coded Google blues
priority: low
labels: [phase-0, branding, ui]
depends_on: []
created: 2026-10-27
---

## Goal

No Google blue is left in Pantera's UI where it stands for "the accent color", now that the
default theme is amber (DECISION-8).

## Acceptance criteria

- [ ] Decide which blues are accents and which are semantic (the "Blue" tab group, Reading
      mode's blue theme) and stay
- [ ] Accent blues use the theme's primary or a neutral

## Notes

Found 2026-10-27 in `build/src` (`grep -rn kGoogleBlue` over the color mixers):

- `chrome/browser/ui/color/chrome_color_mixer.cc`: feature promo (help) bubbles
  (`kColorFeaturePromoBubbleBackground` = Google blue 700), Reading mode's blue theme, tab group
  blue.
- `chrome/browser/ui/color/omnibox_color_mixer.cc`: starter-pack result icons
  (`@bookmarks`, `@history`), a selected-keyword color, Google's compose box.
- `chrome/browser/ui/color/new_tab_page_color_mixer.cc`: the Google New Tab page's colors (not
  shown without Google search) and `kColorNewTabPageText` (`#E8EAED` in dark mode, slightly
  blue).
- WebUI search fields kept a blue focus ring in a first check; recheck after
  `default-theme-webui-colors.patch`.
