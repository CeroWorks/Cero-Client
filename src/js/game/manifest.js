const fs = require("fs");
const path = require("path");
const { download } = require('./download');

async function getManifest(rootPath) {
  if (!rootPath) throw new Error("[ERROR] getManifest: rootPath is undefined");

  await fs.promises.mkdir(rootPath, { recursive: true });

  const manifestPath = path.join(rootPath, "version_manifest.json");

  console.log("[INFO] Getting version manifest... ");
  await download(
    "https://launchermeta.mojang.com/mc/game/version_manifest.json",
    manifestPath
  );
  console.log("[INFO] Manifest downloaded:", manifestPath);
}

module.exports = { getManifest };
