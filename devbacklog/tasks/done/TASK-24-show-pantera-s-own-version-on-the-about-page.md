---
id: TASK-24
title: Show Pantera's own version on the About page
priority: medium
labels: [phase-0, branding, release]
depends_on: []
created: 2026-10-27
completed: 2026-10-27
---

## Goal

The About page reads like Brave's: Pantera's own version first, the Chromium version it's built
on under it, and a greyed note about updates. Asked for by the user on 2026-10-27 (DECISION-9).

## Acceptance criteria

- [x] Pantera's version lives in `VERSION` (0.1.0) with `make bump-patch|minor|major` and
      `make version`
- [x] The About page shows "Pantera 0.1.0 (Official Build) (arm64)" and "Chromium:
      156.0.8078.47 (ungoogled-chromium)"
- [x] A greyed line says Pantera doesn't check for updates yet
- [x] Checked in a build

## Notes

The user suggested "Pantera is up to date", greyed. Pantera doesn't check for updates, so that
would be untrue whenever a newer release exists; the page says "Pantera doesn't check for
updates yet" instead (`IDS_SETTINGS_ABOUT_PAGE_NO_UPDATE_CHECKS`). Switch to
`IDS_SETTINGS_UPGRADE_UP_TO_DATE` if the user prefers their wording; an updater (TASK-6) should
replace the line with a real status.

How it works (2026-10-27):

- `build.sh` appends `PANTERA_VERSION=<VERSION>` to the BRANDING file after copying the overlay.
  `chrome/common/chrome_version.h` is generated from `chrome/VERSION` and BRANDING
  (`process_version("version_header")`), and `patches/pantera/branding/pantera-version.patch`
  adds `PANTERA_VERSION_STRING` to its template.
- `patches/pantera/settings/about-page-versions.patch`: three messages in
  `settings_strings.grdp` (Pantera version, Chromium version, no update checks; English only
  for now), `settings_localized_strings_provider.cc` fills `aboutBrowserVersion`,
  `aboutChromiumVersion` and `aboutNoUpdateChecks`, and `about_page.html.ts` shows the three
  lines. Chromium's "Official Build" string is "Official Build, ungoogled-chromium" in
  ungoogled-chromium, so the Pantera line has its own message.
- `settings_strings.grdp`, `settings_localized_strings_provider.cc` and `about_page.html.ts` are
  also changed by ungoogled-chromium's patches and domain substitution, so the patch was made
  from each file just before the edit, and its context has no substituted domains.
- The user sent their own Makefile's bump targets as an attachment, but macOS denied reading it
  from the shared pasteboard folder; the targets follow the usual pattern and can be aligned.
- Verified 2026-10-27 in the build: About shows "Pantera doesn't check for updates yet",
  "Pantera 0.1.0 (Official Build) (arm64)" and "Chromium: 156.0.8078.47 (ungoogled-chromium)".
