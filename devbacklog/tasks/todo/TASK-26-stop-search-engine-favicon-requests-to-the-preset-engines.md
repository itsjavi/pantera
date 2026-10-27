---
id: TASK-26
title: Stop search engine favicon requests to the preset engines
priority: medium
labels: [phase-0, privacy]
depends_on: []
created: 2026-10-27
---

## Goal

Pantera doesn't contact search engines the user isn't using. Each such request shows the user's
IP address to that company.

## Acceptance criteria

- [ ] Find what triggers the requests (Settings' Search engine page, a first-run step, the
      search engine list's icons)
- [ ] The preset engines' icons come from bundled resources, or the requests happen only when
      the user acts, and the README says so
- [ ] A network log of a fresh profile shows no requests to search engines the user didn't use

## Notes

Found 2026-10-27 in the network log of a TASK-22/23 test session (fresh profile, Spanish region):
at one moment, seven requests to the preset search engines' favicons, nothing else in the
session:

- `https://cdn.ecosia.org/assets/images/ico/favicon.ico`
- `https://cdn.search.brave.com/serp/favicon.ico`
- `https://duckduckgo.com/favicon.ico`
- `https://es.search.yahoo.com/favicon.ico`
- `https://www.bing.com/favicon.ico`
- `https://www.perplexity.ai/favicon.ico`
- `https://www.privacywall.org/images/favicon_32x32.ico`

The user was browsing Settings at the time, so the Search engine page is the likely trigger.
Upstream Chromium behavior, not from Pantera's patches.
