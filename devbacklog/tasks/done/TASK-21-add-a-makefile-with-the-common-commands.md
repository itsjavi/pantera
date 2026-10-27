---
id: TASK-21
title: Add a Makefile with the common commands
priority: low
labels: [phase-0, build]
depends_on: []
created: 2026-10-27
completed: 2026-10-27
---

## Goal

The common commands are one short `make` target each, so nobody has to retype the build's
environment (venv on `PATH`, `DEVELOPER_DIR`, `GIT_LFS_SKIP_SMUDGE`, the macOS 26 SDK) or the
signing variables. Asked for by the user on 2026-10-27.

## Acceptance criteria

- [x] `make setup`, `make build`, `make release`, `make check` and `make branding`, with
      `make help` as the default
- [x] The logic stays in the scripts; local overrides go in a git-ignored `local.mk`
- [x] README, `docs/releasing.md` and AGENTS.md mention it

## Notes

Verified on 2026-10-27: `make help` and `make check` ran (scripts, patch series, backlog all
pass). `make -n setup`, `make -n build` and `make -n release` print the same commands that built,
signed and notarized the TASK-4/TASK-5 build. `CERTIFICATE` is read from the keychain
(`security find-identity`), so the Makefile holds no personal details. `make check` deletes
Finder's `.DS_Store` in `patches/` first, because upstream's `check_patch_files.py` reports it
as an unused patch.

Not run for real: `setup` (installs Homebrew packages), `build` (hours), `release` (submits two
notarizations again) and `branding`.
