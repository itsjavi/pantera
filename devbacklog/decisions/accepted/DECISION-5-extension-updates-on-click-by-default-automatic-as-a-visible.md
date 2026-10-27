---
id: DECISION-5
title: Extension updates: on click by default, automatic as a visible opt-in
date: 2026-10-27
tasks: [TASK-3, TASK-13]
---

## Context

Extensions from the Chrome Web Store update through Google's update server, which sees the IP
address and the list of installed extensions on every check. Chrome checks in the background
every few hours; ungoogled-chromium removed update checks entirely. DECISION-4 chose click-only
updates. The user then asked to let people opt in to automatic updates.

## Decision

- By default, extensions update only when the user presses Update in chrome://extensions. No
  background, startup, policy, external-provider or extension-initiated (`runtime.requestUpdateCheck`)
  checks, and no usage pings.
- Users can turn on automatic updates. While it's on, Pantera shows a clear hint that extensions
  update in the background and that each check tells Google the IP address and extension list.
- Every automatic check goes through one switch in the extension updater, so the setting only
  has to flip that switch.

## Consequences

- TASK-3 ships the click-only behavior and the switch (always off). TASK-13 adds the setting and
  the hint.
- Visiting the store, installing, and updating on click contact Google as a direct result of a
  user action. The README says so.
- Replaces DECISION-4.
