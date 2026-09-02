use crate::log::Log;
use std::fs;
use std::path::Path;

pub fn extract_zip(zip_path: &Path, dest: &Path, log: &Log) -> Result<(), String> {
    let file = fs::File::open(zip_path).map_err(|e| format!("open zip: {e}"))?;
    let mut archive = zip::ZipArchive::new(file).map_err(|e| format!("zip: {e}"))?;

    for i in 0..archive.len() {
        let mut entry = archive.by_index(i).map_err(|e| format!("zip entry: {e}"))?;
        let outpath = dest.join(entry.name());
        if entry.is_dir() {
            fs::create_dir_all(&outpath).map_err(|e| format!("mkdir {outpath:?}: {e}"))?;
            continue;
        }
        if let Some(p) = outpath.parent() {
            fs::create_dir_all(p).map_err(|e| format!("mkdir {p:?}: {e}"))?;
        }
        let mut outfile =
            fs::File::create(&outpath).map_err(|e| format!("create {outpath:?}: {e}"))?;
        std::io::copy(&mut entry, &mut outfile)
            .map_err(|e| format!("extract {outpath:?}: {e}"))?;
        log.note(&format!("→ {}", entry.name()));
    }
    Ok(())
}
