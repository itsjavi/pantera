---
id: TASK-16
title: Sync bookmarks and preferences through iCloud
priority: medium
labels: [phase-3, sync, privacy]
depends_on: [TASK-5]
created: 2026-10-27
---

## Goal

Users can opt in to syncing bookmarks and a chosen set of preferences between their Macs through
their own iCloud account. No cookies, passwords, history or other sensitive data, and no Pantera
server, per DECISION-6.

## Acceptance criteria

- [ ] To be defined. Likely: off by default, a setting to turn it on, bookmarks sync first, then
      an allowlist of preferences, and the README says what syncs and where it's stored

## Notes

- Needs iCloud entitlements, which need a Developer ID provisioning profile embedded in the app.
  Apple's Developer ID page confirms CloudKit for Developer ID apps. Apple's capability table
  seems to list key-value storage too, but that wasn't confirmed. This builds on the signing in
  TASK-5.
- Storage: the CloudKit private database, with record fields in `encryptedValues` so they're
  end-to-end encrypted even without Advanced Data Protection. iCloud key-value storage holds only
  1 MB, so at most it fits preferences.
- Option A, custom CloudKit sync: watch the bookmark model and a preference allowlist, map
  changes to records, and handle merges, moves and deletes. Full control, larger patch.
  Recommended.
- Option B, Chromium's sync engine with its local sync backend (built for Windows roaming
  profiles, stores everything in one file) pointed at iCloud Drive. Cheap, but it assumes one
  machine at a time: two Macs open at once give conflicting copies. Not end-to-end encrypted
  without Advanced Data Protection. It's from memory whether the backend runs on macOS, so check
  it in a short spike before ruling it out.
- Open decision: which preferences sync. Site permissions may count as sensitive.
- Docs: https://developer.apple.com/support/developer-id/
