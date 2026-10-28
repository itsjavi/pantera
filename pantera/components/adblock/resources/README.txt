Pantera's ad blocker data

list_catalog.json
  Brave's filter list catalog, from brave/adblock-resources at commit
  6da9b247c3368baa8b4f24cd02046e18702a6527, under the Mozilla Public License 2.0
  (LICENSE-Brave.txt). Pantera downloads the lists it names from their
  maintainers.

resources.json
  The scriptlets and redirect resources that filter rules use, generated from
  uBlock Origin 1.75.0 (https://github.com/gorhill/uBlock/tree/1.75.0) and the
  same brave/adblock-resources commit. uBlock Origin's parts are under the GNU
  General Public License version 3 (LICENSE-uBlock-Origin.txt), and so is the
  file as a whole; Brave's parts are under the Mozilla Public License 2.0. Their
  source is those releases. Pantera's generator is
  pantera/components/adblock/resources/build_adblock_resources.mjs in Pantera's
  source.

The rest of Pantera is under its own licenses; see chrome://credits.
