# Upgrading Chromium and Pantera's dependencies

How to move Pantera to a newer Chromium and raise its own dependencies, from the version check to
a build ready to release. `scripts/upgrade.py` does the mechanical steps; the rest needs someone
reading diffs and build errors. Expect half a day, most of it the build.

## What gets upgraded

| Dependency | Pinned in | Raised with |
|---|---|---|
| Chromium, ungoogled-chromium and upstream's macOS patches | the `ungoogled-chromium` submodule, `patches/ungoogled-chromium/macos/`, the base commit in `AGENTS.md` | `scripts/upgrade.py sync-upstream` |
| LLVM, Rust, Node.js, Crubit | `downloads-*.ini` | the upstream sync |
| uBlock Origin's scriptlets, Brave's adblock-resources | `downloads-adblock.ini` and three README files | `scripts/upgrade.py adblock-pins` |
| adblock-rust and the crates it needs | `patches/pantera/rust/add-adblock-crates.patch`, `third_party/rust/` | `scripts/update-rust-crates.sh` ([rust-crates.md](rust-crates.md)) |

Code ported from brave-core (`pantera/components/`) is not a pinned dependency. Porting newer
brave-core changes is a task of its own.

## 1. Check what is available

```bash
scripts/upgrade.py status
```

Pantera follows ungoogled-chromium-macos (DECISION-1), so the Chromium to move to is the one its
`master` branch is on. It usually trails Chrome stable by a few days and can be a patch release
or, during a rollout, a major version behind. Don't move the submodule ahead of upstream macOS:
its patches are written for one Chromium version.

Create a backlog task for the upgrade and move it to in progress.

## 2. Sync upstream

```bash
scripts/upgrade.py sync-upstream
```

It applies upstream's changes since the base recorded in `AGENTS.md`, puts the patches upstream
added to `patches/series` before Pantera's, leaves out upstream's `set-bundle-identifier.patch`
(Pantera has its own) and moves the submodule. Then:

1. Read `git diff --cached`. Upstream's changes to `build.sh`, `flags.macos.gn`,
   `retrieve_and_unpack_resource.py` and `sign_and_package_app.sh` merge into Pantera's versions
   and are the likeliest to conflict.
2. If upstream changed its `set-bundle-identifier.patch`
   (`git diff <base> upstream/master -- patches/ungoogled-chromium/macos/set-bundle-identifier.patch`),
   make the same change in `patches/pantera/macos/set-bundle-identifier.patch`, in the fix-up
   commit.
3. Commit the sync on its own: `chore: sync ungoogled-chromium-macos to <sha>`.
4. In a second commit, record the new base and Chromium version in `AGENTS.md` (Upstream), and
   update the Chromium version and SDK notes in `README.md` and the `Makefile` if they changed.

## 3. Raise the ad-block data pins

```bash
scripts/upgrade.py adblock-pins
```

It moves `downloads-adblock.ini` to the latest stable uBlock Origin release and the head of
brave/adblock-resources, with new checksums, and updates the versions in the README files.

## 4. Get the new Chromium source

```bash
export PATH="$PWD/build/python/bin:$PATH" GIT_LFS_SKIP_SMUDGE=1
export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer
./retrieve_and_unpack_resource.py -g arm64
```

This resets `build/src` to the new Chromium tag and deletes everything untracked in it,
including `out/`. It downloads about 10 GB and takes 30 to 60 minutes.

## 5. Rebase Pantera's patches

Apply ungoogled-chromium's patches as `build.sh` does, then Pantera's series one patch at a time,
so a failure names the patch and the tree stays usable:

```bash
python3 ungoogled-chromium/utils/prune_binaries.py build/src ungoogled-chromium/pruning.list
python3 ungoogled-chromium/utils/patches.py apply build/src ungoogled-chromium/patches
scripts/upgrade.py apply-patches --skip add-adblock-crates
```

`patches/pantera/rust/add-adblock-crates.patch` is left out because it fails on every Chromium
update: it carries Chromium's lockfile. The next step regenerates it.

When a patch fails, the command stops and prints the rejected hunks. Fix the patch file against
the tree as it is at that point, then continue with
`scripts/upgrade.py apply-patches --skip add-adblock-crates --from <patch-name>`.

- To refresh a patch: apply what applies (`patch -p1 -d build/src < patches/...`), fix the `.rej`
  hunks by hand in the source and delete them, then write the patch again with
  `git -C build/src diff -- <its files>` under the patch's header comment, without the
  `diff --git` and `index` lines. Keep the header, and correct it if it's no longer true.
- A patch that applies with fuzz is worth refreshing too: fuzz is how a hunk lands in the wrong
  place a version later.
- If Chromium removed or rewrote what a patch changes, read the new code before porting the
  patch: the feature may need a different hook, not the old lines in a new place. If Chromium now
  does what the patch did, delete the patch and its line in `patches/series`.
