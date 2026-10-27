---
id: DECISION-10
title: Product identity: names, bundle ID, profile folder, keychain item and a one-color icon
date: 2026-10-27
tasks: [TASK-1, TASK-2, TASK-23]
---

## Context

macOS ties permissions, keychain access and signing to the bundle ID, and the browser keeps the
key that encrypts passwords and cookies in a keychain item. Changing any of these after release
breaks existing installs.

This replaces DECISION-2, whose icon had an amber eye. While designing the New Tab page's panther
(TASK-23), the user chose a one-color panther there and then for the app icon too, in both
appearances (2026-10-27). The amber stays as the UI's primary color (DECISION-8).

## Decision

| What | Value |
|---|---|
| App name (`PRODUCT_FULLNAME`) | Pantera Browser |
| Short name, menus and UI strings | Pantera |
| Bundle ID | `com.itsjavi.pantera` |
| Profile folder | `~/Library/Application Support/Pantera Browser` |
| Keychain item | "Pantera Safe Storage", account "Pantera" |
| Icon | A one-color black panther in profile, the eye and whiskers cut out to the background; silver on near-black in dark mode |

The New Tab page uses the same one-color panther, in a faint tint of the theme's text color.

## Consequences

Don't change the bundle ID, profile folder or keychain item once Pantera ships. Doing so would
need a migration plan recorded in a new decision.
