---
id: TASK-27
title: Credit the Pantera Authors and the BSD license on the About page
priority: medium
labels: [phase-0, branding, l10n]
depends_on: []
created: 2026-10-27
completed: 2026-10-27
---

## Goal

The About page credits Pantera's authors and says which license Pantera's source code is under
(DECISION-11), as Brave's does for the MPL. Asked for by the user on 2026-10-27 (DECISION-12):

```
Pantera
Copyright 2026 The Pantera Authors. All rights reserved.

Pantera is made possible by the Chromium open source project and other open source software.
Its source code is available under the BSD 3-Clause License, with instructions to build it yourself.
```

## Acceptance criteria

- [x] `IDS_ABOUT_VERSION_COPYRIGHT` and `IDS_ABOUT_VERSION_COMPANY_NAME` say "The Pantera
      Authors", in English and in every translation
- [x] Every translation ID that matched a message before the rename still matches one after
- [x] The About page shows the license sentence, linking to `LICENSE` and to the README's build
      section on GitHub
- [x] The patch applies after the rest of the series
- [x] Checked in a build: the About page in English and Spanish, and the copyright in Finder's
      Get Info

## Notes

How it works (2026-10-27):

- Copyright: `scripts/rebrand_strings.py` no longer spares "Chromium Authors", so
  `IDS_ABOUT_VERSION_COPYRIGHT` and `IDS_ABOUT_VERSION_COMPANY_NAME` become "The Pantera Authors"
  and their translations are rekeyed with the rest. The same string feeds the About page,
  chrome://version and the Get Info copyright (`infoplist_strings_util.cc`).
- License: `patches/pantera/settings/about-page-license.patch` adds
  `IDS_SETTINGS_ABOUT_PAGE_PANTERA_LICENSE` to `settings_strings.grdp` (English only for now),
  `settings_localized_strings_provider.cc` fills `aboutPanteraLicense` with the two GitHub links,
  and `about_page.html.ts` shows it under Chromium's credit. github.com isn't in ungoogled's
  domain substitution list, so the links survive it.
- The links assume the repository is public at `github.com/itsjavi/pantera` and the README keeps
  its "Building" heading.

Checked without a build (2026-10-27):

- Ran the old and new script on Chromium's original catalogs (`git -C build/src archive HEAD`).
  The new one renames 2 more messages and rekeys 162 more translations (2 × 81 languages), and
  nothing else changes. Translations matching a message: 65,853 in 162 catalogs, the same
  before and after. Spanish reads "Copyright 2026 Los creadores de Pantera", German "Die
  Pantera-Autoren".
- Bosnian, Croatian and Slovenian keep the inflected "Autori Chromiuma" / "Ustvarjalci
  Chromiuma" in the company name, like the other inflected forms (TASK-11).
- Rebuilt the three patched files from `build/src`'s HEAD through ungoogled-chromium's series
  and Pantera's: the new patch applies last and gives the intended files, and it also applies
  to the domain-substituted tree.

Checked in a build (2026-10-27): applied the patch and ran the new rename on `build/src`, then an
incremental `ninja -C out/Default chrome` (1,610 steps, no errors). A headless run of the built
app, driven over the DevTools protocol with `-AppleLanguages (es)` for Spanish:

- English: "Copyright 2026 The Pantera Authors. All rights reserved.", then Chromium's credit,
  then "Its source code is available under the BSD 3-Clause License, with instructions to build
  it yourself", linking to `.../blob/main/LICENSE` and `.../pantera#building`.
- Spanish: "Copyright 2026 Los creadores de Pantera. Todos los derechos reservados." and the
  translated Chromium credit; the license sentence is in English, like TASK-24's version lines.
- Get Info: `NSHumanReadableCopyright` and `CFBundleGetInfoString` in the app's
  `en.lproj/InfoPlist.strings` say "The Pantera Authors", `es.lproj` "Los creadores de Pantera".
  Checked in the strings files Finder reads, not in Finder's window.
- Still crediting Chromium in the copyright: bs, hr and sl (inflected, TASK-11), ne
  ("क्रोमियम लेखक", the name in Devanagari) and fr_CA ("Les auteurs de Chrome", upstream's
  translation). Added to TASK-11's notes.
