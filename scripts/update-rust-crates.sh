#!/usr/bin/env bash
# Regenerates the Rust crates Pantera adds to Chromium (DECISION-13) with Chromium's own gnrt.
#
#   scripts/update-rust-crates.sh prepare   copy Chromium's crate folders from build/src into
#                                           .local/chromium and apply Pantera's crates on top
#   (edit .local/chromium/third_party/rust/chromium_crates_io/{Cargo.toml,gnrt_config.toml})
#   scripts/update-rust-crates.sh update    run gnrt vendor and gen there, then write
#                                           third_party/ and patches/pantera/rust/add-adblock-crates.patch
#
# Needs a Chromium checkout in build/src (build.sh makes one) with gn built in out/Default, and
# network access to crates.io. Details in docs/rust-crates.md.

set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
src="$root/build/src"
work="$root/.local/chromium"
patch_file="$root/patches/pantera/rust/add-adblock-crates.patch"

prepare() {
  rm -rf "$work"
  mkdir -p "$work/build"
  # Chromium's files as released, before ungoogled-chromium's domain substitution rewrote them.
  git -C "$src" archive HEAD third_party/rust tools/crates build/rust | tar -x -C "$work"
  # run_cargo.py imports these helpers even on macOS.
  cp "$src/build/vs_toolchain.py" "$src/build/gn_helpers.py" "$work/build/"
  ln -s "$src/third_party/rust-toolchain" "$work/third_party/rust-toolchain"
  git -C "$work" init -q
  git -C "$work" add -A -f .
  git -C "$work" -c user.name=pantera -c user.email=pantera@localhost commit -q -m chromium
  # Pantera's current crates on top, as working tree changes.
  sed '/^diff --git/,$!d' "$patch_file" | patch -s -p1 -d "$work"
  ditto "$root/third_party" "$work/third_party"
  echo "Edit $work/third_party/rust/chromium_crates_io, then run: $0 update"
}

update() {
  cd "$work"
  mkdir -p "$root/.local/bin"
  ln -sf "$src/out/Default/gn" "$root/.local/bin/gn"
  python3 tools/crates/run_gnrt.py vendor
  # gnrt formats each BUILD.gn with the gn on PATH.
  PATH="$root/.local/bin:$PATH" python3 tools/crates/run_gnrt.py gen

  # Chromium's own files that changed go into the patch, below its header.
  local header
  header="$(sed '/^diff --git/,$d' "$patch_file")"
  local changed=()
  while IFS= read -r f; do changed+=("$f"); done < <(git diff --name-only -- third_party/rust)
  { printf '%s\n\n' "$header"
    git diff --src-prefix=a/ --dst-prefix=b/ -- "${changed[@]}" | grep -v '^index '
  } > "$patch_file"

  # New files, including those the crates' own .gitignore files hide, go into third_party/.
  rm -rf "$root/third_party"
  git ls-files --others -z -- third_party/rust ':!third_party/rust-toolchain' |
    rsync -a --from0 --files-from=- ./ "$root/"
  echo "Updated $patch_file and $root/third_party"
}

case "${1:-}" in
  prepare) prepare ;;
  update) update ;;
  *) echo "Usage: $0 prepare|update" >&2; exit 1 ;;
esac
