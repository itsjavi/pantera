---
id: TASK-22
title: Give Pantera its own default theme
priority: medium
labels: [phase-0, branding, ui]
depends_on: []
created: 2026-10-27
completed: 2026-10-27
---

## Goal

Pantera's default look is its own: pure neutral grays (no tint) for every surface, one
distinctive primary color instead of Chromium's blue, and a darker dark mode. It's the first
choice in the theme color picker and the default for new profiles. Asked for by the user on
2026-10-27.

## Acceptance criteria

- [x] Primary color and dark-mode depth chosen by the user (open decisions below)
- [x] With no theme chosen, light and dark mode use zero-chroma grays and Pantera's primary
- [x] Dark mode surfaces (tab strip, toolbar, address bar, WebUI pages) are darker than Chromium's
- [x] The Pantera theme is the first entry in Customize Chrome's color picker
- [x] Other picker colors, Grey and custom colors still work
- [x] Checked in a build in light and dark mode: browser frame, Settings, New Tab, menus

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

### Implementation (2026-10-27)

The user chose amber and charcoal (DECISION-8). Patches in `patches/pantera/ui/`:

- `default-theme-palette.patch` (`ref_color_mixer.cc`): `AddPanteraPalette()` replaces the
  baseline palette. A `PanteraPalette` uses the seed `#F2A516` at its own chroma for the
  primary, zero-chroma neutrals and neutral variants, and Tonal Spot's secondary, tertiary and
  error. In dark mode, neutral tones up to 25 are halved and ease back to their own value at
  40. `AddGeneratedPalette()` now goes through a shared `AddPalette()`.
- `default-theme-neutral-surfaces.patch` (`sys_color_mixer.cc`): the default theme applies the
  Grey theme's neutral surface overrides, keeps `kColorSysOnSurfacePrimary` in the primary, and
  makes `kColorSysSurfaceNumberedForeground` (a hard-coded blue that tinted menus, dialogs and
  cards) neutral.
- `default-theme-picker-swatch.patch` (`color_utils.ts`): the Default and Grey swatches show
  amber instead of blue.

Both C++ files compiled. The user asked for the New Tab page (TASK-23) to ship in the same build,
so the in-app check waits for that combined incremental build (`ninja -C out/Default chrome`).

### First build check and fixes (2026-10-27)

Measured on the New Tab page in dark mode: tab strip `#131313`, toolbar `#212121`, address bar
`#171717`, pages `#111111`, menus and cards (surface 1) `#1B1B1B`, divider `#5E5E5E`, primary
`#FFB94D`, tonal container `#56442A`. All zero chroma except the primary and secondary. The
picker's Default and Grey swatches show amber.

Still blue, found by the user and in the build:

- Settings (and other WebUI pages) used Google's fixed blue and grays: Chromium 156 loads
  `chrome://theme/colors.css` there only with `kWebuiRefresh2026`. With the feature on,
  Settings, Extensions, History and Downloads were charcoal with amber toggles and selection.
  `patches/pantera/ui/default-theme-webui-colors.patch` enables it by default. Focus rings in
  those pages were blue for the same reason; native focus rings already use
  `kColorSysStateFocusRing` (the primary).
- Tab favicons of Chromium's pages (Settings, Downloads, Bookmarks, History, Flags, Passwords,
  Extensions/plugins, Management) are static blue PNGs. `scripts/recolor-favicons.swift`
  recolors their blue pixels to amber tone 55 (`#B37800`: 3.7:1 on white, 4.3:1 on `#212121`,
  like Google blue's 4.5 and 3.6) into `branding/overlay/`. The New Tab and product favicons are
  Chromium's logo, left to TASK-19.
- Smaller hard-coded blues remain (help bubbles, the address bar's starter-pack icons, Reading
  mode's blue theme, the "Blue" tab group color): TASK-25.

### Verified (2026-10-27)

Incremental builds (`ninja -C out/Default chrome`, about 3 minutes each after the first),
ad-hoc signed, on a test profile over DevTools:

- Dark: tab strip `#131313`, toolbar `#212121`, address bar `#171717`, pages `#111111`, surfaces
  `#1B1B1B`, primary `#FFB94D`, focus ring `#FFB94D`. Light: tab strip `#E2E2E2`, header
  container `#F1F1F1`, toolbar `#FFFFFF`, address bar `#EEEEEE`, surfaces `#FAFAFA`, primary
  and focus ring `#815500`.
- Settings, About, Extensions, History and Downloads follow the theme (charcoal or white,
  amber toggles, selection and links). The Settings favicon renders amber.
- The picker shows Default (amber, selected) first; choosing a blue chip gave a generated blue
  primary (`#AAC7FF`) and Default brought amber back.
- The user looked at the browser frame and menus in the first build and only flagged the blue
  focus rings and favicons, both fixed since (`default-theme-focus-ring.patch`: macOS replaced
  the theme's focus ring with the system keyboard focus color when no theme color is set).
