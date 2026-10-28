// Builds the resources.json that adblock-rust uses for +js() scriptlets and $redirect rules
// (DECISION-14), the way Brave's packager does (brave/brave-core-crx-packager,
// lib/adBlockRustUtils.js). It combines, in this order:
//
// - uBlock Origin's redirect resources: the files in src/web_accessible_resources/ that
//   src/js/redirect-resources.js names,
// - Brave's own resources, from brave/adblock-resources' dist/resources.json,
// - uBlock Origin's scriptlets from src/js/resources/scriptlets.js, as the source of each function.
//
// Unlike Brave's packager, scriptlets that uBlock Origin marks requiresTrust get permission bit 0,
// so only the lists Brave's catalog trusts with it, and the user's own filters, can run them.
//
// Usage: node scripts/build-adblock-resources.mjs <ublock-dir> <brave-adblock-resources-dir> <out>
// The build runs it on the sources downloads-adblock.ini pins. The result is GPL-3.0, like
// uBlock Origin, and ships as its own file.

import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ublockDir, braveDir, outPath] = process.argv.slice(2);
if (!outPath) {
  console.error('Usage: build-adblock-resources.mjs <ublock-dir> <brave-adblock-resources-dir> <out>');
  process.exit(1);
}

// adblock-rust's MimeType::from_extension.
const MIME_TYPES = new Map([
  ['css', 'text/css'],
  ['gif', 'image/gif'],
  ['html', 'text/html'],
  ['js', 'application/javascript'],
  ['json', 'application/json'],
  ['mp3', 'audio/mp3'],
  ['mp4', 'video/mp4'],
  ['png', 'image/png'],
  ['txt', 'text/plain'],
  ['xml', 'text/xml'],
]);
const TEXT_TYPES = new Set(['application/javascript', 'text/html', 'text/plain']);

// Bit 0 is the permission Brave's catalog grants to its trusted lists.
const TRUSTED_SCRIPTLET_PERMISSION = 1;

const importModule = (file) => import(pathToFileURL(path.resolve(file)).href);

// adblock-rust's assemble_web_accessible_resources, which Brave runs through adblock-rs.
const redirectResources = async () => {
  const { default: resourceMap } =
      await importModule(path.join(ublockDir, 'src/js/redirect-resources.js'));
  const resources = [];
  for (const [name, properties] of resourceMap) {
    // Resources with parameters are handled by uBlock Origin itself, and adblock-rust skips them.
    if (properties.params !== undefined) {
      continue;
    }
    // Brave leaves this one out.
    if (name === 'google-ima-dai.js') {
      continue;
    }
    // Names without a known extension, such as "empty", are octet streams to adblock-rust.
    const dot = name.lastIndexOf('.');
    const mime = (dot >= 0 && MIME_TYPES.get(name.slice(dot + 1))) || 'application/octet-stream';
    let content = readFileSync(path.join(ublockDir, 'src/web_accessible_resources', name));
    if (TEXT_TYPES.has(mime)) {
      content = Buffer.from(content.toString('utf8').replaceAll('\r', ''), 'utf8');
    }
    const alias = properties.alias ?? [];
    resources.push({
      name,
      aliases: Array.isArray(alias) ? alias : [alias],
      kind: { mime },
      content: content.toString('base64'),
    });
  }
  return resources;
};

const scriptletResources = async () => {
  const { builtinScriptlets } =
      await importModule(path.join(ublockDir, 'src/js/resources/scriptlets.js'));
  const names = new Set(builtinScriptlets.map(scriptlet => scriptlet.name));
  return builtinScriptlets.map(scriptlet => {
    const dependencies = scriptlet.dependencies ?? [];
    for (const dependency of dependencies) {
      if (!names.has(dependency)) {
        throw new Error(`Scriptlet ${scriptlet.name} needs missing ${dependency}`);
      }
    }
    const resource = {
      name: scriptlet.name,
      aliases: scriptlet.aliases ?? [],
      kind: { mime: 'application/javascript' },
      content: Buffer.from(scriptlet.fn.toString()).toString('base64'),
      dependencies,
    };
    if (scriptlet.requiresTrust) {
      resource.permission = TRUSTED_SCRIPTLET_PERMISSION;
    }
    return resource;
  });
};

const braveResources =
    JSON.parse(readFileSync(path.join(braveDir, 'dist/resources.json'), 'utf8'));

const resources = [
  ...await redirectResources(),
  ...braveResources,
  ...await scriptletResources(),
];
writeFileSync(outPath, JSON.stringify(resources));
console.log(`Wrote ${resources.length} resources to ${outPath}`);
