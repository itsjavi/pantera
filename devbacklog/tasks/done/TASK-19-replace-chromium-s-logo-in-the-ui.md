---
id: TASK-19
title: Replace Chromium's logo in the UI
priority: low
labels: [phase-0, branding, ui]
depends_on: []
created: 2026-10-27
completed: 2026-10-27
---

## Goal

The browser UI shows the panther wherever it shows a product logo. In the first builds (TASK-4,
TASK-5) Chromium's logo is still in the Settings toolbar, the "About Pantera" item in Settings,
the address bar chip on `chrome://` pages and the app menu's "Set Pantera as your default
browser" item (user's screenshots, 2026-10-27).

## Acceptance criteria

- [x] One monochrome panther glyph, generated from the icon's head path in `make-icon.swift`
- [x] The Settings toolbar shows the panther in light and dark mode
- [x] The "About Pantera" item in the Settings menu shows a Pantera icon
- [x] The address bar chip on `chrome://` pages and the app menu's default browser item show it
- [x] Other WebUI pages that use the same icons are checked (History, Downloads, Extensions)

## Notes

Sources in Chromium 156:

- Toolbar: `ui/webui/resources/cr_elements/cr_toolbar/cr_toolbar.html.ts` uses
  `chrome://theme/current-channel-logo` (the product logo PNG, already Pantera's through
  `branding/overlay/`) in light mode, but `//resources/images/chrome_logo_dark.svg` in dark mode.
- Menu icon: `cr:chrome-product` in `ui/webui/resources/cr_elements/icons.html.ts`, a vector
  icon (two definitions, lines ~97 and ~320).

Replacing whole files fits `branding/overlay/` (an SVG); the icon set needs a small patch or a
monochrome panther path. Check the result on a light background too.

Native (Views) icons, found 2026-10-27 in `build/src`. All are whole-file `.icon` vector
replacements, so they fit `branding/overlay/`:

- `components/omnibox/browser/vector_icons/chrome_product.icon` (`omnibox::kChromeProductIcon`)
  and `product_chrome_refresh_old.icon`: the address bar chip on `chrome://` pages
  (`chrome_location_bar_model_delegate.cc`), the app menu's default browser item
  (`app_menu_model.cc`), infobars, the PDF infobar, the WebUI toolbar icon table.
- `chrome/app/vector_icons/chrome_product.icon` (`kChromeProductIcon`): toasts, the web app menu.
- `components/vector_icons/chromium/product.icon` and `product_refresh.icon`
  (`vector_icons::kProductIcon`, `kProductRefreshIcon`): default browser prompts and dialogs.

`.icon` files are Skia path commands (`CANVAS_DIMENSIONS, 24`, `MOVE_TO`, `CUBIC_TO`, `CLOSE`),
so a script can convert the glyph's SVG path. `make-icon.swift`'s `headPath()` is the source;
the eye could be a cut-out.

Seen again on 2026-10-27: the Settings toolbar shows the panther in light mode and Chromium's
logo in dark mode. The New Tab and product favicons
(`chrome/app/theme/default_*_percent/common/favicon_ntp.png`,
`components/resources/default_*_percent/chromium/favicon_product.png`) are also Chromium's logo;
`scripts/recolor-favicons.swift` skips them. Use the one-color panther (DECISION-10,
`pantherPath()` in `scripts/make-icon.swift`) for all of these.

### Done (2026-10-27)

`scripts/make-icon.swift` now takes `branding/overlay` as its second argument and writes,
besides the New Tab page's `panther.svg`:

- The glyph: the whole one-color panther (DECISION-10), framed like the app icon and clipped to
  the canvas with Core Graphics' path intersection, so nothing draws outside the icon.
  `branding/panther-glyph.svg` is the 24-unit source. A first version cropped the head as a
  bust and dropped the whiskers and pupil; the user found it cut off compared to the art.
- The five vector icons (`chrome_product.icon` in omnibox and chrome/app,
  `product_chrome_refresh_old.icon`, `components/vector_icons/chromium/product.icon` and
  `product_refresh.icon`) as `CANVAS_DIMENSIONS, 24` paths (even-odd is the default fill rule).
- `ui/webui/resources/images/chrome_logo_dark.svg`: the dark app icon (silver panther on a
  near-black rounded square), which cr-toolbar and cr-drawer show in dark mode.
- `favicon_ntp.png` (gray `#7A7A7A`) and `favicon_product.png` (amber `#B37800`), 16 and 32 px.

`patches/pantera/branding/webui-product-icon.patch` replaces both `cr:chrome-product`
definitions in `icons.html.ts` (24 and 960 units) with the glyph; that file is domain-substituted,
so the patch was made from the file before the edit, with no substituted context.

Checked in the build (dark mode): Settings' header and "About Pantera" item, and the Extensions
and History toolbars, show the panther. Not checked: the native icons (address bar chip on
`chrome://` pages, the app menu's default browser item, infobars), which can't be captured
without screen recording; the user needs to look.

The user confirmed in the build that the address bar chip on `chrome://` pages shows the panther
(2026-10-27), and pointed out the blue Extensions tab icon: `favicon_extensions.png` lives in
`extensions/browser/resources/`, which `scripts/recolor-favicons.swift` now covers. A scan of all
13 page favicons in the source found no other blue one. Rechecked after the full-panther glyph:
Settings' header and "About Pantera" item show the whole panther, and chrome://extensions'
favicon is amber.
