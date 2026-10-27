---
id: TASK-3
title: Restore Chrome Web Store installs
priority: high
labels: [phase-0, extensions, privacy]
depends_on: []
created: 2026-10-27
---

## Goal

Install extensions from chromewebstore.google.com with "Add to Chrome", as in Brave, without
re-enabling any other Google service.

## Acceptance criteria

- [ ] The update policy below is decided and recorded here
- [ ] "Add to Chrome" on chromewebstore.google.com installs an extension
- [ ] Extension updates follow the decided policy, documented in the README
- [ ] Domain substitution still applies to every file not needed for the Web Store

## Notes

ungoogled-chromium blocks the Web Store in three ways:

- `core/ungoogled-chromium/disable-webstore-urls.patch` removes the store URLs and the extension
  update checks.
- Domain substitution rewrites `google.com` in the files the store needs, including
  `extensions/common/extension_urls.cc`/`.h`, `extensions/browser/updater/extension_downloader.cc`,
  `chrome/browser/extensions/updater/chrome_extension_downloader_factory.cc`,
  `chrome/common/extensions/chrome_extensions_client.cc` and
  `chrome/browser/resources/webstore_app/manifest.json`.
- It also rewrites `chrome/common/extensions/api/_api_features.json`, which limits the
  `webstorePrivate` install API to the store's origin.

Plan: a list of files that `build.sh` keeps out of domain substitution (check each for other
Google URLs first), plus a Pantera patch that restores the store URLs and the install path on top
of ungoogled's patch. Verifying needs a build (TASK-4).

**Decision needed:** how extensions update. Google's update server sees the IP address and the
installed extension list on every check.

- Only when the user presses Update in chrome://extensions (no background contact).
- Automatically every few hours, without usage pings.
- On click now, with an opt-in "update automatically" setting later.
