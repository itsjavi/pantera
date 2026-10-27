---
id: DECISION-2
title: Product identity: names, bundle ID, profile folder and keychain item
date: 2026-10-27
tasks: [TASK-1, TASK-2]
replaced_by: DECISION-10
---

## Context

macOS ties permissions, keychain access and signing to the bundle ID, and the browser keeps the
key that encrypts passwords and cookies in a keychain item. Changing any of these after release
breaks existing installs.

## Decision

| What | Value |
|---|---|
| App name (`PRODUCT_FULLNAME`) | Pantera Browser |
| Short name, menus and UI strings | Pantera |
| Bundle ID | `com.itsjavi.pantera` |
| Profile folder | `~/Library/Application Support/Pantera Browser` |
| Keychain item | "Pantera Safe Storage", account "Pantera" |
| Icon | Black panther in profile with an amber eye; silver on near-black in dark mode |

## Consequences

Don't change the bundle ID, profile folder or keychain item once Pantera ships. Doing so would
need a migration plan recorded in a new decision.
