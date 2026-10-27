---
id: TASK-4
title: Build Pantera locally and check the branding
priority: high
labels: [phase-0, build]
depends_on: []
created: 2026-10-27
completed: 2026-10-27
---

## Goal

A first ad-hoc signed build, to find build breakage early and confirm the branding works in the
running app.

## Acceptance criteria

- [x] About 150 GB free disk and the README's prerequisites installed
- [x] `MACOS_AD_HOC_SIGNING=1 ./build.sh` finishes with a `.dmg` in `build/`
- [x] The app is "Pantera Browser" with the panther icon in light and dark mode
- [x] Profile in `~/Library/Application Support/Pantera Browser`, keychain item "Pantera Safe Storage"
- [x] The UI says Pantera (About page, menus) in English and at least one other language
- [x] Every fix needed to get here is committed on its own

## Notes

Blocked on disk space: 63 GB free on 2026-10-27 (M1 Max, 10 cores, 32 GB RAM, Xcode 27.1 with
the macOS 27.0 SDK; use `build.sh -s` if Chromium 156 rejects that SDK).

Prerequisites on 2026-10-27: node, readline and the Metal toolchain are installed, binutils isn't
(nothing to unlink). Missing: python@3.13, ninja, coreutils, esbuild, typescript, and the PySocks
and httplib2 Python modules.

GitHub-hosted runners don't fit a first build (see TASK-12), so this one runs locally.

### Build Mac (2026-10-27)

The first Mac didn't have the disk space, so the build moved to a second one: M5 Pro, 18 cores,
48 GB RAM, 756 GB free, macOS 27.0.1, Xcode 27.1 with the macOS 27.0 SDK.

What it had:

- Already there: Homebrew 7.0.11, readline 8.3.6, Go 1.27.3 (`retrieve_and_unpack_resource.py`
  needs `go` for Dawn, but the README doesn't list it), node 26 from mise (ahead of Homebrew in
  `PATH`). binutils isn't installed, so there was nothing to unlink.
- Installed: python@3.13 3.13.18, ninja 1.13.2, coreutils 9.12, node 26.12.1, esbuild 0.28.2,
  typescript 7.0.2 (the native Go `tsc` that `fix-typescript.patch` points at), and the Metal
  toolchain 27A266a.
- `xcode-select` pointed at the Command Line Tools. Instead of `sudo xcode-select -s`, the build
  runs with `DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer`.
- The default `python3` is Homebrew's 3.15, and Homebrew's Pythons refuse a global
  `pip3 install` (PEP 668). PySocks 1.7.1 and httplib2 0.32.0 went into a Python 3.13 venv in
  `.local/py313`, whose `bin/` goes first in `PATH` for the build so `python3` is 3.13.

### Build log

1. First run: `gclient sync` failed checking out `third_party/litert/src` after 10 minutes.
   `~/.gitconfig` sets up Git LFS with `filter.lfs.required true`, but `git-lfs` wasn't
   installed. LiteRT keeps only its prebuilt `.so`/`.dylib`/`.dll` files in LFS, and
   `prune_binaries.py` removes prebuilt binaries anyway, so their contents aren't needed.
   Fix: `brew install git-lfs` (3.8.1) and build with `GIT_LFS_SKIP_SMUDGE=1`. A Mac without
   LFS in its git config isn't affected. Reran without `-d`: in `build.sh`, `-d` switches to
   the official source tarball, while the default clone path already reuses an existing
   checkout (`git fetch` + `reset --hard`, then `gclient sync`).
