#!/usr/bin/env python3
"""Helpers for upgrading Pantera to a newer Chromium and newer dependencies.

    scripts/upgrade.py status            what is pinned, and what is available
    scripts/upgrade.py sync-upstream     apply ungoogled-chromium-macos's changes since the base
    scripts/upgrade.py adblock-pins      raise the uBlock Origin and Brave adblock-resources pins
    scripts/upgrade.py apply-patches     apply the patch series to build/src, one patch at a time

The whole procedure, with the steps that need judgment (fixing patches that no longer apply,
regenerating the Rust crates, building and checking the app), is in docs/upgrading.md.

Only the standard library is used. Network access: GitHub, crates.io and Google's version
history API for `status`; GitHub for `sync-upstream` and `adblock-pins`.
"""

from __future__ import annotations

import argparse
import configparser
import hashlib
import json
import re
import subprocess
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "build" / "src"
MAIN_REPO = ROOT / "ungoogled-chromium"
UPSTREAM_URL = "https://github.com/ungoogled-software/ungoogled-chromium-macos.git"
# Upstream files Pantera doesn't import (AGENTS.md, Upstream). Its bundle ID patch is replaced
# by patches/pantera/macos/set-bundle-identifier.patch.
NOT_IMPORTED = (
    ".github",
    "README.md",
    "announcements.md",
    "ungoogled-chromium",
    "patches/ungoogled-chromium/macos/set-bundle-identifier.patch",
)
UPSTREAM_ONLY_SERIES_LINES = ("ungoogled-chromium/macos/set-bundle-identifier.patch",)
ADBLOCK_INI = ROOT / "downloads-adblock.ini"
CRATES_PATCH = ROOT / "patches" / "pantera" / "rust" / "add-adblock-crates.patch"
AGENTS = ROOT / "AGENTS.md"


