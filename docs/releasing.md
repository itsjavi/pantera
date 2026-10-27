# Releasing Pantera

A release is a `.dmg` built on a Mac, signed with a Developer ID, notarized by Apple and published
as a GitHub Release. Builds don't run in CI yet (TASK-12), so every step below runs locally.

## One-time setup

1. **Developer ID certificate.** Signing needs the "Developer ID Application" certificate and its
   private key in the login keychain. Signing in to Xcode doesn't bring the key. To move it from
   another Mac, open Keychain Access there, go to **login → My Certificates**, select the
   certificate and choose **File → Export Items…** as `.p12`. Copy the file over by AirDrop or
   USB, double-click it, choose the **login** keychain, then delete the `.p12` from both Macs. Check:

   ```bash
   security find-identity -v -p codesigning
   ```

2. **Notarization credentials.** Create an app-specific password at account.apple.com under
   **Sign-In and Security**, then save it as a notarytool profile in the keychain. The command asks
   for the password:

   ```bash
   xcrun notarytool store-credentials notary --apple-id <apple-account-email> --team-id <TEAMID>
   ```

3. **Build prerequisites:** see [Building](../README.md#building) in the README.

## Version

Pantera's own version (DECISION-9) is in `VERSION`, for example `0.1.0`, and the About page shows
it above the Chromium version. Raise it before a release with `make bump-patch`, `make bump-minor`
or `make bump-major`.

Release files and tags are still named `<Chromium version>-<ungoogled-chromium
revision>.<Pantera revision>`, read from `ungoogled-chromium/chromium_version.txt`,
`ungoogled-chromium/revision.txt` and `revision.txt`, for example `156.0.8078.47-1.1`. To release
Pantera changes on the same Chromium version, raise `revision.txt`.

## Build, sign and notarize

1. Build as in the README. The build is ad-hoc signed.
2. Sign with the Developer ID, notarize and package:

   ```bash
   MACOS_CERTIFICATE_NAME="Developer ID Application: <name> (<TEAMID>)" \
   PROD_MACOS_NOTARIZATION_PROFILE=notary \
   ./sign_and_package_app.sh
   ```

   The script signs every helper, library and the app with the hardened runtime and a secure
   timestamp, notarizes and staples the app, packages the `.dmg` in `build/`, then signs,
   notarizes and staples the `.dmg`. The first time, macOS asks whether `codesign` can use the
   key: choose **Always Allow**. It takes about 10 minutes, most of it waiting for Apple.

   `make release` runs this step and the next. It signs with the first Developer ID identity in
   the keychain and notarizes with the `notary` profile; set `CERTIFICATE` or `NOTARY_PROFILE`
   in a git-ignored `local.mk` to change them.

3. Check that Gatekeeper accepts both. Each `spctl` line must say `source=Notarized Developer ID`:

   ```bash
   spctl -a -vvv -t open --context context:primary-signature build/pantera-browser_*_macos.dmg
   spctl -a -vvv -t exec "build/src/out/Default/Pantera Browser.app"
   syspolicy_check distribution "build/src/out/Default/Pantera Browser.app"
   ```

## Publish

Upload the `.dmg` and its checksum as a GitHub Release, tagged with the version:

```bash
version=156.0.8078.47-1.1
cd build
shasum -a 256 "pantera-browser_${version}_macos.dmg" > SHA256SUMS
gh release create "$version" "pantera-browser_${version}_macos.dmg" SHA256SUMS \
  --title "Pantera $version" --prerelease --generate-notes
```

Keep `--prerelease` while Pantera is pre-alpha.

## Known issues

- Installing from the `.dmg` gives every file in the app an empty `com.apple.FinderInfo`
  attribute, so `codesign --verify --strict` fails on the installed copy. Gatekeeper accepts it.
  See TASK-20.
