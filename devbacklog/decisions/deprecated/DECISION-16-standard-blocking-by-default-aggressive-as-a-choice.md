---
id: DECISION-16
title: Standard blocking by default, aggressive as a choice
date: 2026-10-28
tasks: [TASK-8, TASK-33]
replaced_by: DECISION-17
---

## Context

Brave blocks in two modes. Standard, its default, leaves first-party requests and elements alone
for lists marked with first-party protections in the catalog (DECISION-15), so fewer sites break.
Aggressive applies every rule to first-party content, like uBlock Origin. The user chose Brave's
default on 2026-10-28.

## Decision

- Standard mode is the default. Lists with first-party protections block only third-party
  requests, unless a rule is `$important`. Their cosmetic filters hide only elements judged
  third-party and skip procedural and `:has()` selectors. Other lists and the user's own filters
  always apply.
- Aggressive mode applies every rule everywhere. Users choose it globally or per site.
- Turning shields off for a site stops all blocking there.
- youtube.com is always aggressive, as in Brave.

## Consequences

- Pantera breaks about as many sites as Brave and depends on Brave's first-party and unbreak lists
  to do so.
- The first-party checks (request initiator versus URL, and the element checks in the cosmetic
  script) are ported from brave-core rather than rewritten.
