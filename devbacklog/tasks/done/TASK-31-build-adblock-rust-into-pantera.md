---
id: TASK-31
title: Build adblock-rust into Pantera
priority: high
labels: [phase-2, shields, build]
depends_on: []
created: 2026-10-28
completed: 2026-10-28
---

## Goal

Brave's adblock-rust engine compiles inside Pantera's Chromium build and C++ can call it, the
foundation for the rest of the shields port (TASK-32, TASK-8, TASK-33). Code lives in the
`//pantera` tree (DECISION-13).

## Acceptance criteria

- [x] adblock 0.13.3 and the crates it needs are vendored into Chromium's
      `third_party/rust/chromium_crates_io` with Chromium's own gnrt, committed unmodified, with
      the changes to Chromium's existing crate files as a patch
- [x] `build.sh` copies `pantera/` and `third_party/` into the source tree, and Chromium's build
      loads `//pantera`
- [x] The cxx bridge is ported from brave-core into `pantera/components/adblock/`, keeping its
      MPL-2.0 headers, with public suffix lookups answered by `net::registry_controlled_domains`
- [x] A unit test builds and passes in `build/src`: it parses rules, blocks and allows requests,
      returns cosmetic resources and round-trips a serialized engine
- [x] Every new crate has a `README.chromium` with its license (`chrome://credits` lists them
      once Chrome links the engine, TASK-32)
- [x] `make check` passes and AGENTS.md describes the new layout

## Notes

- Research (2026-10-28): adblock 0.13.3 with brave-core's features (`default-features = false`,
  `full-regex-handling`, `debug-info`, `css-validation`, `single-thread`) needs 31 crates Chromium
  156 doesn't have, about 5.9 MB. None conflicts with Chromium's versions.
- Brave replaces the `url` and `idna` crates with shims over Chromium's URL code. Pantera vendors
  them as they are; they cost about 1.5 MB of source.
- brave-core's bridge: `components/brave_shields/core/common/adblock/rs/` and `.../resolver/`
  at brave/brave-core@2f490708. Imported unmodified first; Pantera's version drops the iOS
  content-blocking conversion, renames `BraveCoreResourceStorage` to `ResourceStorage` and
  replaces `thiserror` (Chromium only lets first-party code use crates listed in its
  `Cargo.toml`) with hand-written `Display`/`From` impls.

### What the crates needed (2026-10-28)

All in `gnrt_config.toml`, inside `patches/pantera/rust/add-adblock-crates.patch`, regenerated
with `scripts/update-rust-crates.sh` (`docs/rust-crates.md`):

- flatbuffers, seahash and selectors are published without a license file: gnrt patches in
  `chromium_crates_io/patches/` add them from upstream, as Chromium does for siphasher.
- regex and siphasher were test-only; aho-corasick had `allow_unsafe = false` although it uses
  unsafe code (it was only reached from tests before).
- rand_core 0.6 (phf's code generator) and thiserror 1 share their names with Chromium's
  rand_core 0.9 and thiserror 2, so their entries are split per version.
- flatbuffers' and thiserror 1's build scripts only probe rustc and are removed; selectors'
  generates a source file; derive_more-impl reads its docs with `include_str!`.
- `gnrt gen` needs gn on PATH (`build/src/out/Default/gn`), or it leaves every BUILD.gn
  unformatted.

### Checked (2026-10-28)

- `build/src` got the series' two new patches and the copied trees by hand, then
  `ninja -C out/Default pantera_unittests`: all 10 `AdblockRustTest` tests pass (blocking,
  exceptions, `$domain` on subdomains with Chromium's public suffix list, `$redirect` with a
  resource, `$removeparam`, `$csp`, cosmetic and generic class selectors, list metadata,
  serialization round trip).
- `scripts/update-rust-crates.sh prepare` then `update`, from a fresh copy of Chromium's files,
  reproduced `third_party/` and the crates patch byte for byte.
- `ninja -C out/Default chrome` rebuilt Chrome with the crate changes (serde, bitflags and
  stable_deref_trait gain features, so Chrome's Rust code rebuilds). An ad-hoc signed copy of the
  app ran headless and rendered a page. The app in `out/Default` still has the release's
  Developer ID signature, which the relinked framework no longer matches: re-sign before
  launching it.
- Not run: `build.sh` from scratch. The new patches were applied on top of the already patched
  tree, in series order, so a fresh run is expected to apply them the same way.
