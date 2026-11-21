---
name: upgrade-chromium
description: Upgrade Pantera to the latest Chromium that ungoogled-chromium-macos supports and raise its own dependencies (uBlock Origin, Brave adblock-resources, adblock-rust), then build the app. Use when asked to upgrade, update or sync Chromium, upstream or Pantera's dependencies, or to prepare a release on a newer Chromium.
---

# Upgrade Chromium and Pantera's dependencies

The procedure is [docs/upgrading.md](../../../docs/upgrading.md). Read it first and follow its
steps in order; `scripts/upgrade.py` does the mechanical parts. This file adds what an agent needs
to know to run it without stopping.

## Before starting

- Run `scripts/upgrade.py status` and tell the user in one or two lines what will change. If
  nothing is newer, say so and stop.
- The target Chromium is the one ungoogled-chromium-macos's `master` is on (DECISION-1). When
  Chrome stable or ungoogled-chromium is ahead of it, say so, and don't jump ahead unless the
  user asks: upstream's macOS patches would have to be ported by hand.
- Create a backlog task (`backlog.py new`, labels `build,release` plus the phase) and move it to
  in progress. Record what was raised, which patches needed rebasing and why, and anything that
  couldn't be verified in its Notes.
- Leave uncommitted changes that aren't yours alone. Stage only your own hunks.

## While working

- Start the source download (step 4) as soon as the sync is committed, in the background, and do
  steps 3 and 5's preparation meanwhile. It deletes `build/src/out`; that's expected.
- Steps 4 and 7 take from 30 minutes to several hours. Run them in the background with a log in
  `.local/`, and don't poll: wait for the notification.
- A patch that fails needs reading, not forcing. Look at what Chromium changed
  (`git -C build/src log -3 -- <file>` has little history in a shallow clone; read the code) and
  keep the patch doing what its header says. If the header is no longer true, rewrite it.
- When Chromium now does what a patch did, drop the patch from `patches/series` and delete it,
  and say so in the commit.
- Don't edit `patches/ungoogled-chromium/macos/` to make a build pass unless upstream is broken;
  if you must, note it in the task so the next sync expects the conflict.
- Build errors in `pantera/` after an upgrade are usually Chromium API changes. Fix them in
  `pantera/`, copy with `ditto`, and continue the ninja build. Don't rerun `build.sh`: it starts
  from zero.

## Commits

Conventional Commits, in this order, so the history shows what came from where:

1. `chore: sync ungoogled-chromium-macos to <sha>`: upstream's changes only.
2. `chore: record the upstream base ...`: `AGENTS.md`, `README.md`, `Makefile`.
3. `chore(adblock): raise ...`: the ad-block data pins.
4. `chore(rust): import adblock-rust <version> ...`: crate sources, unmodified.
5. `build(rust): ...`: the regenerated crates patch and build files.
6. `fix(patches): rebase ... on Chromium <major>`: Pantera's patches, one commit per area when
   the reasons differ.
7. `chore(backlog): ...`: the task, when everything is verified.

Never push, tag or publish a release unless the user asks.

## Done means

- `make check` passes and `scripts/upgrade.py apply-patches` applies every patch with no fuzz.
- The app builds, `pantera_unittests` passes, and the checks in step 8 of the guide were done on
  the built app, with screenshots sent to the user in light and dark mode.
- The user knows what was upgraded, what wasn't and why, and what is left for them (signing with
  the Developer ID and notarizing ask for their keychain; publishing is their call).
