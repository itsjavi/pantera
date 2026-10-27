---
id: TASK-12
title: Set up CI builds
priority: low
labels: [build, ci]
depends_on: [TASK-4]
created: 2026-10-27
---

## Goal

Build Pantera automatically on new Chromium releases.

## Acceptance criteria

- [ ] To be defined, including where it runs

## Notes

A Chromium build doesn't fit GitHub-hosted runners' 6-hour limit and disk. Upstream chains
several jobs on Depot runners; its workflows are in ungoogled-chromium-macos's `.github/`, which
wasn't imported.

Upstream's `building.yml` (2026-10-27) runs on `depot-macos-latest`: a `retrieve-resources` job,
then up to 20 chained `build_job_NN` jobs that each resume from the previous job's build-directory
artifact. `itsjavi/pantera` is private, so GitHub-hosted macOS minutes would also be billed at
macOS rates. Options: Depot (a paid third-party account), GitHub's larger macOS runners (paid), or
a self-hosted runner on the user's own Mac. DECISION-6 is about services Pantera users talk to,
so CI isn't ruled out by it. For releases, follow the user's other apps (see TASK-5).
