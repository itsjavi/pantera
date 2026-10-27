---
id: TASK-4
title: Build Pantera locally and check the branding
priority: high
labels: [phase-0, build]
depends_on: []
created: 2026-10-27
---

## Goal

A first ad-hoc signed build, to find build breakage early and confirm the branding works in the
running app.

## Acceptance criteria

- [x] About 150 GB free disk and the README's prerequisites installed
- [ ] `MACOS_AD_HOC_SIGNING=1 ./build.sh` finishes with a `.dmg` in `build/`
- [ ] The app is "Pantera Browser" with the panther icon in light and dark mode
- [ ] Profile in `~/Library/Application Support/Pantera Browser`, keychain item "Pantera Safe Storage"
- [ ] The UI says Pantera (About page, menus) in English and at least one other language
- [ ] Every fix needed to get here is committed on its own

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
