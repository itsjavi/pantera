# Pantera Browser: agent guide

Pantera is a privacy-first Chromium browser for macOS built on ungoogled-chromium-macos. Read
[README.md](README.md) for the goals, principles and roadmap.

## Scratch space

Put every temporary file (experiments, renders, downloaded sources, notes) in `.local/` at the
repository root. It is git-ignored. Don't leave scratch files anywhere else in the repository.

## Layout

- `ungoogled-chromium/`: git submodule, pinned to the commit upstream macOS uses. Never edit it.
- `patches/series`: patch order. Upstream's macOS patches first, then Pantera's.
- `patches/ungoogled-chromium/macos/`: upstream's macOS patches. Edit only during an upstream sync.
- `patches/pantera/`: Pantera's patches to Chromium.
- `branding/overlay/`: files copied over Chromium's source after patching, mirroring its tree
  (`BRANDING`, `Assets.car`, `app.icns`, product logos). Use it for binary files and whole-file
  replacements that patches can't carry.
- `branding/AppIcon.icon/`: Icon Composer source of the app icon.
- `scripts/`: `make-icon.swift` draws the icon layers, `build-branding.sh` regenerates every
  branding asset, `preview-icons.swift` renders a built app's icon beside system apps, and
  `rebrand_strings.py` (run by `build.sh`) renames Chromium to Pantera in the UI strings and
  rekeys their translations.

## Upstream

- Base: ungoogled-chromium-macos `cb57ca4773476197e810b251dea6623907bff40a` (Chromium 156.0.8078.47).
  The commit `chore: import ungoogled-chromium-macos at cb57ca4` holds the unmodified copy.
- Not imported: `.github/` (upstream's release CI), `README.md`, `announcements.md`.
- Pantera replaces upstream's `set-bundle-identifier.patch` with `patches/pantera/macos/set-bundle-identifier.patch`
  and the bundle ID in `branding/overlay/.../BRANDING`.

To sync with a newer upstream:

```bash
git remote add upstream https://github.com/ungoogled-software/ungoogled-chromium-macos.git  # once
git fetch upstream
git diff <base> upstream/master -- . ':!.github' ':!README.md' ':!announcements.md' ':!ungoogled-chromium' | git apply -3
git -C ungoogled-chromium fetch
git -C ungoogled-chromium checkout "$(git ls-tree upstream/master ungoogled-chromium | awk '{print $3}')"
```

Commit the sync on its own (`chore: sync ungoogled-chromium-macos to <sha>`), then any Pantera
fix-ups in separate commits, and update the base above. Conflicts are most likely in `build.sh`,
`sign_and_package_app.sh`, `patches/series` and the bundle ID patch.

## Rules

- **Privacy:** no telemetry, ever. A feature that contacts a server by default must be documented
  in the README and be possible to turn off.
- **Patches:** keep them small and single-purpose, add them to the end of `patches/series`, and
  start each with a comment saying what it changes and why.
- **Branding:** edit `scripts/make-icon.swift` or `branding/AppIcon.icon/icon.json`, run
  `scripts/build-branding.sh`, and commit the generated files with the source change.
- **Commits:** Conventional Commits. Third-party code is committed unmodified first;
  modifications go in separate commits.

## Validation

A full Chromium build takes hours and about 100–150 GB of disk, so don't start one unless asked.

- Shell scripts: `bash -n <script>`.
- Patch series and files: `devutils/check_patch_files.sh` (needs the submodule).
- A Pantera patch without a Chromium checkout: download the files it touches for the pinned
  version into `.local/` (`https://chromium.googlesource.com/chromium/src/+/refs/tags/<version>/<path>?format=TEXT`,
  base64-encoded) and run `patch --dry-run -p1` there.
- Icon: run `scripts/build-branding.sh`, then look at `branding/icon-light.png` and `branding/icon-dark.png`.
- String rename: download `tools/grit` (`https://chromium.googlesource.com/chromium/src/+archive/refs/tags/<version>/tools/grit.tar.gz`),
  the two branded `.grd` files, `settings_chromium_strings.grdp` and their `.xtb` translations
  into `.local/`, keeping Chromium's paths, then run `scripts/rebrand_strings.py` on that folder.
  Check that every translation ID that matched a message before still matches one after.

## Known gaps (Phase 0)

- Inflected forms of "Chromium" stay in 11 languages (az, bs, da, et, fi, hr, hu, no, sl, sv, uz),
  e.g. Finnish "Chromiumin". Fixing them needs per-language replacements.
- No auto-updates (Sparkle) and no CI yet.
