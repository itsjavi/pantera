---
id: DECISION-1
title: Build on a copy of ungoogled-chromium-macos
date: 2026-10-27
status: accepted
tasks: [TASK-1]
---

## Context

Pantera needs Chromium without Google services and without telemetry, maintained by one person
with agents. Forking Chromium directly would mean carrying and rebasing every
de-Googling change ourselves.

## Decision

- Start from ungoogled-chromium-macos, copied into this repository rather than forked on GitHub.
  The first commit after the initial one holds upstream's files unmodified (cb57ca4); every
  Pantera change comes in later commits.
- Keep ungoogled-chromium as a git submodule pinned to the commit upstream macOS uses.
- Put Pantera's Chromium changes in small patches under `patches/pantera/`, and binary or
  whole-file replacements in `branding/overlay/`.
- Not imported: upstream's `.github/` CI, README and announcements.

## Consequences

- Upstream syncs follow AGENTS.md: diff upstream from the recorded base, apply, and commit the
  sync separately from any fix-ups.
- Pantera inherits ungoogled-chromium's choices. Undoing one, as TASK-3 does for the Web Store,
  needs a deliberate patch and its own decision.
