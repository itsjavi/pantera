---
id: TASK-18
title: Rename Chromium in the remaining UI strings
priority: medium
labels: [phase-0, branding, l10n]
depends_on: []
created: 2026-10-27
---

## Goal

The browser never calls itself Chromium in its UI. `scripts/rebrand_strings.py` renames only the
two branded catalogs (`chromium_strings.grd`, `components_chromium_strings.grd`), but the first
build (TASK-4) still has 52 English strings that name the browser "Chromium", hard-coded in
other catalogs.

## Acceptance criteria

- [ ] Decide which catalogs and messages to rename (open question below)
- [ ] The English locale pack has no "Chromium" naming the browser (the credit, "Chromium OS"
      and the linked project name stay, per DECISION-3)
- [ ] Translations keep matching after the rename

## Notes

Visible examples from the English `locale.pak` (IDs from `generated_resources.grd`,
`components_strings.grd` and others):

- `IDS_NEW_TAB_OTR_NOT_SAVED`: "Chromium won't save: …" on every Incognito new tab page
- `IDS_SAD_TAB_RELOAD_RESTART_BROWSER`: "Restart Chromium"
- `IDS_PASSWORD_MANAGER_RELAUNCH_CHROME_BUBBLE_*_NON_BRANDED`: "Relaunch Chromium", "allow
  Keychain access"
- Extension warnings: "… may change the way Chromium works", "Remove from Chromium"
  (resource IDs 45980–45984, likely from ungoogled-chromium's own strings)
- Autofill and password manager prompts, SSL interstitials, search engine choice dialog

Many of these are Google-service features that ungoogled-chromium disables, so the visible set
is smaller than 52. To list them, read the pack with grit's `data_pack.ReadDataPack` and the
`IDS_*` defines in `out/Default/gen/**/grit/*.h`.

Open question: rename these catalogs wholesale (more translations to rekey, and messages that
mean the project rather than the browser need excluding) or keep a list of message names.
