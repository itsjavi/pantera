---
id: TASK-35
title: Apply $removeparam and $csp filter rules
priority: medium
labels: [phase-2, shields]
depends_on: [TASK-8]
created: 2026-10-28
---

## Goal

Filter rules that change requests instead of blocking them work: `$removeparam` strips tracking
parameters from URLs, and `$csp` adds Content-Security-Policy directives to pages.

## Acceptance criteria

- [ ] `$removeparam` rewrites the URL of matching GET, HEAD and OPTIONS subresource and frame
      requests with an internal redirect, using only the additional engine's result, as Brave does
- [ ] `$csp` adds the engines' directives to matching main-frame and frame documents, in the raw
      headers and in the parsed policies the browser enforces
- [ ] Unit tests cover both, and pages with neither keep loading without an extra hop
- [ ] Checked in the build on pages with `utm_` parameters and a `$csp` rule

## Notes

Split from TASK-8 on 2026-10-28. Blocking only needs to hold a request until the engine answers.
These two need more:

- `$removeparam` needs a per-request URL loader proxy. It sends `OnReceiveRedirect`, waits for
  `FollowRedirect`, then hands the request on with the new URL.
- `$csp` needs a client proxy on navigations that rewrites the response head. The network service
  has already parsed the headers by then, so the new policies have to be added to
  `parsed_headers` too.

brave-core's version is `browser/net/brave_proxying_url_loader_factory.cc` (its InProgressRequest)
and `brave_ad_block_csp_network_delegate_helper.cc`.
