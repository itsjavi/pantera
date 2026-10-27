---
id: TASK-5
title: Ship a signed and notarized release
priority: medium
labels: [phase-0, release]
depends_on: [TASK-4]
created: 2026-10-27
---

## Goal

A Developer ID signed and notarized `.dmg` that opens on any Mac without Gatekeeper warnings.

## Acceptance criteria

- [ ] Signing and notarization variables set up (see the README's Building section)
- [ ] `spctl -a -vvv -t install "Pantera Browser.app"` reports "Notarized Developer ID"
- [ ] The release process is written down

## Notes

The user has an Apple Developer ID.

The user's other apps (lamina, notemd, quickedit) all release the same way, worth reusing here:
`.github/workflows/release.yml` runs on a `vX.Y.Z` tag in a protected `release` environment. It
imports a base64 .p12 into a temporary keychain, stores notarytool credentials from an Apple ID
and app-specific password, builds, signs with `--timestamp`, notarizes and staples the app and
the DMG, and publishes a GitHub Release with the DMG, zip, `SHA256SUMS` and a Sparkle
`appcast.xml` (fed from `releases/latest/download/appcast.xml`). Secrets: `DEVELOPER_ID_P12`,
`DEVELOPER_ID_P12_PASSWORD`, `APPLE_ID`, `APPLE_APP_PASSWORD`, `APPLE_TEAM_ID`,
`SPARKLE_PRIVATE_KEY`. Each one is optional: without them the build is ad-hoc signed and the
release notes say so. Local releases use a notarytool keychain profile (`NOTARY_PROFILE`), and
docs live in each repo's `docs/releasing.md`.

Pantera's `sign_and_package_app.sh` reads `MACOS_CERTIFICATE_NAME` and
`PROD_MACOS_NOTARIZATION_APPLE_ID` / `_TEAM_ID` / `_PWD`, so the same secrets map onto it. The
`itsjavi/pantera` repo has no `release` environment or secrets yet (2026-10-27).
