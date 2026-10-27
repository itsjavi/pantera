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
