---
id: DECISION-17
title: Standard blocking as in Brave's code, aggressive as a choice
date: 2026-10-28
tasks: [TASK-8, TASK-33]
---

## Context

DECISION-16 chose Brave's two blocking modes, with standard as the default. It also said
`$important` rules from lists with first-party protections still block first-party requests.
That came from a misreading of brave-core: its `AdBlockEngineWrapper::ShouldStartRequest`
drops every match of those lists for first-party requests in standard mode, `$important`
included. The user chose to follow Brave's code on 2026-10-28.

## Decision

- Standard mode is the default. Lists with first-party protections (Brave's default engine)
  apply only to third-party requests, whatever their rules say. For third-party requests their
  `$important` rules win over the other lists' exceptions.
- Their cosmetic filters hide only elements judged third-party and skip procedural and `:has()`
  selectors.
- Other lists and the user's own filters always apply, to first- and third-party content.
- Aggressive mode applies every rule everywhere. Users choose it globally or per site.
- Turning shields off for a site stops all blocking there.
- youtube.com is always aggressive, as in Brave.

## Consequences

- Pantera blocks and breaks sites as Brave does, and relies on Brave's first-party and unbreak
  lists for the cases standard mode leaves alone.
- A site's own ads that only a protected list's `$important` rule matches get through in
  standard mode; aggressive mode or another list catches them.
- The first-party checks are ported from brave-core rather than rewritten
  (`AdBlockEngineWrapperTest.ImportantDefaultRules` covers the `$important` case).
