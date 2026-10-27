---
id: TASK-22
title: Give Pantera its own default theme
priority: medium
labels: [phase-0, branding, ui]
depends_on: []
created: 2026-10-27
---

## Goal

Pantera's default look is its own: pure neutral grays (no tint) for every surface, one
distinctive primary color instead of Chromium's blue, and a darker dark mode. It's the first
choice in the theme color picker and the default for new profiles. Asked for by the user on
2026-10-27.

## Acceptance criteria

- [ ] Primary color and dark-mode depth chosen by the user (open decisions below)
- [ ] With no theme chosen, light and dark mode use zero-chroma grays and Pantera's primary
- [ ] Dark mode surfaces (tab strip, toolbar, address bar, WebUI pages) are darker than Chromium's
- [ ] The Pantera theme is the first entry in Customize Chrome's color picker
- [ ] Other picker colors, Grey and custom colors still work
- [ ] Checked in a build in light and dark mode: browser frame, Settings, New Tab, menus

## Notes

How Chromium 156 colors its UI (`build/src`, 2026-10-27):

- `ui/color/ref_color_mixer.cc`: with no user color (and for Grey), `AddBaselinePalette()`
  hard-codes Google's palette: primary tone 40 `#0B57D0`, slightly tinted neutrals and
  neutral-variants. With a user color, `AddGeneratedPalette()` builds the ref tones from
  `ui/color/dynamic_color/palette_factory.cc`'s `GeneratePalette(seed, variant)`.
- Variants (`palette_factory.cc`): TonalSpot (default; primary chroma 40, neutral 6, neutral
  variant 8), Vibrant, Neutral (primary chroma 12-20, neutral 2), Expressive. None has
  zero-chroma neutrals with a saturated primary, so Pantera needs its own palette.
- `ui/color/sys_color_mixer.cc` maps ref tones to surfaces. Dark: tab strip (`kColorSysHeader`)
  N12, toolbar and active tab (`kColorSysBase`) N25, address bar N15, page surface N10, primary
  P80 on P20. Light: tab strip P90 and header container P95, which tint the frame with the
  primary color, so neutral light surfaces need a mixer change as well as a palette.
- Picker: `ui/webui/resources/cr_components/theme_color_picker/theme_color_picker.html.ts` shows
  Default (baseline), Grey, the 13 colors in
  `chrome/browser/ui/webui/cr_components/theme_color_picker/customize_chrome_colors.h`
  (`kDynamicCustomizeChromeColors`), then a custom color. Replacing the baseline makes Pantera's
  theme the first chip and the default without adding an entry.

Exact tones can be computed with Chromium's own Material color code: a small C++ program
against `third_party/material_color_utilities/src/cpp` (stub `absl::StrCat`/`absl::Hex`)
prints `TonalPalette(hue, chroma).get(tone)`. Candidates computed on 2026-10-27:

| Seed | Hue | P40 (light) | P80 (dark) | P90 |
|---|---|---|---|---|
| Amber `#F2A516` (the icon's eye) | 75 | `#815500` | `#FFB94D` | `#FFDDB2` |
| Jade `#1E9E6A` | 162 | `#006C46` | `#73DAA5` | `#8FF7BF` |
| Fuchsia `#C026D3` | 329 | `#8D3E94` | `#FDA9FF` | `#FFD6FB` |

Zero-chroma grays: N4 `#0E0E0E`, N6 `#131313`, N10 `#1B1B1B`, N12 `#1F1F1F`, N15 `#262626`,
N25 `#3B3B3B`, N90 `#E2E2E2`, N95 `#F1F1F1`.

Open decisions (ask the user): the primary color, and how dark dark mode gets (near-black
surfaces, or true black for the frame and pages).

Iterating needs builds; `build.sh` deletes `out/` and rebuilds everything, so for this work run
`ninja -C out/Default chrome` in `build/src` (as `build.sh` does) after applying the patch by hand.
