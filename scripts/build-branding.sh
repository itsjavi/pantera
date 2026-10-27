#!/usr/bin/env bash
# Regenerates Pantera's branding assets: the icon layers, light and dark icon renders, and the
# files build.sh copies over the Chromium source (Assets.car, app.icns, product logos).
# Run it after changing make-icon.swift or branding/AppIcon.icon, then commit the results.
# Needs Xcode 26+ (actool and Icon Composer's ictool).

set -euo pipefail

_root_dir="$(cd "$(dirname "$0")/.." && pwd)"
_branding="$_root_dir/branding"
_theme="$_branding/overlay/chrome/app/theme"
_ictool="$(xcode-select -p)/../Applications/Icon Composer.app/Contents/Executables/ictool"

swift "$_root_dir/scripts/make-icon.swift" "$_branding/AppIcon.icon" \
  "$_branding/overlay/chrome/browser/resources/new_tab_page_third_party"

# Compile the Icon Composer icon into a Liquid Glass asset catalog plus a flat .icns
# fallback. Chromium's Info.plist already names them: CFBundleIconName=AppIcon and
# CFBundleIconFile=app.icns. The deployment target matches Chromium's mac_deployment_target.
_tmp="$(mktemp -d)"
trap 'rm -rf "$_tmp"' EXIT
xcrun actool "$_branding/AppIcon.icon" --compile "$_tmp" --app-icon AppIcon \
  --platform macosx --target-device mac --minimum-deployment-target 13.0 \
  --output-partial-info-plist "$_tmp/icon.plist" --errors --warnings >/dev/null
mkdir -p "$_theme/chromium/mac"
cp "$_tmp/Assets.car" "$_theme/chromium/mac/Assets.car"
cp "$_tmp/AppIcon.icns" "$_theme/chromium/mac/app.icns"

# Rendered icons, as macOS draws them in light and dark mode (README, website, docs).
for _rendition in Default Dark; do
  _name="icon-light.png"
  [[ "$_rendition" == Dark ]] && _name="icon-dark.png"
  "$_ictool" "$_branding/AppIcon.icon" --export-image --output-file "$_branding/$_name" \
    --platform macOS --rendition "$_rendition" --width 512 --height 512 --scale 1 >/dev/null
done

# Product logos that Chromium's UI shows (About page, menus, task manager).
logo() {
  mkdir -p "$(dirname "$2")"
  sips -z "$1" "$1" "$_branding/icon-light.png" --out "$2" >/dev/null
}
for size in 16 24 48 64 128 256; do
  logo "$size" "$_theme/chromium/product_logo_$size.png"
done
for size in 16 32; do
  logo "$size" "$_theme/default_100_percent/chromium/product_logo_$size.png"
  logo "$((size * 2))" "$_theme/default_200_percent/chromium/product_logo_$size.png"
done

echo "Branding assets written to $_branding"
