---
id: DECISION-11
title: Pantera is licensed under BSD-3-Clause
date: 2026-10-27
tasks: []
---

## Context

Code imported from Chromium, ungoogled-chromium and ungoogled-chromium-macos is BSD-3-Clause, and
the README said the license for Pantera's own additions hadn't been chosen. The user asked for the
least restrictive license compatible with the rest and chose BSD-3-Clause (2026-10-27).

## Decision

- Pantera's own work (scripts, patches, build files, docs, branding sources and assets) is under
  BSD-3-Clause, the same license as upstream.
- The root `LICENSE` covers the whole repository. It keeps the ungoogled-chromium copyright line
  and adds one for "The Pantera Authors".
- The name and icon have no separate trademark policy for now.

## Consequences

- Pantera's patches change Chromium's BSD-3-Clause files, so one license fits them and they can go
  to ungoogled-chromium without relicensing.
- Code taken from projects under other licenses keeps its license in its own files, with a header
  saying so (for example MPL-2.0 files from adblock-rust, TASK-8).
- Chromium's third-party components keep their own licenses; a binary release still has to carry
  their notices, whatever the repository's license.
- MIT and 0BSD were considered. They only drop the no-endorsement clause or the attribution
  requirement, which redistributors already face for the upstream code, and would leave the
  repository with two licenses.
