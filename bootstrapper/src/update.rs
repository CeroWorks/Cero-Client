use crate::archive::extract_zip;
use crate::http;
use crate::log::Log;
use crate::platform::{self, BOOTSTRAPPER_NAME, GITHUB_RELEASES_BASE};
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

fn fetch_verified_zip(
    agent: &ureq::Agent,
    zip_name: &str,
    dest_dir: &Path,
    log: &Log,
) -> Result<PathBuf, String> {
    let checksums = http::get_text(agent, &format!("{GITHUB_RELEASES_BASE}/checksums.txt"))
        .map_err(|e| format!("checksums.txt indisponible : {e}"))?;
    let remote_hash = http::lookup_checksum(&checksums, zip_name)
        .ok_or_else(|| format!("aucune entrée checksums.txt pour {zip_name}"))?;

    let zip_path = dest_dir.join(zip_name);
    let _ = fs::remove_file(&zip_path);

    log.info(&format!("↓ {}", zip_name));
    http::download(
        agent,
        &format!("{GITHUB_RELEASES_BASE}/{zip_name}"),
        &zip_path,
        log,
    )?;

    let new_hash = http::sha256_file(&zip_path).ok_or("hash post-DL échoué")?;
    if !new_hash.eq_ignore_ascii_case(&remote_hash) {
        let _ = fs::remove_file(&zip_path);
        return Err(format!(
            "Checksum invalide pour {zip_name}\n    attendu : {remote_hash}\n    obtenu  : {new_hash}"
        ));
    }

    Ok(zip_path)
}

pub fn ensure_zip_asset(
    agent: &ureq::Agent,
    zip_name: &str,
    dest_dir: &Path,
    bin: &Path,
    log: &Log,
) -> Result<bool, String> {
    let checksums = http::get_text(agent, &format!("{GITHUB_RELEASES_BASE}/checksums.txt"))
        .map_err(|e| format!("checksums.txt indisponible : {e}"))?;
    let remote_hash = http::lookup_checksum(&checksums, zip_name)
        .ok_or_else(|| format!("aucune entrée checksums.txt pour {zip_name}"))?;

    let local_zip = bin.join(zip_name);
    let local_hash = if local_zip.exists() {
        http::sha256_file(&local_zip).unwrap_or_default()
    } else {
        String::new()
    };

    if !local_hash.is_empty() && local_hash.eq_ignore_ascii_case(&remote_hash) {
        log.ok(&format!("{} déjà à jour", zip_name));
        return Ok(false);
    }

    let zip_path = fetch_verified_zip(agent, zip_name, bin, log)?;

    fs::create_dir_all(dest_dir).map_err(|e| format!("mkdir {dest_dir:?}: {e}"))?;
    extract_zip(&zip_path, dest_dir, log)?;
    log.ok(&format!("{} mis à jour et extrait", zip_name));
    Ok(true)
}

pub fn self_update(agent: &ureq::Agent, log: &Log) -> Result<bool, String> {
    log.section("Vérification du bootstrapper");

    let current_exe = std::env::current_exe().map_err(|e| format!("current_exe: {e}"))?;
    let dir = current_exe
        .parent()
        .ok_or("dossier de l'exe introuvable")?
        .to_path_buf();

    let zip_name = platform::bootstrapper_zip_name();

    let checksums = match http::get_text(agent, &format!("{GITHUB_RELEASES_BASE}/checksums.txt"))
    {
        Ok(s) => s,
        Err(e) => {
            log.warn(&format!("checksums.txt indisponible : {e}"));
            return Ok(false);
        }
    };
    let remote_hash = match http::lookup_checksum(&checksums, &zip_name) {
        Some(h) => h,
        None => {
            log.warn(&format!("aucune entrée checksums.txt pour {zip_name}"));
            return Ok(false);
        }
    };

    let stamp_file = dir.join(format!("{zip_name}.sha256"));
    let previous_zip_hash = fs::read_to_string(&stamp_file)
        .unwrap_or_default()
        .trim()
        .to_lowercase();
    if previous_zip_hash == remote_hash.to_lowercase() {
        log.ok("Bootstrapper à jour");
        return Ok(false);
    }

    log.info("Nouvelle version détectée, téléchargement...");
    let new_zip = fetch_verified_zip(agent, &zip_name, &dir, log)?;

    let extract_dir = dir.join("bootstrapper_update");
    let _ = fs::remove_dir_all(&extract_dir);
    fs::create_dir_all(&extract_dir).map_err(|e| format!("mkdir {extract_dir:?}: {e}"))?;
    extract_zip(&new_zip, &extract_dir, log)?;
    let _ = fs::remove_file(&new_zip);

    let extracted_exe = extract_dir.join(BOOTSTRAPPER_NAME);
    if !extracted_exe.exists() {
        return Err(format!("{BOOTSTRAPPER_NAME} introuvable dans {zip_name}"));
    }

    let old_exe = dir.join(if cfg!(windows) {
        "bootstrapper_old.exe"
    } else {
        "bootstrapper_old"
    });
    let _ = fs::remove_file(&old_exe);
    fs::rename(&current_exe, &old_exe).map_err(|e| format!("rename current→old: {e}"))?;

    if let Err(e) = fs::rename(&extracted_exe, &current_exe) {
        let _ = fs::rename(&old_exe, &current_exe);
        return Err(format!("échec du remplacement : {e}"));
    }
    let _ = fs::remove_dir_all(&extract_dir);
    fs::write(&stamp_file, &remote_hash).ok();

    platform::make_executable(&current_exe);

    log.ok("Bootstrapper mis à jour, relancement...");

    Command::new(&current_exe)
        .spawn()
        .map_err(|e| format!("spawn relance: {e}"))?;

    Ok(true)
}
