---
id: TASK-5
title: Ship a signed and notarized release
priority: medium
labels: [phase-0, release]
depends_on: [TASK-4]
created: 2026-10-27
completed: 2026-10-27
---

## Goal

A Developer ID signed and notarized `.dmg` that opens on any Mac without Gatekeeper warnings.

## Acceptance criteria

- [x] Signing and notarization variables set up (see the README's Building section)
- [x] `spctl -a -vvv -t install "Pantera Browser.app"` reports "Notarized Developer ID"
- [x] The release process is written down

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

Found in the TASK-4 build (2026-10-27): copying the app out of the DMG gives all 946 of its
files an empty `com.apple.FinderInfo` attribute (from `hdiutil makehybrid -hfs` in `pkg-dmg`), and
then `codesign --verify --deep --strict` fails with "resource fork, Finder information, or
similar detritus not allowed". The copy in `build/src/out/Default` has none, and non-strict
verification passes. Check whether this matters for a notarized DMG (Gatekeeper, `spctl`) and,
if it does, build the DMG another way (for example `hdiutil create -srcfolder`). Answered below:
Gatekeeper accepts it; the fix is TASK-20.

### First signed and notarized build (2026-10-27)

Signed the TASK-4 build in place, without rebuilding:
`MACOS_CERTIFICATE_NAME="Developer ID Application: Francisco Javier Aguilar Sanchez (9MM6NQSSQA)"
PROD_MACOS_NOTARIZATION_PROFILE=notary ./sign_and_package_app.sh`. The whole run took about
6 minutes. The user moved the Developer ID certificate and key from their other Mac as a `.p12`
into the login keychain, and saved a notarytool profile named `notary` (app-specific password).

Script changes:

- The app's designated requirement now pins the Team ID (codesign's default requirement).
  Upstream's custom one accepted any Developer ID signature with the bundle ID, so another
  developer's app could have read "Pantera Safe Storage". The signed app's requirement ends
  with `certificate leaf[subject.OU] = "9MM6NQSSQA"`.
- Notarization takes a keychain profile (`PROD_MACOS_NOTARIZATION_PROFILE`) or the Apple ID
  variables, and missing credentials stop the script before it signs anything.
- The `.dmg` is signed, notarized and stapled too, after the app.

Results:

- App: notarytool `Accepted` (submission `10bc6315-2389-44d0-ba41-bc91409085f6`), stapled.
  Flags `kill,restrict,library-validation,runtime`, secure timestamp, Team ID `9MM6NQSSQA`.
- `.dmg`: `Accepted` (submission `b8970ed5-798e-4001-817d-65045b4b5604`), stapled.
  `build/pantera-browser_156.0.8078.47-1.1_macos.dmg`, SHA-256
  `7cd11585100c41125b726533ac85fa0335076e59b9647e9b675d2f6a9476dba6`.
- `spctl -a -vvv` with `-t install` and `-t exec` on the app, and `-t open --context
  context:primary-signature` on the `.dmg`: `accepted`, `source=Notarized Developer ID`.
  `syspolicy_check distribution` passes.
- Simulated download: a copy of the `.dmg` with a Safari quarantine attribute, mounted, and the
  app copied out. Gatekeeper accepts the copy as Notarized Developer ID, its stapled ticket
  validates, and `syspolicy_check` passes. Strict `codesign` verification fails on its Finder
  info attributes (TASK-20).

The release process is in `docs/releasing.md`. Not done here: publishing a GitHub Release (the
user decides when), and CI releases (TASK-12).
