# Roadmap

Pantera is a privacy-first Chromium browser for macOS: stock Chromium without Google services,
with Brave-style protections and nothing extra. Each phase below is a goal; its tasks carry the
phase's label. See `devbacklog/scripts/backlog.py list --label phase-N`.

## Phase 0: Foundation (`phase-0`), in progress

A Pantera build you can install and use every day.

- ungoogled-chromium-macos as the base, branded as Pantera Browser: name, icon, profile folder,
  keychain item and UI strings.
- Extensions install from the Chrome Web Store, like in Brave.
- A local build that works, then a Developer ID signed and notarized release.

## Phase 1: Updates and controls (`phase-1`)

Pantera keeps itself up to date and puts privacy controls one click away.

- Sparkle auto-updates for Pantera itself.
- A per-site panel, in the spirit of Brave Shields, for the protections Chromium already has:
  scripts, third-party cookies and HTTPS upgrades.

## Phase 2: Content blocking (`phase-2`)

Built-in ad and tracker blocking that doesn't depend on extensions.

- Brave's adblock-rust engine for request blocking, element hiding and scriptlets.
- A blocked counter in the panel and a filter lists page.
- Filter lists downloaded straight from their maintainers, never through a Pantera server.

## Phase 3: More protections (`phase-3`)

- "Forget me when I close this site".
- Fingerprinting protections.

## Always

- No telemetry, usage statistics or crash reports. Anything that contacts a server by default is
  documented and can be turned off.
- Keep the patch set small so Chromium security fixes ship quickly.

## Not planned

Crypto wallets, rewards, VPN, AI assistants and news feeds.
