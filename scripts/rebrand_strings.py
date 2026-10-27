#!/usr/bin/env python3
"""Renames Chromium to Pantera in Chromium's branded UI strings and their translations.

Run it on a patched Chromium source tree; build.sh does this after applying patches:

    python3 scripts/rebrand_strings.py build/src

Translations (.xtb) are keyed by a fingerprint of the English message, so changing the
English text alone would silently drop every translation. The script computes each
message's fingerprint before and after the rename with the source tree's own grit, and
rewrites the translation keys to match.

"Chromium" stays where it names the open source project rather than the browser:
"The Chromium Authors", the linked "Chromium" in "... made possible by the Chromium open
source project", and Chromium OS. Inflected forms in other languages (e.g. Finnish
"Chromiumin") are left as they are.
"""

import argparse
import re
import sys
from pathlib import Path

BRAND = "Pantera"

# The string catalogs that carry the product name, relative to the source root.
GRD_FILES = (
    "chrome/app/chromium_strings.grd",
    "components/components_chromium_strings.grd",
)

# "Chromium", except the cases above that name the project. Particles attached in other
# scripts ("Chromium에") and noun-class prefixes ("kuChromium" in Zulu) read correctly with
# the new name, so only a following Latin letter (an inflection, or "ChromiumOS") stops it.
LATIN = r"A-Za-z0-9_À-ɏ"
CHROMIUM = re.compile(
    rf'(?<![0-9_.])Chromium(?![{LATIN}]| Authors| OS\b|\s*<ph name="END_LINK_CHROMIUM")')
MESSAGE = re.compile(r"(<message\b[^>]*>)(.*?)(</message>)", re.DOTALL)
TRANSLATION = re.compile(r'<translation id="(\d+)"([^>]*)>(.*?)</translation>', re.DOTALL)
PART = re.compile(r'<part file="([^"]+)"')
XTB = re.compile(r'<file path="([^"]+\.xtb)"')


def rebrand(text):
    return CHROMIUM.sub(BRAND, text)


def message_ids(grd):
    """Returns the translation ID of every message in a .grd and its parts, in document order."""
    from grit import grd_reader
    from grit.node import message

    root = grd_reader.Parse(str(grd), skip_validation_checks=True)
    return [node.GetCliques()[0].GetId() for node in root.Preorder()
            if isinstance(node, message.MessageNode)]


def rebrand_messages(path):
    """Rebrands the text of every <message> in a .grd or .grdp file. Returns the change count."""
    text = path.read_text(encoding="utf-8")
    count = 0

    def replace(match):
        nonlocal count
        body, n = CHROMIUM.subn(BRAND, match.group(2))
        count += n
        return match.group(1) + body + match.group(3)

    path.write_text(MESSAGE.sub(replace, text), encoding="utf-8")
    return count


def rebrand_translations(path, new_ids):
    """Rekeys and rebrands the translations in an .xtb file. Returns the rekeyed count."""
    text = path.read_text(encoding="utf-8")
    rekeyed = 0

    def replace(match):
        nonlocal rekeyed
        old_id, attrs, body = match.groups()
        new_id = new_ids.get(old_id, old_id)
        rekeyed += new_id != old_id
        return f'<translation id="{new_id}"{attrs}>{rebrand(body)}</translation>'

    path.write_text(TRANSLATION.sub(replace, text), encoding="utf-8")
    return rekeyed


def rebrand_grd(grd):
    sources = [grd] + [grd.parent / part for part in PART.findall(grd.read_text(encoding="utf-8"))]
    before = message_ids(grd)
    renamed = sum(rebrand_messages(source) for source in sources)
    after = message_ids(grd)
    if len(before) != len(after):
        sys.exit(f"{grd}: message count changed while rebranding ({len(before)} -> {len(after)})")

    new_ids = {}
    for old_id, new_id in zip(before, after):
        if new_ids.setdefault(old_id, new_id) != new_id:
            sys.exit(f"{grd}: translation {old_id} maps to two different messages")

    xtbs = [grd.parent / path for path in XTB.findall(grd.read_text(encoding="utf-8"))]
    rekeyed = sum(rebrand_translations(xtb, new_ids) for xtb in xtbs)
    print(f"{grd}: renamed {renamed} mentions, rekeyed {rekeyed} translations in {len(xtbs)} languages")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("src", type=Path, help="Chromium source root")
    src = parser.parse_args().src.resolve()
    sys.path.insert(0, str(src / "tools" / "grit"))
    for grd in GRD_FILES:
        rebrand_grd(src / grd)


if __name__ == "__main__":
    main()
