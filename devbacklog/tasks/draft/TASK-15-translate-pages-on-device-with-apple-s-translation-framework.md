---
id: TASK-15
title: Translate pages on device with Apple's Translation framework
priority: medium
labels: [phase-3, translate, privacy]
depends_on: [TASK-4]
created: 2026-10-27
---

## Goal

Pantera translates web pages and selected text on the Mac itself, with Apple's Translation
framework, instead of Google's translate service. No Pantera server, per DECISION-6.

## Acceptance criteria

- [ ] To be defined. Likely: a "Translate selection" context menu item first, then full-page
      translation from Chromium's translate bubble, hidden on macOS versions without the
      framework, and a README note on what Apple collects

## Notes

- `TranslationSession` exists from macOS 15. Before macOS 26 a session only comes from SwiftUI's
  `.translationTask()`, which a hidden or popover `NSHostingView` could host. macOS 26 adds
  `init(installedSource:target:)` without UI, but only for languages already installed. Asking to
  download a language always needs the SwiftUI path.
- `.translationPresentation()` (macOS 14.4) shows the system translation popover for a string,
  a cheap way to translate a selection.
- Batch APIs: `translations(from:)` and `translate(batch:)`.
- Translation runs on device. Apple's docs say it may collect usage metrics (bundle ID and the
  language pair, not the text). The README should say so.
- The framework is Swift-only and Chromium is Objective-C++, so it needs a Swift shim with an
  Objective-C interface. Check how Chromium's GN supports Swift on macOS (iOS uses it).
- Chromium already detects the page language locally. Its translate step injects Google's
  script, which would be replaced: collect the page's text nodes, send them to the native side
  in batches, and write the results back. Safari translates pages with the same framework.
  This is a medium-to-large patch, against the small-patch-set rule. Weigh that before starting.
- Open decision: selection only, or full-page translation too.
- Docs: https://developer.apple.com/documentation/translation/translationsession
