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

- [x] The update policy is decided: DECISION-5, on click by default; the opt-in setting is TASK-13
- [ ] "Add to Chrome" on chromewebstore.google.com installs an extension (needs a build, TASK-4)
- [ ] Extensions update only from the Update button in chrome://extensions, with no usage pings
      (implemented and documented in the README; needs a build to confirm, TASK-4)
- [x] Domain substitution still applies to every file not needed for the Web Store

## Notes

### What blocked the store

- `core/inox-patchset/0005-disable-default-extensions.patch` empties `WebstoreInstaller::DownloadCrx()`
  and `StartCrxInstaller()`, so installs never download or install anything.
- `core/ungoogled-chromium/disable-webstore-urls.patch` stops `ExtensionDownloader` from starting
  update checks, drops the default store update URL and removes the store's dedicated process.
- `core/ungoogled-chromium/disable-google-host-detection.patch` drops the store's X-Frame-Options
  protection.
- Domain substitution rewrites the store and update URLs in `extensions/common/extension_urls.cc`
  and the store origin that may use the `webstorePrivate` API in
  `extensions/common/api/_api_features.json`.

### What changed

- `patches/pantera/extensions/restore-web-store-installs.patch`: restores the installer.
- `patches/pantera/extensions/harden-web-store-pages.patch`: restores the dedicated process and
  X-Frame-Options protection for store pages.
- `patches/pantera/extensions/update-extensions-on-click.patch`: restores update checks, and gates
  every check in `ExtensionUpdater::CheckNow()`. Only the Update button (the one caller with a
  foreground priority) runs one while `AutomaticUpdatesEnabled()` is false, which TASK-13 will
  make a setting. Store extensions update through `ExtensionDownloader` instead of
  `UpdateService`, whose component-updater server stays disabled.
- `domain-substitution-exclusions.list` + `build.sh`: keep the two files above out of domain
  substitution.

Usage pings stay off without extra code: the downloader only sends them to its
`ping_enabled_domain` ("google.com" in `chrome_extension_downloader_factory.cc`), which domain
substitution still rewrites. Store update requests carry no cookies (Chromium omits credentials
for the store's update URL).

Left as ungoogled-chromium has it: content verification of store extensions
(`chrome_content_verifier_delegate.cc`), which could fetch hashes from Google in the background.

### Verified so far (Chromium 156 sources, no build)

- The whole patch series applies in order, and the patched files match the intended result.
- Running ungoogled's `domain_substitution.py` with the filtered list keeps
  `chromewebstore.google.com` and `clients2.google.com` in the two excluded files and rewrites the
  ping domain to `9oo91e.qjz9zk`.
- `check_patch_files.py` passes.

Left for TASK-4: compile, install an extension from the store, press Update, and confirm with a
proxy or `chrome://net-export` that no update check happens on startup or in the background.
