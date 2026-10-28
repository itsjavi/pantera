---
id: TASK-33
title: Hide ads and run scriptlets with cosmetic filtering
priority: high
labels: [phase-2, shields]
depends_on: [TASK-8]
created: 2026-10-28
---

## Goal

Element hiding, procedural filters and scriptlets, ported from brave-core's cosmetic filters, so
pages lose the ads that request blocking alone can't remove.

## Acceptance criteria

- [ ] Pantera has its own isolated world; a render frame observer asks the browser for the
      frame's cosmetic resources at document start over Mojo
- [ ] brave-core's `content_cosmetic.ts` and `procedural_filters.ts` are ported and built with
      Chromium's TypeScript tooling into a resource
- [ ] Specific and generic hiding, procedural filters and scriptlets work; scriptlets run in the
      page's main world, and trusted scriptlets only come from lists allowed to use them
- [ ] Standard and aggressive modes behave as DECISION-16 says, and nothing runs with shields off
- [ ] Checked in the build: a test page's elements are hidden, a `+js()` scriptlet runs, and
      YouTube plays without ads

## Notes

- brave-core sources: `components/cosmetic_filters/` (Mojo interfaces, `CosmeticFiltersResources`,
  renderer `CosmeticFiltersJSHandler` and `CosmeticFiltersJsRenderFrameObserver`) and
  `components/cosmetic_filters/resources/data/`.
- The element picker (`element_picker.ts`) is left for a later task.
