---
id: TASK-8
title: Block ad and tracker requests with adblock-rust
priority: high
labels: [phase-2, shields]
depends_on: [TASK-32]
created: 2026-10-27
---

## Goal

Network requests that match the filter lists are blocked, redirected or cleaned up, with per-site
shields settings and a per-tab count of what was blocked, ported from brave-core.

## Acceptance criteria

- [ ] Content settings for shields on/off and the ad-block mode (standard or aggressive) per site,
      with a global default
- [ ] A proxying URL loader factory, installed in `WillCreateURLLoaderFactory`, checks the
      subresource requests of frames and workers against the engines and fails blocked ones with
      `ERR_BLOCKED_BY_CLIENT`
- [ ] `$redirect` answers with the bundled resource, `$removeparam` rewrites the URL and `$csp`
      adds its directives to documents
- [ ] First-party requests follow DECISION-17, and nothing is blocked on a site with shields off
- [ ] A tab helper counts blocked requests per page for the panel (TASK-7)
- [ ] Checked in the build: ad and tracker requests are blocked on a news site, first-party ones
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
