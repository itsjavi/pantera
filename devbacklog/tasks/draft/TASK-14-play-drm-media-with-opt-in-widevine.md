---
id: TASK-14
title: Play DRM media with opt-in Widevine
priority: medium
labels: [phase-1, media, privacy]
depends_on: [TASK-4]
created: 2026-10-27
---

## Goal

DRM-protected sites play in Pantera once the user opts in, the way Brave does it: Spotify and
other sites that accept Widevine L3 first, and Netflix if the VMP licensing question below is
answered.

## Acceptance criteria

- [ ] To be defined. Likely, for the Spotify step: Widevine is off by default, a prompt on DRM
      sites and a setting turn it on, the CDM is downloaded from Google only after that, and the
      README documents the request and how to turn it off
- [ ] Decision recorded on whether to pursue Netflix (VMP signing) and how

## Notes

**Spotify step (Widevine L3).** Pantera inherits `enable_widevine=true` from
`ungoogled-chromium/flags.gn`, so the CDM host code is built in, but no CDM ever arrives. The
component updater's server stays disabled (see TASK-3), and domain substitution likely breaks the
Widevine download URL too (to confirm). Brave turns Widevine off by default, shows an infobar on
DRM sites plus a toggle in its settings, and fetches the CDM through Chromium's component updater
via its own proxy. Pantera would download it straight from Google, per the roadmap's
no-Pantera-server rule. The CDM can't be bundled, since redistributing it needs a license.
Possibly a narrow exception to the disabled component updater, similar to TASK-3's Web Store
carve-out.

**Netflix step (VMP).** On macOS and Windows, Netflix also needs Widevine's Verified Media Path:
`.sig` files for the browser's binaries, made with a key from a Widevine license that Google
grants. Brave has one and signs its releases. Without it, Netflix refuses to play even with the
CDM installed. Open decision for the user: apply to Google for a Widevine license (a legal and
business step, historically slow for small browsers) or accept no Netflix. If it's granted, the
signing step goes into the release scripts after TASK-5.

Unverified: whether a manually installed CDM loads in a signed and notarized Pantera build. Vivaldi
broke Netflix on macOS once by changing its signing.
