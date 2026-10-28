---
id: TASK-33
title: Hide ads and run scriptlets with cosmetic filtering
priority: high
labels: [phase-2, shields]
depends_on: [TASK-8]
created: 2026-10-28
completed: 2026-10-28
---

## Goal

Element hiding, procedural filters and scriptlets, ported from brave-core's cosmetic filters, so
pages lose the ads that request blocking alone can't remove.

## Acceptance criteria

- [x] Pantera has its own isolated world; a render frame observer asks the browser for the
      frame's cosmetic resources at document start over Mojo
- [x] brave-core's `content_cosmetic.ts` and `procedural_filters.ts` are ported and built with
      Chromium's TypeScript tooling into a resource
- [x] Specific and generic hiding, procedural filters and scriptlets work; scriptlets run in the
      page's main world, and trusted scriptlets only come from lists allowed to use them
- [x] Standard and aggressive modes behave as DECISION-17 says, and nothing runs with shields off
- [x] Checked in the build: a test page's elements are hidden, a `+js()` scriptlet runs, and
      YouTube plays without ads

## Notes

- brave-core sources: `components/cosmetic_filters/` (Mojo interfaces, `CosmeticFiltersResources`,
  renderer `CosmeticFiltersJSHandler` and `CosmeticFiltersJsRenderFrameObserver`) and
  `components/cosmetic_filters/resources/data/`.
- The element picker (`element_picker.ts`) is left for a later task.

### Plan (2026-10-28)

- Import brave-core's files unmodified at 2f49070 into `pantera/components/cosmetic_filters/`
  (common, browser, renderer, resources/data), then adapt them in separate commits.
- Browser: `CosmeticFiltersResources` answers on the engines' sequence, owned by `AdBlockService`
  so it never outlives the engines, and checks the site's shields settings (DECISION-20).
  `//pantera/browser` registers its binder through a small patch to
  `chrome_browser_interface_binders.cc`.
- Renderer: the frame observer and JS handler, without the element picker, de-AMP and perf
  tracing, created from `ChromeContentRendererClient` by a small patch; a Pantera isolated world
  in `chrome_isolated_world_ids.h`.
- Resources: `ts_library` and `bundle_js` build `content_cosmetic.ts` and
  `procedural_filters.ts` into one script, packed with grit into Chrome's resources.
- Brave's feature defaults are fixed: sync load, procedural filtering on, its polling and
  throttling numbers.
- Brave's TypeScript compiles under Chromium's strict settings with 8 small errors to fix.

### Checked in the build (2026-10-28)

Test pages served from `.local/` on 127.0.0.1, mapped to real hosts with
`--host-resolver-rules` in a scratch profile, with the lists Pantera downloaded:

- `palemoon.org`: `palemoon.org##+js(set, abd, false)` ran in the page's main world (the page's
  own script saw `window.abd === false`). Generic rules from an unprotected list hid their boxes,
  including one added a second later. In standard mode `##.sponsored-ad`, only in EasyList (in
  Brave's default list, with first-party protections), left a box with a sentence of text, which
  brave-core's script treats as the site's own content; aggressive mode hid it. With shields down
  nothing was hidden and the scriptlet didn't run.
- `kleinezeitung.at`: the procedural `.bg-gray-100:has-text(Anzeige)` hid only the box with the
  word, and the site-specific `.min-h-\[200px\]` hid its box.
- YouTube: the cookie wall was gone with shields up (it blocks the page with shields down), and
  two videos played from the start without an ad, with `--autoplay-policy=no-user-gesture-required`
  so they started on their own.
- Trusted scriptlets: `AdblockRustTest.TrustedScriptletsNeedAPermittedList`; the service passes
  each list's permission from Brave's catalog.

`CosmeticFiltersResourcesTest` covers shields down, standard and aggressive modes, frames from
other sites and YouTube.

