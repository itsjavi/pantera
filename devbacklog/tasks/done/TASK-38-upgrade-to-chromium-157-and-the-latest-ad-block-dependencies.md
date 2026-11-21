---
id: TASK-38
title: Upgrade to Chromium 157 and the latest ad-block dependencies
priority: high
labels: [phase-1, build, release]
depends_on: []
created: 2026-11-21
completed: 2026-11-21
---

## Goal

Pantera builds on the latest Chromium that ungoogled-chromium-macos supports, with its own
dependencies raised to their latest releases, and the upgrade is repeatable: the user expects to
do it often and asked for a script and a skill for it (2026-11-21).

## Acceptance criteria

- [x] Upstream is synced to ungoogled-chromium-macos 84768e7 (Chromium 157.0.8103.78), committed
      on its own, with the new base recorded in AGENTS.md
- [x] uBlock Origin and Brave adblock-resources are pinned to their latest releases
- [x] adblock-rust is raised with gnrt to the newest version Chromium 157's crates allow (0.13.5),
      sources committed unmodified
- [x] Every patch in `patches/series` applies to Chromium 157 without fuzz
- [x] `make check` passes
- [x] The app builds, and `pantera_unittests` passes
- [x] The built app was checked: About page versions, New Tab page, Shields panel and blocking,
      in light and dark mode
- [x] `scripts/upgrade.py`, `docs/upgrading.md` and the `upgrade-chromium` skill cover the
      procedure
- [x] A release build for end users exists: signed with the Developer ID and notarized

## Notes

- Versions on 2026-11-21: Chrome stable on macOS was mid-rollout, 157.0.8103.79/.80 for about 75%
  of users and 158.0.8127.36 for 25%. ungoogled-chromium had 158.0.8127.36; ungoogled-chromium-macos
  was on 157.0.8103.78. Pantera follows upstream macOS (DECISION-1), so this upgrade stops at
  157.0.8103.78.
- Upstream's sync brought Crubit (`build_crubit.sh`, `rust_crubit_root`, the rustc-dev and crubit
  downloads), Rust nightly 2026-09-08, a new `add-local-network-usage-description.patch`, and
  dropped three patches (`fix-disable-verification-token`, both `fix-gn-compatibility`).
- Patches that needed work on 157: `ntp/grow-the-favorites-grid` (context moved) and
  `ntp/follow-the-system-theme` (Chromium now has the observation, for Android only; the patch
  lifts the guard). New: `macos/local-network-usage-description` renames Chromium in upstream's
  new permission text.
- Ad-block data: uBlock Origin 1.75.0 to 1.76.6, brave/adblock-resources 6da9b24 to b84ad0f.
  `resources.json` goes from 213 to 224 resources; `list_catalog.json` is unchanged.
- adblock-rust 0.13.4 moved the saved engine format to v7. Pantera's `LoadSerialized` returns
  false for a file it can't read and the service rebuilds the engines from the cached lists.
- `build.sh -k` keeps a tree that was pruned and patched by hand, so an upgrade doesn't download
  Chromium's dependencies twice.
- adblock-rust stops at 0.13.5: 0.13.6 (adds `$to=`) needs serde_json 1.0.154 and base64 0.23,
  and Chromium 157 vendors 1.0.151 and 0.22. Raising it means replacing crates Chromium's own
  code uses, so it waits for a Chromium that has them. 0.13.4 dropped the `url` crate, so `url`
  and `form_urlencoded` left `third_party/rust`.
- `git fetch upstream` brought 370 upstream tags, named like Pantera's releases. They were deleted
  (each matched upstream's tag exactly) and `remote.upstream.tagOpt` is now `--no-tags`;
  `sync-upstream` sets that itself.
- Xcode's Metal toolchain was missing after an Xcode update (`xcrun metal` failed); installed
  with `xcodebuild -downloadComponent MetalToolchain`, as `make setup` does.

### Kernel panic during the first build (2026-11-21)

- Eleven minutes into ninja (step 8543 of 57884) the Mac froze and panicked: "watchdog timeout:
  no checkins from watchdogd in 93 seconds". 48 GB of RAM, 18 cores, ninja at its default 20 jobs.
- The panic report showed 15 MB free, 26 GB in the compressor and 230 Python processes holding
  39.5 GB: ninja's 20 Python actions, 11 of which had 19 children each.
- Cause: Chromium 157 split `//third_party/blink/renderer/bindings:generate_bindings` into 11
  per-type actions "to run in parallel on RBE". Each starts `multiprocessing.Pool(cpu_count)`,
  and with macOS's spawn start method every worker loads its own Web IDL database. One action
  alone peaks at about 13 GB here.
- Fix: `patches/pantera/gn/serialize-bindings-generation.patch` puts the 11 actions in a GN
  pool of depth 1. Measured on the step alone: 22 Python processes and 13 GB at most, about a
  minute in total. Worth reporting to Chromium or ungoogled-chromium-macos; not done.
- The outputs of the crashed run were deleted and the build restarted from `gn gen`, because a
  hard crash can leave incomplete files that ninja takes as built.

### Build and checks (2026-11-21)

- The second compile ran to the end with no failed step: 57866 steps in about 1 hour 45 minutes,
  never below 74% free memory, at most 21 Python processes. Nothing in `pantera/` needed changes
  for Chromium 157.
- `build/pantera-browser_157.0.8103.78-1.1_macos.dmg` is the ad hoc signed package (158 MB).
- `pantera_unittests`: 42 of 42 pass.
- Checked on a copy of the built app with scratch profiles, in dark and light mode: About page
  (Pantera 0.1.0, Chromium 157.0.8103.78), `chrome://version`, the New Tab page, Settings and
  Extensions (both say Pantera), and the Shields panel on theverge.com (42 and 35 blocked). The
  filter lists downloaded on first run and the engines were saved in the new format.
- Web Store: the detail page offers the install button and the "Add extension" prompt appears.
  Confirming it wasn't done: the prompt is a native dialog and the session had no Accessibility
  permission to click it. Check one install by hand before releasing.
- Release build (`make release`, on the user's go-ahead): signed with the Developer ID, app and
  disk image both notarized ("Accepted") and stapled; `spctl` reports "Notarized Developer ID" for
  both and `syspolicy_check distribution` passes. The signed app launches with the hardened
  runtime, loads a page and downloads its filter lists.
  `build/pantera-browser_157.0.8103.78-1.1_macos.dmg`, SHA-256
  `d3c8479aee3030d46598c94e8cade98cc41d0572354f8f6eadcadd83f3069250`. Not published: no tag, no
  GitHub release, nothing pushed.
- Seen while checking, not from this upgrade: `chrome://version` still shows Chromium's logo and
  wordmark (TASK-39).
