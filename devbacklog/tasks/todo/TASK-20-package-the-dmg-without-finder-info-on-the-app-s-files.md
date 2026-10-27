---
id: TASK-20
title: Package the DMG without Finder info on the app's files
priority: low
labels: [phase-1, release]
depends_on: []
created: 2026-10-27
---

## Goal

The app installed from the `.dmg` passes strict signature verification, so stricter checks than
Gatekeeper's (for example an updater's, TASK-6) accept it.

## Acceptance criteria

- [ ] An app copied out of the `.dmg` has no `com.apple.FinderInfo` attributes
- [ ] `codesign --verify --deep --strict` passes on that copy
- [ ] The `.dmg` keeps the `/Applications` shortcut and is still signed, notarized and stapled

## Notes

Found in TASK-4 and confirmed with the first notarized `.dmg` in TASK-5 (2026-10-27): every one
of the 947 files in an installed copy has an empty `com.apple.FinderInfo` attribute, and strict
verification fails with "resource fork, Finder information, or similar detritus not allowed".
Gatekeeper (`spctl -t exec`) and `syspolicy_check distribution` accept the copy anyway, so the
release isn't blocked.

The attributes come from `hdiutil makehybrid -hfs` in Chromium's `pkg-dmg`, which
`sign_and_package_app.sh` calls. The app in `build/src/out/Default` has none. Likely fix: build
the image with `hdiutil create -srcfolder` from a staging folder holding the app and an
`/Applications` symlink, and check whether `pkg-dmg` has an option that avoids the hybrid step.
