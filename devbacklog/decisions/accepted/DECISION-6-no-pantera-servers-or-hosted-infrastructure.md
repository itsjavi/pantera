---
id: DECISION-6
title: No Pantera servers or hosted infrastructure
date: 2026-10-27
tasks: [TASK-14]
---

## Context

ungoogled-chromium removes the Google services behind Safe Browsing, CRLSets and other component
updates, web push, sync and translate. Brave restores most of them by proxying the requests
through its own servers, so Google doesn't see the user's IP address. The user doesn't want
Pantera to run servers or infrastructure.

## Decision

- Pantera runs no servers, proxies or hosted services of its own.
- A missing service is restored in one of three ways, or not at all: a macOS system framework
  (for example on-device translation or iCloud), a direct connection to the original provider as
  a documented opt-in, or a local-only implementation.

## Consequences

- Features that talk to Google or another provider do so directly, with the IP address visible
  to them. The README says so and the feature is off by default unless an accepted decision says
  otherwise.
- Brave-style proxied features (Safe Browsing, CRLSets, push) aren't copied as they are. Each
  needs its own choice between direct opt-in and leaving it out.
- Release downloads and the Sparkle feed (TASK-6) still need static file hosting. Whether a
  third-party host such as GitHub Releases is acceptable is decided in TASK-6.