- `git status` in `build/src` doesn't show files under `build/` when a global gitignore hides
  that name. Use `git status --ignored` or `git diff` on the path.

Finish with `make check`.

## 6. Regenerate the Rust crates

gnrt needs the Rust toolchain and a built `gn` in the tree:

```bash
./retrieve_and_unpack_resource.py -p arm64
(cd build/src && ./tools/gn/bootstrap/bootstrap.py -o out/Default/gn --skip-generate-buildfiles)
```

Then follow [rust-crates.md](rust-crates.md): `scripts/update-rust-crates.sh prepare`, set the
adblock version in `.local/chromium/third_party/rust/chromium_crates_io/Cargo.toml` to the latest
on crates.io, and run `scripts/update-rust-crates.sh update`. Read adblock-rust's `CHANGELOG.md`
between the two versions for API changes that reach `pantera/components/adblock/rs/`, and for
changes to the saved engine format (Pantera rebuilds its engines when a saved one doesn't load).

Apply the regenerated patch to the tree:

```bash
scripts/upgrade.py apply-patches --only add-adblock-crates
```

Commit the crate sources (`third_party/rust/chromium_crates_io/vendor/`) unmodified first, then
the patch and the generated `BUILD.gn` and `README.chromium` files. If the newest adblock-rust
needs a newer version of a crate Chromium already vendors (`serde_json`, `base64`), take the
newest adblock-rust that fits instead of replacing Chromium's crate, and note it in the task.

## 7. Build

The tree is now pruned and patched, so tell `build.sh` to keep it instead of cloning again:

```bash
export PATH="$PWD/build/python/bin:$PATH" GIT_LFS_SKIP_SMUDGE=1
export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer
MACOS_AD_HOC_SIGNING=1 ./build.sh -k -s /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk \
  > .local/build.log 2>&1
```

`-k` skips the clone, the pruning and the patches; everything after (domain substitution,
Pantera's sources, the ad-block data, the string rename, the toolchains, `gn gen`, `ninja` and
packaging) runs as usual. It can only run once on a tree, because domain substitution isn't
repeatable. The build takes several hours: run it in the background.

- A plain `make build` works too, but it resets the tree and downloads Chromium's dependencies
  again before patching.
- When the compile fails, fix the cause, then continue with
  `ninja -C build/src/out/Default chrome chromedriver` instead of running `build.sh` again, and
  finish with `CHROMIUM_SRC_DIR="$PWD/build/src" MACOS_AD_HOC_SIGNING=1 ./sign_and_package_app.sh`
  for the `.dmg`. For a failure in Pantera's code, fix it in `pantera/` and copy it over with
  `ditto pantera build/src/pantera`. For a failure in patched Chromium code, fix the file in
  `build/src` and refresh the patch from it.

If the Mac crashes or loses power during the build, don't continue from `out/Default`: files
written just before the crash can be incomplete, and ninja would take them as built. Start the
output folder again, keeping only what `build.sh` put there:

```bash
cd build/src
mkdir -p ../../.local/out-keep && cp out/Default/args.gn out/Default/gn ../../.local/out-keep/
rm -rf out/Default && mkdir -p out/Default/sdk
cp ../../.local/out-keep/args.gn ../../.local/out-keep/gn out/Default/
ln -s /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk out/Default/sdk/selected_macos_sdk
SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk ./out/Default/gn gen out/Default --fail-on-unused-args
SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk ninja -C out/Default chrome chromedriver
```

A kernel panic with "watchdog timeout" during a build means the machine ran out of memory. The
panic report in `/Library/Logs/DiagnosticReports/` lists every process and its memory
(`processByPid`); look for one build step running far more processes than ninja's job count
(TASK-38 has an example, fixed by `patches/pantera/gn/serialize-bindings-generation.patch`).

Also build and run the unit tests:

```bash
ninja -C build/src/out/Default pantera_unittests && build/src/out/Default/pantera_unittests
```

## 8. Check the app

Launch a copy of the app with a scratch profile (AGENTS.md, Validation) and check, in light and
dark mode:

- The About page shows the new Chromium version and Pantera's version.
- The New Tab page, the default theme and the toolbar look as before.
- The Shields panel opens, and a page with ads shows a blocked count.
- An extension installs from the Chrome Web Store.
- `chrome://settings` and `chrome://extensions` say Pantera, not Chromium.

The app follows the system's appearance. For light mode on a Mac set to dark, add
`-NSRequiresAquaSystemAppearance YES` after the URL on the command line (Chromium opens `yes` as
a second tab; ignore it). `--remote-debugging-port=<port>` lets a script open pages in the running
copy: `curl -X PUT "http://127.0.0.1:<port>/json/new?chrome://settings/help"`. Confirming the
extension's install prompt takes a click by hand.

## 9. Release

Raise `VERSION` (`make bump-patch` or `bump-minor`), reset `revision.txt` to 1 for a new Chromium
version, close the backlog task and commit. Then sign, notarize and publish as in
[releasing.md](releasing.md).
