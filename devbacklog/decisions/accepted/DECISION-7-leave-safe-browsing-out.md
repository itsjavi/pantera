---
id: DECISION-7
title: Leave Safe Browsing out
date: 2026-10-27
tasks: []
---

## Context

ungoogled-chromium builds with `safe_browsing_mode=0`, and two of its patches
(`inox-patchset/0001-fix-building-without-safebrowsing.patch` and
`ungoogled-chromium/fix-building-without-safebrowsing.patch`) delete the code. Brave keeps Safe
Browsing by proxying it through its own servers, which DECISION-6 rules out. A direct opt-in, with
a hint like DECISION-5's, would need `safe_browsing_mode=1`, a way for `build.sh` to skip those
two patches, Safe Browsing files kept out of domain substitution, build fixes in a configuration
ungoogled doesn't test, and a Google API key, either shared by every install or entered by each
user.

## Decision

Pantera ships without Safe Browsing, as ungoogled-chromium does. No opt-in.

## Consequences

- No phishing or malware warnings from Google's lists. On macOS, Gatekeeper and XProtect still
  check downloaded apps.
- The ungoogled Safe Browsing patches and flag stay as they are, which keeps upstream syncs
  simple.
- Revisit only if Safe Browsing becomes possible without an API key or the extra patching.
