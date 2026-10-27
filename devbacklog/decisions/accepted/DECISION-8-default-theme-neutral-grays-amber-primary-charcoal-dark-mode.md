---
id: DECISION-8
title: Default theme: neutral grays, amber primary, charcoal dark mode
date: 2026-10-27
tasks: [TASK-22]
---

## Context

Chromium's default theme is Google's: a blue primary (`#0B57D0`), grays with a slight tint, and
a light-mode tab strip tinted with the primary color. The user wants Pantera to look like its own
browser: no tinted grays, a primary color that isn't blue or indigo, and a darker dark mode
(2026-10-27). They chose from previews computed with Chromium's own color code: amber, jade or
fuchsia as the primary, and Chromium's dark mode, charcoal or true black for dark mode.

## Decision

- Pantera's default theme replaces Google's default palette, so it's the "Default" entry, the
  first in the theme color picker, and what new profiles get.
- Surfaces use zero-chroma grays in light and dark mode, including the light-mode tab strip.
- The primary color is amber, from the panther's eye in the first app icon: seed `#F2A516` (HCT hue
  75). It renders as `#815500` in light mode and `#FFB94D` in dark mode.
- Dark mode is charcoal: near black with visible steps between the tab strip, toolbar and pages
  (roughly `#0E0E0E`, `#1F1F1F` and `#131313`), not true black.
- Other picker colors, Grey and custom colors keep Chromium's behavior.

## Consequences

- Patches to Chromium's color mixers (`ui/color/`), which may need refreshing on upstream syncs.
- Amber is the accent for buttons, links, toggles and focus rings, so it shouldn't double as a
  warning color in Pantera's own UI.
- Branding elsewhere (icons, WebUI logos, the website if there is one) should use the same amber.
