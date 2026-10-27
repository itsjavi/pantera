---
id: TASK-19
title: Replace Chromium's logo in the UI
priority: low
labels: [phase-0, branding, ui]
depends_on: []
created: 2026-10-27
---

## Goal

The browser UI shows the panther wherever it shows a product logo. In the first builds (TASK-4,
TASK-5) Chromium's logo is still in the Settings toolbar, the "About Pantera" item in Settings,
the address bar chip on `chrome://` pages and the app menu's "Set Pantera as your default
browser" item (user's screenshots, 2026-10-27).

## Acceptance criteria

- [ ] One monochrome panther glyph, generated from the icon's head path in `make-icon.swift`
- [ ] The Settings toolbar shows the panther in light and dark mode
- [ ] The "About Pantera" item in the Settings menu shows a Pantera icon
- [ ] The address bar chip on `chrome://` pages and the app menu's default browser item show it
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
