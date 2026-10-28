---
id: TASK-8
title: Block ad and tracker requests with adblock-rust
priority: high
labels: [phase-2, shields]
depends_on: [TASK-32]
created: 2026-10-27
completed: 2026-10-28
---

## Goal

Network requests that match the filter lists are blocked, redirected or cleaned up, with per-site
shields settings and a per-tab count of what was blocked, ported from brave-core.

## Acceptance criteria

- [x] Content settings for shields on/off and the ad-block mode (standard or aggressive) per site,
      with a global default
- [x] A proxying URL loader factory, installed in `WillCreateURLLoaderFactory`, checks the
      subresource requests of frames and workers against the engines and fails blocked ones with
      `ERR_BLOCKED_BY_CLIENT`
- [x] `$redirect` answers with the bundled resource (`$removeparam` and `$csp` moved to TASK-35)
- [x] First-party requests follow DECISION-17, and nothing is blocked on a site with shields off
- [x] A tab helper counts blocked requests per page for the panel (TASK-7)
- [x] Checked in the build: ad and tracker requests are blocked on a news site, first-party ones
      are allowed in standard mode and blocked in aggressive mode

## Notes

adblock-rust and Brave's browser code are MPL-2.0: files taken from them keep that license and
their source must be published. Download lists straight from their maintainers, not through
Brave's servers.

Split on 2026-10-28: the engine build is TASK-31, filter lists TASK-32, cosmetic filtering
TASK-33 and the settings page TASK-34. This task is now the network blocking.

- brave-core sources: `browser/net/brave_proxying_url_loader_factory.cc`,
  `brave_ad_block_tp_network_delegate_helper.cc`, `brave_ad_block_csp_network_delegate_helper.cc`,
  `components/brave_shields/content/browser/adblock_stub_response.cc`,
  `brave_shields_web_contents_observer.cc`, `brave_shields_tab_helper.cc`, `brave_shields_utils.cc`.
- Brave adds its content settings types through mojom enum extensions; Pantera patches
  `content_settings_types.mojom` and the registry directly.
- Left for later tasks: WebSocket blocking, CNAME uncloaking, the interstitial for blocked
  top-level pages, DevTools details.

### What was built (2026-10-28)

- Content settings: `patches/pantera/shields/add-shields-content-settings.patch` adds
  `PANTERA_SHIELDS` and `PANTERA_FIRST_PARTY_ADS`. Both are ALLOW by default, keyed on the
  top-level site, inherited in incognito and unsynced. The UMA and site settings tables list
  every type, so they gain the two too.
  - `pantera/components/shields/shields_settings` reads and writes them for a site and its
    subdomains (`[*.]example.com`, or the bare host for IPs and localhost). Their defaults are
    the global settings, which TASK-34 exposes.
  - In Preferences the exceptions sit under `pantera_shields` and `pantera_first_party_ads`:
    the registry's names with underscores.
- `pantera/browser/adblock/ad_block_proxying_url_loader_factory` is installed after the
  extensions' webRequest proxy by `patches/pantera/shields/block-ad-requests.patch`.
  - It holds each request until the engine answers, then forwards it untouched, fails it with
    `ERR_BLOCKED_BY_CLIENT` and `should_collapse_initiator`, or answers it with the `$redirect`
    resource. The stub response is Brave's `adblock_stub_response`, imported unmodified first.
  - It skips what Brave skips: non-http(s) URLs, top-level page loads, and requests from
    extensions.
  - The verdict is Brave's: `$important`, or a match without an exception. youtube.com
    initiators are always aggressive.
  - Settings come from the top-level site: the request's trusted isolation info (navigations),
    then the factory's, then the tab's page, then the initiator (workers).
- `AdBlockTabHelper` counts blocked requests, and keeps the first 500 URLs, for the current page.
  It is created on the first block and resets when the primary page changes.
- Found while checking: the service read its catalog at best-effort priority, which Chrome holds
  back until startup ends, so pages loaded at startup weren't checked. The engine sequence now
  loads the saved engines and `resources.json` first, and requests queue behind that load (unit
  test in `UsesSavedEnginesAndRefreshesWhenDue`).

### Checked (2026-10-28)

- `pantera_unittests`: 35 tests pass, adding shields settings, the stub response and the startup
  ordering.
- `ninja chrome` built. An ad-hoc signed copy ran headless on a local test page served from
  127.0.0.1, with the external hosts mapped to it. Each case was run twice with the same result.
  - Standard: the page's own `/fbevents.js` (an EasyPrivacy generic rule) loaded. The same file
    from another host, a Taboola script and `popads.net/pop.js` were blocked; the last one was
    answered with uBlock Origin's `popads.net.js` resource and loaded.
  - Aggressive (`pantera_first_party_ads` set for 127.0.0.1): the page's own `/fbevents.js` was
    blocked too.
  - Shields off (`pantera_shields` set for 127.0.0.1): nothing was blocked.
- On real sites in standard mode, edition.cnn.com had 14 requests blocked: DoubleClick, Google
  ad syndication and IMA, Scorecard Research, Quantcast, Optimizely, ml314 and others.
  theguardian.com had one, an events endpoint on guardianapis.com.
- Not run: `build.sh` from scratch. The new patches were applied by hand in series order.

