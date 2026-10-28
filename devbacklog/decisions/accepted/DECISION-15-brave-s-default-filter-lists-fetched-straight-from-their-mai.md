---
id: DECISION-15
title: Brave's default filter lists, fetched straight from their maintainers
date: 2026-10-28
tasks: [TASK-32]
---

## Context

The README promises that filter lists come straight from their maintainers, that this is
documented and that it can be turned off. Brave's list catalog (`filter_lists/list_catalog.json`
in `brave/adblock-resources`, MPL-2.0) records each list's upstream URLs, which lists are on by
default and which get first-party protections (DECISION-16). Brave itself serves the lists from a
mirror through its component updater. The user chose Brave's default set on 2026-10-28.

## Decision

- Pantera bundles a pinned copy of Brave's catalog and turns on the lists Brave turns on for
  desktop: Brave Default (uBlock Origin's filters, badware, unbreak and quick fixes, EasyList,
  URLhaus, Brave's unbreak and site lists), Privacy (EasyPrivacy, uBlock Origin privacy), Brave
  First Party, Cookie notices and Mobile app promos, plus the regional lists for the browser's
  languages.
- Each list is downloaded from the URL in the catalog, never through Brave or a Pantera server
  (DECISION-6).
- A list is refreshed when its `! Expires` header says, at most every 14 days like Brave, and
  retried after an hour when a download fails.
- A setting turns list updates off. The README lists the hosts contacted.

## Consequences

- About 30 downloads on first run and then on each list's schedule, mostly from
  raw.githubusercontent.com, easylist.to, secure.fanboy.co.nz and malware-filter.gitlab.io, plus
  the regional lists' hosts. Those hosts see the user's IP address.
- Nothing is blocked until the first lists arrive.
- The clean-up Brave's packager does on the lists (dropping `+js(brave-…)` scriptlets from
  non-Brave lists and rules adblock-rust can't use) happens in the browser.
- Catalog changes reach users with Pantera releases.
