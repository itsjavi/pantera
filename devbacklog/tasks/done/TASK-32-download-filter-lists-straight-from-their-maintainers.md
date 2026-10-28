---
id: TASK-32
title: Download filter lists straight from their maintainers
priority: high
labels: [phase-2, shields, privacy]
depends_on: [TASK-31]
created: 2026-10-28
completed: 2026-10-28
---

## Goal

An ad-block service, ported from brave-core, keeps adblock-rust engines loaded with Brave's
default filter lists, downloaded from their maintainers (DECISION-15), and with uBlock Origin's
scriptlets and redirect resources bundled as a separate file (DECISION-14).

## Acceptance criteria

- [x] The service holds Brave's two engines (lists with first-party protections, and everything
      else) on a background sequence and rebuilds them when lists change
- [x] A pinned copy of Brave's list catalog is bundled; Brave's desktop defaults and the regional
      lists for the browser's languages are on
- [x] Lists download from their catalog URLs without cookies, are cached in the profile, refresh
      per `! Expires` (14 days at most) and retry after an hour
- [x] The engines are serialized to the profile and loaded from there at startup
- [x] `resources.json` is generated at build time from pinned uBlock Origin and Brave
      adblock-resources sources and ships beside its GPL-3.0 license in the app bundle
- [x] A setting turns list updates off, and the README lists the hosts contacted
- [x] No P3A or other telemetry code is ported
- [x] Chrome links the engine, and `chrome://credits` lists adblock-rust and its crates
- [x] Checked in the build: lists download on first run, the cache is used after a restart, and
      nothing is downloaded with updates off

## Notes

### What was built (2026-10-28)

- `pantera/components/adblock/` (no `//chrome` dependency):
  - `ad_block_engine*` are brave-core's `AdBlockEngine` and `AdBlockEngineWrapper`, imported
    unmodified first. Pantera's version takes adblock-rust's request type strings instead of
    Blink's enum (TASK-8 maps them), and drops histograms, tracing, debug info and the regex
    discard policy. It also fixes `StripProceduralFilters`, which looped forever on a
    non-string entry.
  - `filter_list_preprocessor` is a C++ port of what Brave's packager
    (brave/brave-core-crx-packager) does to each list on Brave's servers: `!#if` resolution,
    dropping `+js(brave-…)` from non-Brave lists, and a rule adblock-rust can't parse. The port
    keeps an `!#else` inside a disabled block disabled, which the packager got wrong.
  - `filter_list_catalog` parses Brave's catalog. `ad_block_service` is Pantera's own, simpler
    than Brave's provider and component-updater layers:
    - It has one per browser process, with files in `<user data>/AdBlock/`: `lists/` and the
      serialized engines plus a key of the files they came from.
    - Downloads use `SimpleURLLoader` with credentials omitted and the cache disabled, one at a
      time, 10 seconds after a start with missing lists (otherwise 1 minute), then hourly
      checks.
    - A download that is empty, HTML, or has unbalanced `!#if` blocks keeps the old copy.
    - Its state is in local state prefs: `pantera.adblock.list_updates_enabled`,
      `pantera.adblock.lists` (per-list overrides, for TASK-34) and
      `pantera.adblock.list_downloads`.
  - Lists that are on: the catalog's `default_enabled` entries, plus the regional lists whose
    `langs` contain the language of the browser's UI locale, as Brave does. Mobile-only entries
    are skipped. User overrides win.
- `pantera/components/adblock/resources/build_adblock_resources.mjs` builds `resources.json` in
  a GN node action from uBlock Origin 1.75.0 and brave/adblock-resources@6da9b247, both pinned in
  `downloads-adblock.ini` and unpacked by `build.sh`. It does what Brave's packager does, plus
  one change: the 33 scriptlets uBlock Origin marks `requiresTrust` get permission bit 0, so
  only lists Brave's catalog trusts can run them (Brave's packager leaves them open to every
  list). The bundled data is 213 resources, about 1 MB.
- `pantera/browser/` starts the service in `PreProfileInit` and registers its prefs.
  `patches/pantera/browser/add-pantera-browser-parts.patch` hooks it into
  `//chrome/browser:core` and `//chrome/browser/prefs:impl`.
  `patches/pantera/shields/bundle-adblock-data.patch` puts the catalog, `resources.json`, a
  README and both licenses in the framework's `Resources/adblock`.

### Open points

- There's no settings UI for turning updates off yet: the README explains how to edit Local
  State until TASK-34 adds the switch.
- Brave's packager also sanity-checks each list against known-good requests before shipping
  it. That isn't ported.
- DECISION-16 said lists with first-party protections still block first-party requests with
  `$important`. brave-core's code doesn't: in standard mode it drops every default-engine match
  for first-party requests, `$important` included. The port follows the code
  (`AdBlockEngineWrapperTest.ImportantDefaultRules`), and on 2026-10-28 the user chose that
  behavior: DECISION-17 replaces DECISION-16.

### Checked (2026-10-28)

- `pantera_unittests`: 27 tests pass, in batches and with `--single-process-tests`.
  - Catalog parsing and the preprocessor.
  - Standard and aggressive mode, `$important`, cosmetic merging and procedural stripping.
  - The service with a test URL loader and mock time: first download, blocking, saved engines,
    refresh after `Expires`, no requests with updates off, broken downloads, and locale and
    user list choices.
- `ninja chrome` built. The framework has `Resources/adblock` with `resources.json`,
  `list_catalog.json`, the README and both licenses. `about_credits.html` lists adblock,
  cssparser, uBlock Origin and Brave adblock-resources.
- An ad-hoc signed copy of the app ran headless with `--vmodule=ad_block_service=1`:
  - On a fresh profile, it found 62 lists in the catalog with 5 on, downloaded their 30 files
    in about 2.4 seconds, and built the engines from 30 lists (8.5 MB and 2.2 MB serialized).
  - On restart, it loaded the saved engines in about 40 ms, and the one-minute check found
    nothing due.
  - With `list_updates_enabled` false in Local State on a fresh profile, it made no list
    request (checked in a net log) and created no AdBlock folder.
- Not run: `build.sh` from scratch. The new download step was run by hand with the same
  commands, and the patches were applied by hand in series order.
