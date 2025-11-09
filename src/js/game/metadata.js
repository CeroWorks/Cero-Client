const fs = require("fs");

const home = process.env.HOME || process.env.USERPROFILE;
const game_directory = `${home}/.minecraft`;

async function getMetadata(path) {
  console.log("[INFO] Reading metadata...");
  const manifest = JSON.parse(
    fs.readFileSync(`${path}/version_manifest.json`)
  );
  for (const v of manifest.versions) {
    // console.log(`[INFO] Registering version : ${v.id} (${v.type})`);
  }
}

module.exports = {
  getMetadata
};