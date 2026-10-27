<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="branding/icon-dark.png">
    <img src="branding/icon-light.png" width="160" alt="Pantera Browser icon: a black panther in profile">
  </picture>
</p>

<h1 align="center">Pantera Browser</h1>

<p align="center">A privacy-first Chromium browser for macOS. No Google services, no telemetry, nothing extra.</p>

---

Pantera is stock Chromium without Google's services, with privacy protections on by default and
built-in content blocking inspired by Brave Shields. It is deliberately small: no crypto wallet,
rewards, VPN, AI assistant or news feed. And unlike most browsers, it collects no usage statistics
at all.

> **Status: pre-alpha.** The repository builds on
> [ungoogled-chromium-macos](https://github.com/ungoogled-software/ungoogled-chromium-macos) with
> Pantera's branding. No release has been built yet.

## Principles

- **No Google services.** Inherited from [ungoogled-chromium](https://github.com/ungoogled-software/ungoogled-chromium):
  no Safe Browsing, sync, translation or other requests to Google. The one exception is the
  Chrome Web Store, and only when you use it (see [Extensions](#extensions)).
- **Nothing phones home.** No usage statistics, crash reports or "daily active user" pings, opt-in
  or otherwise.
- **Private by default.** Protections are on out of the box. Anything that contacts a server
  (for example, filter list updates) is documented and can be turned off.
- **Small patch set.** Pantera stays close to upstream so Chromium security fixes ship quickly.

## Extensions

Install extensions from the [Chrome Web Store](https://chromewebstore.google.com) with "Add to
Chrome", as in Chrome or Brave.

Extensions don't update in the background. To update them, open `chrome://extensions`, turn on
**Developer mode** and press **Update**. Each check tells Google's update server your IP address
and the list of installed store extensions, so Pantera only checks when you ask, and sends no
usage pings. An opt-in setting for automatic updates is planned.

Pantera contacts Google only when you visit the store, install an extension, or press Update.

## Roadmap

| Phase | Goal | Status |
|---|---|---|
| 0. Foundation | ungoogled-chromium-macos with Pantera branding, Chrome Web Store installs, first signed and notarized build | In progress |
| 1. Updates and controls | Sparkle auto-updates. A per-site panel for scripts, cookies and HTTPS upgrades | Planned |
| 2. Content blocking | Brave's [adblock-rust](https://github.com/brave/adblock-rust) engine: request blocking, element hiding, blocked counter, filter lists fetched straight from their maintainers | Planned |
| 3. More protections | "Forget me when I close this site", fingerprinting protections | Planned |

The detailed goals are in [devbacklog/ROADMAP.md](devbacklog/ROADMAP.md) and the work is tracked
as tasks in [devbacklog/](devbacklog/backlog.md).

## How it's built

Pantera is a set of patches and files applied to Chromium's source at build time:

1. `build.sh` downloads Chromium's source for the version in `ungoogled-chromium/chromium_version.txt`.
2. It applies ungoogled-chromium's patches, the macOS patches in `patches/ungoogled-chromium/` and
   Pantera's own patches in `patches/pantera/`, in the order listed in `patches/series`.
3. It copies `branding/overlay/` over the source (product name, bundle ID, icons) and renames
   Chromium to Pantera in the UI strings and their translations.
4. It builds with the flags in `ungoogled-chromium/flags.gn` and `flags.macos.gn`, then signs and
   packages a `.dmg` with `sign_and_package_app.sh`.

| Path | Contents |
|---|---|
| `ungoogled-chromium/` | ungoogled-chromium, as a git submodule pinned to a release |
| `patches/` | macOS patches from upstream and Pantera's own patches |
| `branding/` | App icon source (`AppIcon.icon`), icon renders, and the overlay copied over Chromium's source |
| `scripts/` | Icon and branding generators |
| `entitlements/`, `flags.macos.gn`, `downloads-*.ini` | macOS build configuration from upstream |

## Building

A full build takes several hours and needs about 100–150 GB of free disk space.

**Requirements:** a Mac with Apple Silicon, Xcode 26 or later (keep the app open while
building), and [Homebrew](https://brew.sh).

```bash
brew install python@3.13 go ninja coreutils readline node esbuild typescript
xcodebuild -downloadComponent MetalToolchain
brew unlink binutils  # only if it's installed
```

Clone with the submodule. The build scripts need Python 3.13 as `python3`, with PySocks and
httplib2; Homebrew's Python refuses a global `pip install`, so give the build its own virtual
environment:

```bash
git clone --recurse-submodules <repository-url> pantera-browser
cd pantera-browser
/opt/homebrew/opt/python@3.13/bin/python3.13 -m venv build/python
build/python/bin/pip install PySocks httplib2
```

Then build:

```bash
export PATH="$PWD/build/python/bin:$PATH"
MACOS_AD_HOC_SIGNING=1 ./build.sh -s /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk
```

- Chromium 156 builds with the macOS 26 SDK. The macOS 27 SDK in Xcode 27 doesn't work: the
  linker bundled with Chromium rejects its `.tbd` files. Pass a macOS 26 SDK with `-s`, such as
  the Command Line Tools' `MacOSX26.5.sdk` above or the one in Xcode 26.
- If `xcode-select -p` prints `/Library/Developer/CommandLineTools`, point the build at Xcode
  with `export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer`.
- If your git config sets up Git LFS, install `git-lfs` and build with `GIT_LFS_SKIP_SMUDGE=1`.
  The clone fails without `git-lfs`, and the LFS files (prebuilt binaries) are removed anyway.

The `.dmg` appears in `build/`. Ad-hoc builds are signed without hardened runtime and library
validation, because library validation needs a Team ID and ad-hoc signatures have none. Useful
options:

- Running `./build.sh` again reuses the cloned source.
- `./build.sh -d` uses Chromium's official source archive instead of a shallow git clone.
- `./build.sh -s <path-to-SDK>` builds against a specific macOS SDK.
- `./sign_and_package_app.sh` re-runs signing and packaging without rebuilding.

To sign and notarize with a Developer ID instead of ad-hoc signing, set:

| Variable | Value |
|---|---|
| `MACOS_CERTIFICATE_NAME` | Certificate name, e.g. `Developer ID Application: Your Name (TEAMID)` |
| `PROD_MACOS_NOTARIZATION_APPLE_ID` | Apple Account email |
| `PROD_MACOS_NOTARIZATION_TEAM_ID` | Developer Team ID |
| `PROD_MACOS_NOTARIZATION_PWD` | App-specific password |

## Credits and license

Pantera builds on [Chromium](https://www.chromium.org/),
[ungoogled-chromium](https://github.com/ungoogled-software/ungoogled-chromium) and
[ungoogled-chromium-macos](https://github.com/ungoogled-software/ungoogled-chromium-macos).
Code imported from them is under the BSD-3-Clause license in [LICENSE](LICENSE). The license for
Pantera's own additions hasn't been chosen yet.
