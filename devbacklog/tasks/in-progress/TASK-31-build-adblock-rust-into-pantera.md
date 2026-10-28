---
id: TASK-31
title: Build adblock-rust into Pantera
priority: high
labels: [phase-2, shields, build]
depends_on: []
created: 2026-10-28
---

## Goal

Brave's adblock-rust engine compiles inside Pantera's Chromium build and C++ can call it, the
foundation for the rest of the shields port (TASK-32, TASK-8, TASK-33). Code lives in the
`//pantera` tree (DECISION-13).

## Acceptance criteria

- [ ] adblock 0.13.3 and the crates it needs are vendored into Chromium's
      `third_party/rust/chromium_crates_io` with Chromium's own gnrt, committed unmodified, with
      the changes to Chromium's existing crate files as a patch
- [ ] `build.sh` copies `pantera/` and `third_party/` into the source tree, and Chrome's build
      depends on `//pantera`
- [ ] The cxx bridge is ported from brave-core into `pantera/components/adblock/`, keeping its
      MPL-2.0 headers, with public suffix lookups answered by `net::registry_controlled_domains`
- [ ] A unit test builds and passes in `build/src`: it parses rules, blocks and allows requests,
      returns cosmetic resources and round-trips a serialized engine
- [ ] Every new crate has a `README.chromium` with its license, so `chrome://credits` lists it
- [ ] `make check` passes and AGENTS.md describes the new layout

## Notes

- Research (2026-10-28): adblock 0.13.3 with brave-core's features (`default-features = false`,
  `full-regex-handling`, `debug-info`, `css-validation`, `single-thread`) needs 31 crates Chromium
  156 doesn't have, about 5.9 MB. None conflicts with Chromium's versions. regex and siphasher are
  test-only in Chromium's `gnrt_config.toml` and have to become `safe`; bitflags gains `serde`,
  serde and serde_core gain `rc`, stable_deref_trait gains `std`. flatbuffers, seahash and
  selectors ship without a license file, and selectors needs its build script output declared.
- Brave replaces the `url` and `idna` crates with shims over Chromium's URL code. Pantera
  vendors them as they are unless that turns out to be a problem.
- brave-core's bridge: `components/brave_shields/core/common/adblock/rs/` (lib.rs, about 810
  lines of Rust) and `.../adblock/resolver/`.
