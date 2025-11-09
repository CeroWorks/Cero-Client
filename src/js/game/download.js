const fs = require("fs");
const fetch = (...args) => import('node-fetch').then(({default: fetch}) => fetch(...args));
const path = require("path");


async function download(url, out) {
  console.log(`[DOWNLOAD] ${url} -> ${out}`);
  const res = await fetch(url);
  if (!res.ok) throw new Error(`Download failed: ${url}`);
  await fs.promises.mkdir(path.dirname(out), { recursive: true });
  const fileStream = fs.createWriteStream(out);
  await new Promise((resolve, reject) => {
    res.body.pipe(fileStream);
    res.body.on("error", reject);
    fileStream.on("finish", resolve);
  });
  console.log(`[DONE] ${out}`);
}


module.exports = {
  download
};