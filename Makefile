# Shortcuts for the commands in README.md and docs/releasing.md. The scripts hold the logic;
# these targets only give them the environment the build needs. Override any variable below in a
# git-ignored local.mk.

-include local.mk

# Chromium 156 needs a macOS 26 SDK (README, Building).
SDK ?= /Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk
DEVELOPER_DIR ?= /Applications/Xcode.app/Contents/Developer
# The first Developer ID Application identity in the keychain.
CERTIFICATE ?= $(shell security find-identity -v -p codesigning | sed -n 's/.*"\(Developer ID Application: .*\)"$$/\1/p' | head -n 1)
NOTARY_PROFILE ?= notary

VENV := build/python
VERSION = $(shell cat ungoogled-chromium/chromium_version.txt)-$(shell cat ungoogled-chromium/revision.txt).$(shell cat revision.txt)
APP := build/src/out/Default/Pantera Browser.app
DMG = build/pantera-browser_$(VERSION)_macos.dmg

export DEVELOPER_DIR
export PATH := $(CURDIR)/$(VENV)/bin:$(PATH)
# The LFS files are prebuilt binaries that the build removes anyway.
export GIT_LFS_SKIP_SMUDGE := 1

.PHONY: help setup build release check branding

help:
	@echo "make setup     install the build prerequisites and the Python venv"
	@echo "make build     build Pantera, ad-hoc signed (takes hours)"
	@echo "make release   sign the build with a Developer ID, notarize and check it"
	@echo "make check     check the scripts, the patch series and the backlog"
	@echo "make branding  regenerate the icon and branding assets"

setup:
	brew install python@3.13 go ninja coreutils readline node esbuild typescript
	xcodebuild -downloadComponent MetalToolchain
	if brew list binutils >/dev/null 2>&1; then brew unlink binutils; fi
	git submodule update --init --recursive
	$(MAKE) $(VENV)/bin/python3

$(VENV)/bin/python3:
	/opt/homebrew/opt/python@3.13/bin/python3.13 -m venv $(VENV)
	$(VENV)/bin/pip install PySocks httplib2

build: $(VENV)/bin/python3
	MACOS_AD_HOC_SIGNING=1 ./build.sh -s "$(SDK)"

release:
	MACOS_CERTIFICATE_NAME="$(CERTIFICATE)" PROD_MACOS_NOTARIZATION_PROFILE="$(NOTARY_PROFILE)" \
		./sign_and_package_app.sh "$(CURDIR)/$(DMG)"
	spctl -a -vvv -t open --context context:primary-signature "$(DMG)"
	spctl -a -vvv -t exec "$(APP)"
	syspolicy_check distribution "$(APP)"

check:
	for f in *.sh scripts/*.sh devutils/*.sh; do bash -n "$$f" || exit 1; done
	# Upstream's check takes every file in patches/ for a patch, Finder's too.
	find patches -name .DS_Store -delete
	devutils/check_patch_files.sh
	devbacklog/backlog.py check

branding:
	scripts/build-branding.sh
