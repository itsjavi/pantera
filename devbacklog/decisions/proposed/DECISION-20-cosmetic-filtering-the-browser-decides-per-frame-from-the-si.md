---
id: DECISION-20
title: Cosmetic filtering: the browser decides per frame from the site's shields settings
date: 2026-10-28
tasks: [TASK-33]
---

## Context

brave-core's renderer decides whether a frame gets cosmetic filtering, and how much, from the
shields settings Brave sends to renderers through patches to Blink's content settings client.
Pantera's shields settings (DECISION-17) are content settings the browser reads, which Chromium
doesn't send to renderers.

## Decision

- The renderer asks the browser for a frame's cosmetic resources at document start, as in
  brave-core, passing the frame's URL, its top-level site and whether it's cross-origin to it.
- The browser checks the top-level site's shields settings and answers with nothing when shields
  are down, or with the resources and whether to hide first-party content (aggressive mode,
  cross-origin frames, youtube.com), as brave-core's renderer would decide.
- The browser answers on the ad-block engines' sequence, as brave-core does, and reads the
  content settings there, which Chromium allows from any thread.

## Consequences

- No Blink patch to carry Pantera's settings into renderers.
- The renderer's own idea of the top-level site is trusted for its own cosmetic filtering only;
  a compromised renderer can already skip it.
- Requests and cosmetic resources queue behind engine rebuilds on the same sequence.
