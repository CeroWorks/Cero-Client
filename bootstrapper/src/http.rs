use crate::log::Log;
use sha2::{Digest, Sha256};
use std::fs;
use std::io::{Read, Write};
use std::path::Path;
use std::time::Instant;

pub fn agent() -> ureq::Agent {
    ureq::AgentBuilder::new()
        .timeout_connect(std::time::Duration::from_secs(10))
        .timeout(std::time::Duration::from_secs(60))
        .user_agent("CeroClient-Bootstrapper/0.1")
        .build()
}

pub fn get_text(agent: &ureq::Agent, url: &str) -> Result<String, String> {
    let resp = agent
        .get(url)
        .call()
        .map_err(|e| format!("GET {url}: {e}"))?;
    Ok(resp
        .into_string()
        .map_err(|e| format!("text: {e}"))?
        .trim()
        .to_string())
}

pub fn download(agent: &ureq::Agent, url: &str, dest: &Path, log: &Log) -> Result<(), String> {
    let t0 = Instant::now();
    let resp = agent
        .get(url)
        .call()
        .map_err(|e| format!("GET {url}: {e}"))?;

    let total = resp
        .header("content-length")
        .and_then(|v| v.parse::<u64>().ok());

    let tmp = dest.with_extension("tmp");
    let mut file = fs::File::create(&tmp).map_err(|e| format!("create {tmp:?}: {e}"))?;
    let mut reader = resp.into_reader();
    let mut buf = [0u8; 64 * 1024];
    let mut downloaded: u64 = 0;
    let mut last = Instant::now();

    loop {
        let n = reader.read(&mut buf).map_err(|e| format!("read: {e}"))?;
        if n == 0 {
            break;
        }
        file.write_all(&buf[..n]).map_err(|e| format!("write: {e}"))?;
        downloaded += n as u64;
        if last.elapsed().as_millis() >= 100 {
            log.progress(downloaded, total);
            last = Instant::now();
        }
    }
    log.progress(downloaded, total);
    log.progress_done();

    file.flush().map_err(|e| format!("flush: {e}"))?;
    drop(file);
    fs::rename(&tmp, dest).map_err(|e| format!("rename: {e}"))?;

    log.note(&format!(
        "→ {} en {:.1}s",
        crate::log::fmt_bytes(downloaded),
        t0.elapsed().as_secs_f64()
    ));
    Ok(())
}

pub fn sha256_file(path: &Path) -> Option<String> {
    let mut file = fs::File::open(path).ok()?;
    let mut hasher = Sha256::new();
    let mut buf = [0u8; 8192];
    loop {
        let n = file.read(&mut buf).ok()?;
        if n == 0 {
            break;
        }
        hasher.update(&buf[..n]);
    }
    Some(format!("{:x}", hasher.finalize()))
}

pub fn lookup_checksum(checksums_txt: &str, filename: &str) -> Option<String> {
    checksums_txt.lines().find_map(|line| {
        let mut parts = line.split_whitespace();
        let hash = parts.next()?;
        let name = parts.next()?.trim_start_matches('*');
        if name == filename {
            Some(hash.to_lowercase())
        } else {
            None
        }
    })
}
