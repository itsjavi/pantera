---
id: DECISION-9
title: Pantera has its own version, separate from Chromium's
date: 2026-10-27
tasks: [TASK-24]
---

## Context

Pantera showed only Chromium's version ("Version 156.0.8078.47 (Official Build,
ungoogled-chromium) (arm64)"), and releases are named after it
(`<Chromium version>-<ungoogled revision>.<Pantera revision>`). Brave shows its own version first
and Chromium's on a second line. The user asked for the same, starting at 0.1.0, stored somewhere
sensible with an easy way to bump it (2026-10-27).

## Decision

- Pantera has its own semantic version, `major.minor.patch`, starting at 0.1.0.
- It lives in the `VERSION` file at the repository root. `make bump-patch`, `bump-minor` and
  `bump-major` raise it; `make version` prints it.
- `build.sh` passes it to Chromium through the BRANDING file (`PANTERA_VERSION`), which makes it
  `PANTERA_VERSION_STRING` in `chrome_version.h`.
- The About page shows "Pantera <version> (Official Build) (<arch>)" and, on its own line,
  "Chromium: <version> (ungoogled-chromium)".

## Consequences

- Chromium's version stays where Chromium uses it (chrome://version, the user agent,
  `CFBundleShortVersionString`), so web compatibility doesn't change.
- Release names and tags still use the Chromium-based string; moving them, and the bundle
  version an updater compares (TASK-6), to Pantera's version is follow-up work.
