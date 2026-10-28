---
id: TASK-32
title: Download filter lists straight from their maintainers
priority: high
labels: [phase-2, shields, privacy]
depends_on: [TASK-31]
created: 2026-10-28
---

## Goal

An ad-block service, ported from brave-core, keeps adblock-rust engines loaded with Brave's
default filter lists, downloaded from their maintainers (DECISION-15), and with uBlock Origin's
scriptlets and redirect resources bundled as a separate file (DECISION-14).

## Acceptance criteria

- [ ] The service holds Brave's two engines (lists with first-party protections, and everything
      else) on a background sequence and rebuilds them when lists change
- [ ] A pinned copy of Brave's list catalog is bundled; Brave's desktop defaults and the regional
      lists for the browser's languages are on
- [ ] Lists download from their catalog URLs without cookies, are cached in the profile, refresh
      per `! Expires` (14 days at most) and retry after an hour
- [ ] The engines are serialized to the profile and loaded from there at startup
- [ ] `resources.json` is generated at build time from pinned uBlock Origin and Brave
      adblock-resources sources and ships beside its GPL-3.0 license in the app bundle
- [ ] A setting turns list updates off, and the README lists the hosts contacted
- [ ] No P3A or other telemetry code is ported
- [ ] Checked in the build: lists download on first run, the cache is used after a restart, and
      nothing is downloaded with updates off

## Notes

- brave-core sources: `components/brave_shields/content/browser/ad_block_service.cc`,
  `ad_block_engine.cc`, `ad_block_engine_wrapper.cc`, `core/browser/ad_block_filters_provider*`,
  `ad_block_subscription_*` (direct downloads, closest to what Pantera needs),
  `ad_block_custom_*`, `ad_block_dat_cache_manager`, `filter_list_catalog_entry`.
- Brave downloads subscriptions with `download::BackgroundDownloadService`, which needs patches
  to Chromium's download client list; `network::SimpleURLLoader` may be simpler.
- Brave's packager (`brave/brave-core-crx-packager`, `lib/adBlockRustUtils.js`) builds
  `resources.json` by importing uBlock Origin's `src/js/resources/scriptlets.js` in Node.
