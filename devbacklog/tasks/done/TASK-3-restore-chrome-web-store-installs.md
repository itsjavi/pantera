---
id: TASK-3
title: Restore Chrome Web Store installs
priority: high
labels: [phase-0, extensions, privacy]
depends_on: [TASK-4]
created: 2026-10-27
completed: 2026-10-27
---

## Goal

Install extensions from chromewebstore.google.com with "Add to Chrome", as in Brave, without
re-enabling any other Google service.

## Acceptance criteria

- [x] The update policy is decided: DECISION-5, on click by default; the opt-in setting is TASK-13
- [x] "Add to Chrome" on chromewebstore.google.com installs an extension (needs a build, TASK-4)
- [x] Extensions update only from the Update button in chrome://extensions, with no usage pings
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

Moved back to todo on 2026-10-27, code complete, so that TASK-4 can be the one task in progress.
Pick it up again as soon as the TASK-4 build exists.

### Verified in the TASK-4 build (2026-10-27)

Pantera 156.0.8078.47, ad-hoc signed, with a throwaway profile. The browser was driven over
the DevTools protocol (`--remote-debugging-port` on 127.0.0.1) and recorded with
`--log-net-log` (capture mode Default, so cookie values are stripped but their headers show).

Install:

- The store page has `chrome.webstorePrivate` (23 methods). The store shows "Switch to Chrome?"
  because the browser's brand is Chromium, as it does for Brave, but "Add to Chrome" works.
- Pressing "Add to Chrome" and confirming the native "Add extension" dialog (the user clicked
  it; no test switch auto-confirms it in Chromium 156) installed uBlock Origin Lite
  2026.1026.1812: `from_webstore: true`, update URL `clients2.google.com/service/update2/crx`.
- The download was `clients2.google.com/service/update2/crx?response=redirect&…&x=id=…&installsource=ondemand&uc`,
  redirected to `clients2.googleusercontent.com/crx/blobs/…`. No ping parameter. It carried
  the google.com cookies that the store's consent page had just set: the store page starts the
  download, as in Chrome. Visiting the store also runs the site's own Google Analytics and
  `play.google.com/log` calls. Both are covered by the README ("Pantera contacts Google only
  when you visit the store, install an extension, or press Update").

Updates, on a second launch with the extension installed:

- Launch at 11:09:42, three minutes idle on about:blank: no network requests of any kind. No
  extension update check (an unpatched build checks seconds after startup: `Start()` calls
  `CheckSoon()`), no component updates, nothing else.
- 11:13:11, `chrome.runtime.requestUpdateCheck()` from the extension's page: returned
  `no_update` (the gate answers without a request), then `throttled`. No network request.
- 11:13:57, the Update button in chrome://extensions (Developer mode): one request, the only
  one in the session, to `clients2.google.com/service/update2/crx` with
  `x=id=…&v=2026.1026.1812&installedby=internal&uc` and `x-goog-update-interactivity: fg`.
  No ping parameter (no `ping=r=…`) and no Cookie header (`site_for_cookies` is null). Google
  answered 200; the extension was already current, so nothing changed.

Not observed directly: the scheduled check (every 5 hours, with no switch to shorten it in
Chromium 156). It goes through the same `CheckNow()` call with background priority as the
startup check, so the same gate stops it.