def run(*args: str, cwd: Path = ROOT, check: bool = True, **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(args, cwd=cwd, check=check, text=True, **kwargs)


def output(*args: str, cwd: Path = ROOT) -> str:
    return run(*args, cwd=cwd, stdout=subprocess.PIPE).stdout.strip()


def fetch(url: str) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": "upgrade-check"})
    with urllib.request.urlopen(request, timeout=60) as response:
        return response.read()


def fetch_json(url: str):
    return json.loads(fetch(url))


def version_key(version: str) -> tuple[int, ...]:
    return tuple(int(part) for part in re.findall(r"\d+", version))


# --- what is pinned -------------------------------------------------------------------------


def upstream_base() -> str:
    match = re.search(r"Base: ungoogled-chromium-macos `([0-9a-f]{40})`", AGENTS.read_text())
    if not match:
        sys.exit("AGENTS.md no longer records the upstream base commit")
    return match.group(1)


def adblock_pins() -> configparser.ConfigParser:
    pins = configparser.ConfigParser(interpolation=None)
    pins.read(ADBLOCK_INI)
    return pins


def adblock_crate_version() -> str:
    match = re.search(r'^\+adblock = \{ version = "([^"]+)"', CRATES_PATCH.read_text(), re.M)
    return match.group(1) if match else "unknown"


# --- status ---------------------------------------------------------------------------------


def latest_stable_chromium() -> list[tuple[str, float]]:
    """Mac stable releases being served now, as (version, share of users)."""
    data = fetch_json(
        "https://versionhistory.googleapis.com/v1/chrome/platforms/mac/channels/stable/"
        "versions/all/releases?filter=endtime=none&order_by=version%20desc"
    )
    # Releases belong to rollout groups; the newest group is the rollout in progress.
    group = max(release.get("fractionGroup", 0) for release in data["releases"])
    return [
        (release["version"], release["fraction"])
        for release in data["releases"]
        if release.get("fractionGroup", 0) == group and release.get("fraction")
    ]


def remote_tags(url: str) -> list[str]:
    lines = output("git", "ls-remote", "--tags", "--refs", url).splitlines()
    tags = [line.split("refs/tags/")[1] for line in lines]
    return sorted((tag for tag in tags if re.match(r"\d+\.", tag)), key=version_key)


def cmd_status(_args) -> None:
    chromium = (MAIN_REPO / "chromium_version.txt").read_text().strip()
    pins = adblock_pins()
    print("Pinned")
    print(f"  Chromium                      {chromium}")
    print(f"  ungoogled-chromium            {output('git', 'rev-parse', 'HEAD', cwd=MAIN_REPO)[:10]}")
    print(f"  ungoogled-chromium-macos base {upstream_base()[:10]}")
    print(f"  uBlock Origin                 {pins['ublock']['version']}")
    print(f"  Brave adblock-resources       {pins['brave-adblock-resources']['version'][:10]}")
    print(f"  adblock-rust                  {adblock_crate_version()}")
    print(f"  Pantera                       {(ROOT / 'VERSION').read_text().strip()}")

    print("\nAvailable")
    for version, fraction in latest_stable_chromium():
        print(f"  Chrome stable on macOS        {version}  ({fraction:.1%} of users)")
    print(f"  ungoogled-chromium            {remote_tags(output('git', 'remote', 'get-url', 'origin', cwd=MAIN_REPO))[-1]}")
    macos_head = output("git", "ls-remote", UPSTREAM_URL, "refs/heads/master").split()[0]
    print(f"  ungoogled-chromium-macos      {macos_head[:10]}  (latest tag {remote_tags(UPSTREAM_URL)[-1]})")
    print(f"  uBlock Origin                 {latest_ublock()}")
    print(f"  Brave adblock-resources       {latest_adblock_resources()[:10]}")
    crate = fetch_json("https://crates.io/api/v1/crates/adblock")["crate"]
    print(f"  adblock-rust                  {crate['max_stable_version']}")
    print(
        "\nPantera follows ungoogled-chromium-macos (DECISION-1): the Chromium to move to is the"
        "\none its master branch is on, even when Chrome stable or ungoogled-chromium is newer."
    )


# --- sync-upstream --------------------------------------------------------------------------


def cmd_sync_upstream(args) -> None:
    if output("git", "status", "--porcelain", "--untracked-files=no", "--", ".", ":!AGENTS.md", ":!devbacklog"):
        sys.exit("Commit or stash your changes first: the sync is committed on its own.")
    remotes = output("git", "remote").split()
    if "upstream" not in remotes:
        run("git", "remote", "add", "upstream", UPSTREAM_URL)
    # Upstream names its tags like Pantera names its releases (<chromium>-<revision>), so its
    # tags must stay out of this repository.
    run("git", "config", "remote.upstream.tagOpt", "--no-tags")
    run("git", "fetch", "--no-tags", "upstream")
    base = upstream_base()
    target = output("git", "rev-parse", f"{args.ref}^{{commit}}")
    if target == base:
        print(f"Already at {args.ref} ({base[:10]}).")
        return

    excludes = [f":!{path}" for path in NOT_IMPORTED]
    diff = run("git", "diff", "--binary", base, target, "--", ".", *excludes, stdout=subprocess.PIPE).stdout
    applied = subprocess.run(["git", "apply", "-3", "--whitespace=nowarn"], cwd=ROOT, input=diff, text=True)

    # patches/series conflicts every time: Pantera's patches follow the line upstream appends to.
    series = ROOT / "patches" / "series"
    text = series.read_text()
    if "<<<<<<<" in text:
        ours, theirs = re.search(r"<<<<<<< ours\n(.*?)=======\n(.*?)>>>>>>> theirs\n", text, re.S).groups()
        theirs = "".join(
            line for line in theirs.splitlines(keepends=True) if line.strip() not in UPSTREAM_ONLY_SERIES_LINES
        )
        series.write_text(re.sub(r"<<<<<<< ours\n.*?>>>>>>> theirs\n", lambda _: theirs + ours, text, flags=re.S))
        run("git", "add", "patches/series")

    submodule = output("git", "ls-tree", target, "ungoogled-chromium").split()[2]
    run("git", "fetch", "--tags", "origin", cwd=MAIN_REPO)
    run("git", "checkout", "-q", submodule, cwd=MAIN_REPO)
    run("git", "add", "ungoogled-chromium")

    chromium = (MAIN_REPO / "chromium_version.txt").read_text().strip()
    conflicts = output("git", "diff", "--name-only", "--diff-filter=U")
    print(f"\nApplied {base[:10]}..{target[:10]} (Chromium {chromium}), submodule at {submodule[:10]}.")
    if applied.returncode != 0 or conflicts:
        print("Resolve what git apply reported above, then:")
        if conflicts:
            print("Conflicts:\n  " + conflicts.replace("\n", "\n  "))
    print("Next:")
    print("  1. Read `git diff --cached` (build.sh, flags.macos.gn, patches/series, downloads-*.ini).")
    print(f'  2. git commit -m "chore: sync ungoogled-chromium-macos to {target[:7]}"')
    print(f"  3. Record the new base in AGENTS.md ({target}, Chromium {chromium}), README.md and")
    print("     the Makefile's SDK note, in a separate commit.")


# --- adblock-pins ---------------------------------------------------------------------------


def latest_ublock() -> str:
    """The newest stable uBlock Origin tag (betas end in bN or rcN)."""
    tags = [tag for tag in remote_tags("https://github.com/gorhill/uBlock.git") if re.fullmatch(r"[\d.]+", tag)]
    return tags[-1]


def latest_adblock_resources() -> str:
    return output("git", "ls-remote", "https://github.com/brave/adblock-resources.git", "HEAD").split()[0]


def set_pin(section: str, version: str) -> bool:
    """Rewrite one section's version and sha512 in downloads-adblock.ini, keeping its comments."""
    pins = adblock_pins()
    old = pins[section]["version"]
    if old == version:
        return False
    url = pins[section]["url"].replace("%(version)s", version)
    filename = pins[section]["download_filename"].replace("%(version)s", version)
    print(f"{section}: {old} -> {version}")
    archive = fetch(url)
    cache = ROOT / "build" / "download_cache"
    if cache.is_dir():
        (cache / filename).write_bytes(archive)
    digest = hashlib.sha512(archive).hexdigest()

    text = ADBLOCK_INI.read_text()
    head, body = text.split(f"[{section}]\n", 1)
    body = re.sub(r"^version = .*$", f"version = {version}", body, count=1, flags=re.M)
    body = re.sub(r"^sha512 = .*$", f"sha512 = {digest}", body, count=1, flags=re.M)
    ADBLOCK_INI.write_text(f"{head}[{section}]\n{body}")

    # The same versions are written down in the README files (downloads-adblock.ini's header).
    for path in (
        ROOT / "third_party" / "ublock" / "README.chromium",
        ROOT / "third_party" / "brave_adblock_resources" / "README.chromium",
        ROOT / "pantera" / "components" / "adblock" / "resources" / "README.txt",
    ):
        path.write_text(path.read_text().replace(old, version))
    return True


def cmd_adblock_pins(args) -> None:
    changed = set_pin("ublock", args.ublock or latest_ublock())
    changed |= set_pin("brave-adblock-resources", args.brave or latest_adblock_resources())
    if not changed:
        print("Both pins are already the latest.")
        return
    print(
        "\nUpdated downloads-adblock.ini and the README files. Check that the generator still"
        "\nunderstands both sources by building //pantera/components/adblock/resources, and read"
        "\nlist_catalog.json's diff for new or dropped default lists (DECISION-15)."
    )


# --- apply-patches --------------------------------------------------------------------------


def series() -> list[str]:
    lines = (ROOT / "patches" / "series").read_text().splitlines()
    return [line.strip() for line in lines if line.strip() and not line.startswith("#")]


def cmd_apply_patches(args) -> None:
    """Apply Pantera's series to build/src, stopping at the first patch that fails.

    Run it on a source tree that already has ungoogled-chromium's own patches (build.sh applies
    both sets together and stops at the first failure without saying what else is broken).
    """
    patches = series()
    if args.only_pantera:
        patches = [patch for patch in patches if patch.startswith("pantera/")]
    if args.start:
        matches = [index for index, patch in enumerate(patches) if args.start in patch]
        if len(matches) != 1:
            sys.exit(f"--from {args.start} matches {len(matches)} patches")
        patches = patches[matches[0] :]
    if args.only:
        patches = [patch for patch in series() if args.only in patch]
        if len(patches) != 1:
            sys.exit(f"--only {args.only} matches {len(patches)} patches")
    for skipped in args.skip:
        if not any(skipped in patch for patch in patches):
            sys.exit(f"--skip {skipped} matches no patch")
        patches = [patch for patch in patches if skipped not in patch]

    for index, name in enumerate(patches):
        path = ROOT / "patches" / name
        command = ["patch", "-p1", "--no-backup-if-mismatch", "-f", "-i", str(path)]
        dry = subprocess.run([*command, "--dry-run"], cwd=SRC, text=True, capture_output=True)
        if dry.returncode != 0:
            print(f"FAILED  {name}\n")
            if "Reversed (or previously applied)" in dry.stdout:
                print("It looks applied already. To apply a single patch, use --only.")
            print("\n".join(line for line in dry.stdout.splitlines() if "FAILED" in line or "can't find" in line))
            print(f"\n{index} applied before it, {len(patches) - index} left. Fix the patch, then:")
            print(f"  scripts/upgrade.py apply-patches --from {Path(name).stem}")
            sys.exit(1)
        fuzz = [line for line in dry.stdout.splitlines() if "fuzz" in line or "offset" in line]
        subprocess.run(command, cwd=SRC, check=True, stdout=subprocess.DEVNULL)
        note = f"  ({sum('fuzz' in line for line in fuzz)} fuzzy hunks)" if any("fuzz" in line for line in fuzz) else ""
        print(f"ok      {name}{note}")
    print(f"\nAll {len(patches)} patches applied.")


# --- main -----------------------------------------------------------------------------------


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)

    commands.add_parser("status", help="pinned and available versions").set_defaults(run=cmd_status)

    sync = commands.add_parser("sync-upstream", help="apply upstream's changes since the recorded base")
    sync.add_argument("ref", nargs="?", default="upstream/master", help="upstream commit or branch (default: upstream/master)")
    sync.set_defaults(run=cmd_sync_upstream)

    pins = commands.add_parser("adblock-pins", help="raise the ad-block data pins to the latest releases")
    pins.add_argument("--ublock", help="uBlock Origin tag (default: latest stable)")
    pins.add_argument("--brave", help="brave/adblock-resources commit (default: HEAD)")
    pins.set_defaults(run=cmd_adblock_pins)

    apply = commands.add_parser("apply-patches", help="apply the patch series to build/src one by one")
    apply.add_argument("--from", dest="start", help="start at the patch whose path contains this")
    apply.add_argument("--only-pantera", action="store_true", help="skip upstream's macOS patches")
    apply.add_argument("--skip", action="append", default=[], help="leave out patches whose path contains this")
    apply.add_argument("--only", help="apply just the patch whose path contains this")
    apply.set_defaults(run=cmd_apply_patches)

    args = parser.parse_args()
    args.run(args)


if __name__ == "__main__":
    main()
