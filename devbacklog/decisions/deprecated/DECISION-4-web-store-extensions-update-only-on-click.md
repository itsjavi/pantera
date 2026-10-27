---
id: DECISION-4
title: Web Store extensions update only when the user asks
date: 2026-10-27
tasks: [TASK-3]
replaced_by: DECISION-5
---

## Context

TASK-3 restores Chrome Web Store installs. Extensions from the store update through Google's
update server, which sees the IP address and the list of installed extensions on every check.
Chrome checks in the background every few hours. ungoogled-chromium removed update checks
entirely.

## Decision

- Installing from chromewebstore.google.com works, as in Brave.
- Extensions update only when the user presses Update in chrome://extensions (developer mode).
  There are no background, startup or extension-initiated update checks, and no usage pings.
- Only the files the store needs are kept out of domain substitution. Every other Google request
  stays disabled.

## Consequences

- Visiting the store and installing or updating an extension contacts Google, always as a direct
  result of a user action. The README says so.
- Extensions can go stale if the user never presses Update. An opt-in "update automatically"
  setting can come later as a new task and decision.