2. Second run: everything up to `build_bindgen.py` worked (clone, patches, domain substitution,
   branding overlay, string rename, toolchains, GN). Linking bindgen failed: the bundled
   LLVM 23.1 `ld64.lld` rejects every `.tbd` in the macOS 27.0 SDK as malformed ("unknown
   target"), because they list a new `arm64e.x1-macos` target. Chromium 156 expects SDK 26.5
   (`mac_sdk_official_version` in `build/config/mac/mac_sdk.gni`), and the Command Line Tools
   ship `MacOSX26.5.sdk` alongside 27.0. Fix:
   `build.sh -s /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk`.
3. Third run: finished in 3 h 18 min (06:36–09:54), 57,495 ninja steps, exit 0. It produced
   `build/pantera-browser_156.0.8078.47-1.1_macos.dmg` (155 MB). The app is ad-hoc signed,
   passes `codesign --verify --deep --strict`, and has bundle ID `com.itsjavi.pantera`,
   `CFBundleName` "Pantera", `CFBundleDisplayName` "Pantera Browser" and `CrProductDirName`
   "Pantera Browser". The string rename renamed 765 mentions and rekeyed 46,008 translations
   in 81 languages. No Chromium patch was needed: both failures were in the build environment.

The command that worked:

```bash
PATH="$PWD/.local/py313/bin:$PATH" DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer \
  GIT_LFS_SKIP_SMUDGE=1 MACOS_AD_HOC_SIGNING=1 \
  ./build.sh -s /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk > .local/build.log 2>&1
```

The README's Building section now covers all of this (Go, the Python venv, the macOS 26 SDK,
`DEVELOPER_DIR`, Git LFS).

### The app crashed at launch

The first DMG's app aborted in `main` before any browser code ran: `dlopen` of the framework
failed with "mapping process and mapped file (non-platform) have different Team IDs". Ad-hoc
signatures have no Team ID, and `sign_and_package_app.sh` (unchanged from upstream) signs ad-hoc
builds with the `library` and hardened `runtime` options, both of which turn on library
validation. Removing `library` alone wasn't enough: hardened runtime implies library validation.
Fix (`fix(build): sign ad-hoc builds without library validation`): ad-hoc signing leaves out
`library` and `runtime` and keeps `restrict` and `kill`. Developer ID signing is unchanged.
Upstream's current script has the same problem.

### Checked in the running app (2026-10-27, installed from the DMG into /Applications)

- Name: bundle name "Pantera", display name "Pantera Browser", bundle ID `com.itsjavi.pantera`.
- Icon: `Assets.car` has `AppIcon` stacks for Aqua (light), DarkAqua (dark) and Tintable. This
  Mac is in dark mode, and Icon Services draws icons in the system style whatever appearance
  the drawing process asks for, so only the dark variant could be rendered here
  (`scripts/preview-icons.swift`): the silver panther with the amber eye on near-black, next to
  Notes, Safari and Finder. The light variant still needs a look with the Mac in light mode.
- Profile: the default launch created `~/Library/Application Support/Pantera Browser`; no
  `Chromium` folder.
- Keychain: login keychain item "Pantera Safe Storage", account "Pantera"; no "Chromium Safe
  Storage".
- UI, live: `chrome://settings/help` captured over the DevTools protocol (a throwaway profile
  with `--remote-debugging-port`; this session has no Screen Recording or Accessibility access).
  English: "About Pantera", "Pantera", "Pantera is made possible by the Chromium open source
  project". Spanish (`-AppleLanguages '(es)'`): "Información de Pantera", "Pantera se ha creado
  gracias al proyecto de software libre Chromium".
- Menus, from the locale packs: `BuildMainMenu` fills `IDS_ABOUT_MAC`, `IDS_HIDE_APP_MAC` and
  `IDS_EXIT_MAC` with `IDS_PRODUCT_NAME`, which is "Pantera" in both packs. That gives "About /
  Hide / Quit Pantera" and "Información de / Ocultar / Salir de Pantera". The live menu bar
  still needs a look.
- The user checked the two remaining items on the running app: the app menu shows About, Hide
  and Quit Pantera, and with the Mac in light mode the Dock icon is the light panther.

Follow-ups found: TASK-17 (Spanish and other translations credit "Los creadores de Pantera"
instead of the Chromium Authors), TASK-18 (52 English strings outside the branded catalogs still
say Chromium, such as the Incognito new tab page), TASK-19 (Chromium's logo in the Settings
toolbar in dark mode and in the About menu icon), and a DMG note in TASK-5.
