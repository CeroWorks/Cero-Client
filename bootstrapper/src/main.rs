//#![cfg_attr(all(not(debug_assertions), target_os = "windows"), windows_subsystem = "windows")]

use sha2::{Digest, Sha256};
use std::fs;
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::Instant;

const GITHUB_RELEASES_BASE: &str =
    "https://github.com/CeroWorks/Cero-Client/releases/latest/download";

#[cfg(target_arch = "x86_64")]
const ARCH_SUFFIX: &str = "x86_64";

#[cfg(target_arch = "aarch64")]
const ARCH_SUFFIX: &str = "arm64";

#[cfg(not(any(target_arch = "x86_64", target_arch = "aarch64")))]
compile_error!("Architecture non supportée");

#[cfg(target_os = "windows")]
const LAUNCHER_NAME: &str = "CeroClient.exe";

#[cfg(not(target_os = "windows"))]
const LAUNCHER_NAME: &str = "CeroClient";

#[cfg(target_os = "windows")]
const BOOTSTRAPPER_NAME: &str = "ceroclient-bootstrapper.exe";

#[cfg(not(target_os = "windows"))]
const BOOTSTRAPPER_NAME: &str = "ceroclient-bootstrapper";

#[cfg(target_os = "windows")]
const OS_SUFFIX: &str = "windows";

#[cfg(target_os = "linux")]
const OS_SUFFIX: &str = "linux";

#[cfg(target_os = "freebsd")]
const OS_SUFFIX: &str = "freebsd";

#[cfg(target_os = "macos")]
const OS_SUFFIX: &str = "macos";

#[cfg(not(any(
    target_os = "windows",
    target_os = "linux",
    target_os = "freebsd",
    target_os = "macos"
)))]
compile_error!("OS non supporté");

fn target_triplet() -> String {
    format!("{OS_SUFFIX}-{ARCH_SUFFIX}")
}

fn client_zip_name() -> String {
    format!("CeroClient-{}.zip", target_triplet())
}

fn bootstrapper_zip_name() -> String {
    format!("CeroClient-bootstrapper-{}.zip", target_triplet())
}

struct Log {
    colors: bool,
}

impl Log {
    fn new() -> Self {
        #[cfg(windows)]
        let colors = Self::enable_ansi_windows();
        #[cfg(not(windows))]
        let colors = std::io::IsTerminal::is_terminal(&std::io::stdout());
        Self { colors }
    }

    #[cfg(windows)]
    fn enable_ansi_windows() -> bool {
        use std::os::windows::io::AsRawHandle;

        extern "system" {
            fn GetConsoleMode(handle: *mut std::ffi::c_void, mode: *mut u32) -> i32;
            fn SetConsoleMode(handle: *mut std::ffi::c_void, mode: u32) -> i32;
        }

        let handle = std::io::stdout().as_raw_handle();
        let mut mode: u32 = 0;
        unsafe {
            if GetConsoleMode(handle, &mut mode) != 0 {
                SetConsoleMode(handle, mode | 0x0004) != 0
            } else {
                false
            }
        }
    }

    fn banner(&self) {
        if self.colors {
            println!(
                "\n\
       \x1b[1m ▄▄▄▄▄▄▄\x1b[0m\n\
      \x1b[1m███▀▀▀▀▀\x1b[0m\n\
      \x1b[1m███      ▄█▀█▄ ████▄ ▄███▄\x1b[0m\n\
      \x1b[1m███      ██▄█▀ ██ ▀▀ ██ ██\x1b[0m\n\
      \x1b[1m▀███████ ▀█▄▄▄ ██    ▀███▀\x1b[0m\n\
            \x1b[2mBootstrapper\x1b[0m\n"
            );
        } else {
            println!("\n=== CeroClient Bootstrapper ===\n");
        }
    }

    fn section(&self, msg: &str) {
        if self.colors {
            println!("\n\x1b[1m\x1b[34m»\x1b[0m \x1b[1m{}\x1b[0m", msg);
        } else {
            println!("\n» {}", msg);
        }
    }

    fn ok(&self, msg: &str) {
        if self.colors {
            println!("  \x1b[32m✓\x1b[0m {}", msg);
        } else {
            println!("  [OK] {}", msg);
        }
    }

    fn info(&self, msg: &str) {
        if self.colors {
            println!("  \x1b[36m›\x1b[0m {}", msg);
        } else {
            println!("  [..] {}", msg);
        }
    }

    fn warn(&self, msg: &str) {
        if self.colors {
            println!("  \x1b[33m!\x1b[0m {}", msg);
        } else {
            println!("  [!!] {}", msg);
        }
    }

    fn err(&self, msg: &str) {
        if self.colors {
            eprintln!("  \x1b[31m✗\x1b[0m {}", msg);
        } else {
            eprintln!("  [ERR] {}", msg);
        }
    }

    fn note(&self, msg: &str) {
        if self.colors {
            println!("  \x1b[2m{}\x1b[0m", msg);
        } else {
            println!("  {}", msg);
        }
    }

    fn done(&self, msg: &str) {
        if self.colors {
            println!("\n  \x1b[1m\x1b[32m✓ {}\x1b[0m\n", msg);
        } else {
            println!("\n  [DONE] {}\n", msg);
        }
    }

    fn progress(&self, downloaded: u64, total: Option<u64>) {
        match total {
            Some(t) => {
                let pct = (downloaded as f64 / t as f64) * 100.0;
                let filled = ((pct / 5.0) as usize).min(20);
                let empty = 20usize.saturating_sub(filled);
                if self.colors {
                    print!(
                        "\r  \x1b[36m›\x1b[0m \x1b[2m[\x1b[0m\x1b[32m{}\x1b[2m{}\x1b[0m\x1b[2m]\x1b[0m {:5.1}%  {}  / {}  ",
                        "█".repeat(filled),
                        "░".repeat(empty),
                        pct,
                        fmt_bytes(downloaded),
                        fmt_bytes(t),
                    );
                } else {
                    print!("\r  {:.1}%  {}/{}", pct, fmt_bytes(downloaded), fmt_bytes(t));
                }
            }
            None => {
                if self.colors {
                    print!("\r  \x1b[36m›\x1b[0m {}    ", fmt_bytes(downloaded));
                } else {
                    print!("\r  {}    ", fmt_bytes(downloaded));
                }
            }
        }
        let _ = std::io::Write::flush(&mut std::io::stdout());
    }

    fn progress_done(&self) {
        println!();
    }
}

fn fmt_bytes(b: u64) -> String {
    if b >= 1_048_576 {
        format!("{:.1} Mo", b as f64 / 1_048_576.0)
    } else if b >= 1_024 {
        format!("{:.0} Ko", b as f64 / 1_024.0)
    } else {
        format!("{} o", b)
    }
}

fn agent() -> ureq::Agent {
    ureq::AgentBuilder::new()
        .timeout_connect(std::time::Duration::from_secs(10))
        .timeout(std::time::Duration::from_secs(60))
        .user_agent("CeroClient-Bootstrapper/0.1")
        .build()
}

fn http_get_text(agent: &ureq::Agent, url: &str) -> Result<String, String> {
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

fn http_download(
    agent: &ureq::Agent,
    url: &str,
    dest: &Path,
    log: &Log,
) -> Result<(), String> {
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
        fmt_bytes(downloaded),
        t0.elapsed().as_secs_f64()
    ));
    Ok(())
}

fn sha256_file(path: &Path) -> Option<String> {
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

fn lookup_checksum(checksums_txt: &str, filename: &str) -> Option<String> {
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

fn extract_zip(zip_path: &Path, dest: &Path, log: &Log) -> Result<(), String> {
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
        std::io::copy(&mut entry, &mut outfile).map_err(|e| format!("extract {outpath:?}: {e}"))?;
        log.note(&format!("→ {}", entry.name()));
    }
    Ok(())
}

fn ensure_zip_asset(
    agent: &ureq::Agent,
    zip_name: &str,
    dest_dir: &Path,
    bin: &Path,
    log: &Log,
) -> Result<bool, String> {
    let checksums = http_get_text(agent, &format!("{GITHUB_RELEASES_BASE}/checksums.txt"))
        .map_err(|e| format!("checksums.txt indisponible : {e}"))?;
    let remote_hash = lookup_checksum(&checksums, zip_name)
        .ok_or_else(|| format!("aucune entrée checksums.txt pour {zip_name}"))?;

    let local_zip = bin.join(zip_name);
    let local_hash = if local_zip.exists() {
        sha256_file(&local_zip).unwrap_or_default()
    } else {
        String::new()
    };

    if !local_hash.is_empty() && local_hash.eq_ignore_ascii_case(&remote_hash) {
        log.ok(&format!("{} déjà à jour", zip_name));
        return Ok(false);
    }

    log.info(&format!("↓ {}", zip_name));
    http_download(agent, &format!("{GITHUB_RELEASES_BASE}/{zip_name}"), &local_zip, log)?;

    let new_hash = sha256_file(&local_zip).ok_or("hash post-DL échoué")?;
    if !new_hash.eq_ignore_ascii_case(&remote_hash) {
        let _ = fs::remove_file(&local_zip);
        return Err(format!(
            "Checksum invalide pour {zip_name}\n    attendu : {remote_hash}\n    obtenu  : {new_hash}"
        ));
    }

    fs::create_dir_all(dest_dir).map_err(|e| format!("mkdir {dest_dir:?}: {e}"))?;
    extract_zip(&local_zip, dest_dir, log)?;
    log.ok(&format!("{} mis à jour et extrait", zip_name));
    Ok(true)
}

#[cfg(unix)]
fn make_executable(path: &Path) {
    use std::os::unix::fs::PermissionsExt;
    if let Ok(meta) = fs::metadata(path) {
        let mut perms = meta.permissions();
        perms.set_mode(0o755);
        let _ = fs::set_permissions(path, perms);
    }
}
#[cfg(not(unix))]
fn make_executable(_: &Path) {}

fn spawn_detached(path: &Path, bin: &Path) -> Result<(), String> {
    #[cfg(unix)]
    {
        Command::new(path)
            .current_dir(bin)
            .spawn()
            .map_err(|e| format!("spawn: {e}"))?;
    }
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        const DETACHED_PROCESS: u32 = 0x00000008;
        const CREATE_NEW_PROCESS_GROUP: u32 = 0x00000200;
        Command::new(path)
            .current_dir(bin)
            .creation_flags(DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP)
            .spawn()
            .map_err(|e| format!("spawn: {e}"))?;
    }
    Ok(())
}

#[cfg(any(target_os = "linux", target_os = "freebsd"))]
mod deps {
    use super::*;

    #[allow(dead_code)]
    #[derive(Clone, Copy, PartialEq)]
    enum Pm {
        Apt,
        Dnf,
        Pacman,
        Zypper,
        Emerge,
        Pkg,
        Unknown,
    }

    impl Pm {
        fn name(self) -> &'static str {
            match self {
                Pm::Apt => "apt",
                Pm::Dnf => "dnf",
                Pm::Pacman => "pacman",
                Pm::Zypper => "zypper",
                Pm::Emerge => "emerge",
                Pm::Pkg => "pkg",
                Pm::Unknown => "?",
            }
        }

        fn install_cmd(self) -> Option<(&'static str, Vec<&'static str>)> {
            match self {
                Pm::Apt => Some(("apt-get", vec!["install", "-y"])),
                Pm::Dnf => Some(("dnf", vec!["install", "-y"])),
                Pm::Pacman => Some(("pacman", vec!["-S", "--needed", "--noconfirm"])),
                Pm::Zypper => Some(("zypper", vec!["install", "-y"])),
                Pm::Emerge => Some(("emerge", vec!["--noreplace"])),
                Pm::Pkg => Some(("pkg", vec!["install", "-y"])),
                Pm::Unknown => None,
            }
        }
    }

    fn have(cmd: &str) -> bool {
        Command::new(cmd)
            .arg("--version")
            .stdout(std::process::Stdio::null())
            .stderr(std::process::Stdio::null())
            .status()
            .map(|s| s.success())
            .unwrap_or(false)
    }

    #[cfg(target_os = "freebsd")]
    fn detect_pm() -> Pm {
        if have("pkg") { Pm::Pkg } else { Pm::Unknown }
    }

    #[cfg(target_os = "linux")]
    fn detect_pm() -> Pm {
        let id = fs::read_to_string("/etc/os-release")
            .map(|c| c.to_lowercase())
            .unwrap_or_default();

        if (id.contains("id=arch") || id.contains("id_like=arch")) && have("pacman") {
            return Pm::Pacman;
        }
        if (id.contains("gentoo") || id.contains("id_like=gentoo")) && have("emerge") {
            return Pm::Emerge;
        }
        if (id.contains("fedora")
            || id.contains("rhel")
            || id.contains("centos")
            || id.contains("id_like=fedora"))
            && have("dnf")
        {
            return Pm::Dnf;
        }
        if (id.contains("suse") || id.contains("opensuse")) && have("zypper") {
            return Pm::Zypper;
        }
        if id.contains("debian")
            || id.contains("ubuntu")
            || id.contains("mint")
            || id.contains("id_like=debian")
        {
            if have("apt-get") {
                return Pm::Apt;
            }
        }

        for (cmd, pm) in [
            ("apt-get", Pm::Apt),
            ("dnf", Pm::Dnf),
            ("pacman", Pm::Pacman),
            ("zypper", Pm::Zypper),
            ("emerge", Pm::Emerge),
        ] {
            if have(cmd) {
                return pm;
            }
        }
        Pm::Unknown
    }

    fn pkg_for(lib: &str) -> Option<(&'static str, [&'static str; 6])> {
        let table: &[(&str, (&str, [&str; 6]))] = &[
            ("libwebkit2gtk-4.1.so", ("WebKit2GTK 4.1", [
                "libwebkit2gtk-4.1-0", "webkit2gtk4.1", "webkit2gtk-4.1",
                "webkit2gtk3-soup2", "net-libs/webkit-gtk:4.1", "webkit2-gtk3",
            ])),
            ("libwebkit2gtk-4.0.so", ("WebKit2GTK 4.0", [
                "libwebkit2gtk-4.0-37", "webkit2gtk3", "webkit2gtk",
                "webkit2gtk3", "net-libs/webkit-gtk:4", "webkit2-gtk3",
            ])),
            ("libgtk-3.so", ("GTK 3", [
                "libgtk-3-0", "gtk3", "gtk3", "gtk3", "x11-libs/gtk+:3", "gtk3",
            ])),
            ("libgtk-4.so", ("GTK 4", [
                "libgtk-4-1", "gtk4", "gtk4", "gtk4", "gui-libs/gtk:4", "gtk4",
            ])),
            ("libglib-2.0.so", ("GLib", [
                "libglib2.0-0", "glib2", "glib2", "glib2", "dev-libs/glib", "glib",
            ])),
            ("libgio-2.0.so", ("GIO", [
                "libglib2.0-0", "glib2", "glib2", "glib2", "dev-libs/glib", "glib",
            ])),
            ("libsoup-3.0.so", ("libsoup 3", [
                "libsoup-3.0-0", "libsoup3", "libsoup3", "libsoup-3_0-0",
                "net-libs/libsoup:3.0", "libsoup3",
            ])),
            ("libsoup-2.4.so", ("libsoup 2", [
                "libsoup2.4-1", "libsoup", "libsoup", "libsoup-2_4-1",
                "net-libs/libsoup:2.4", "libsoup",
            ])),
            ("libssl.so", ("OpenSSL", [
                "libssl3", "openssl-libs", "openssl", "libopenssl3",
                "dev-libs/openssl", "openssl",
            ])),
            ("libcrypto.so", ("OpenSSL crypto", [
                "libssl3", "openssl-libs", "openssl", "libopenssl3",
                "dev-libs/openssl", "openssl",
            ])),
            ("libcurl.so", ("cURL", [
                "libcurl4", "libcurl", "curl", "libcurl4",
                "net-misc/curl", "curl",
            ])),
            ("libX11.so", ("libX11", [
                "libx11-6", "libX11", "libx11", "libX11-6",
                "x11-libs/libX11", "libX11",
            ])),
            ("libxdo.so", ("libxdo", [
                "libxdo3", "libxdo", "xdotool", "libxdo3",
                "x11-misc/xdotool", "xdotool",
            ])),
            ("libayatana-appindicator3.so", ("AppIndicator", [
                "libayatana-appindicator3-1", "libayatana-appindicator-gtk3",
                "libayatana-appindicator", "libayatana-appindicator3-1",
                "dev-libs/libayatana-appindicator", "libayatana-appindicator",
            ])),
        ];
        for (key, v) in table {
            if lib.starts_with(key) {
                return Some(*v);
            }
        }
        None
    }

    fn pkg_index(pm: Pm) -> usize {
        match pm {
            Pm::Apt => 0,
            Pm::Dnf => 1,
            Pm::Pacman => 2,
            Pm::Zypper => 3,
            Pm::Emerge => 4,
            Pm::Pkg => 5,
            Pm::Unknown => 0,
        }
    }

    fn missing_libs(binary: &Path) -> Vec<String> {
        let Ok(out) = Command::new("ldd").arg(binary).output() else {
            return vec![];
        };
        let text = String::from_utf8_lossy(&out.stdout);
        text.lines()
            .filter(|l| l.contains("not found") || l.contains("=> not found"))
            .filter_map(|l| l.split_whitespace().next().map(String::from))
            .collect()
    }

    fn is_root() -> bool {
        Command::new("id")
            .arg("-u")
            .output()
            .ok()
            .and_then(|o| String::from_utf8(o.stdout).ok())
            .map(|s| s.trim() == "0")
            .unwrap_or(false)
    }

    fn ask_yes_no(log: &Log, question: &str) -> bool {
        use std::io::Write as _;
        print!("  ");
        if log.colors {
            print!("\x1b[33m?\x1b[0m ");
        } else {
            print!("[?] ");
        }
        print!("{question} [O/n] ");
        let _ = std::io::stdout().flush();

        let mut line = String::new();
        if std::io::stdin().read_line(&mut line).is_err() {
            return false;
        }
        let a = line.trim().to_lowercase();
        a.is_empty() || a == "o" || a == "oui" || a == "y" || a == "yes"
    }

    fn run_install(pm: Pm, pkgs: &[String], log: &Log) -> Result<(), String> {
        let (prog, fixed) = pm
            .install_cmd()
            .ok_or("Gestionnaire de paquets non supporté")?;

        let root = is_root();
        let use_sudo = !root && have("sudo");

        if !root && !use_sudo {
            return Err(
                "Privilèges root requis et 'sudo' introuvable. \
                 Relancez en root ou installez sudo."
                    .into(),
            );
        }

        let mut cmd = if use_sudo {
            let mut c = Command::new("sudo");
            c.arg(prog);
            c
        } else {
            Command::new(prog)
        };
        cmd.args(&fixed);
        for p in pkgs {
            cmd.arg(p);
        }

        let shown = format!(
            "{}{} {} {}",
            if use_sudo { "sudo " } else { "" },
            prog,
            fixed.join(" "),
            pkgs.join(" ")
        );
        log.info(&format!("Exécution : {shown}"));

        let status = cmd
            .status()
            .map_err(|e| format!("Échec d'exécution de {prog} : {e}"))?;

        if status.success() {
            Ok(())
        } else {
            Err(format!(
                "{prog} a retourné un code d'erreur ({})",
                status.code().unwrap_or(-1)
            ))
        }
    }

    pub fn check(binary: &Path, log: &Log) -> Result<(), String> {
        log.section("Vérification des dépendances");

        if !binary.exists() {
            return Err(format!("Binaire introuvable : {binary:?}"));
        }
        if !have("ldd") {
            log.warn("ldd indisponible, vérification ignorée");
            return Ok(());
        }

        let missing = missing_libs(binary);
        if missing.is_empty() {
            log.ok("Toutes les dépendances sont présentes");
            return Ok(());
        }

        let pm = detect_pm();
        log.err(&format!("{} bibliothèque(s) manquante(s) :", missing.len()));
        println!();

        let mut pkgs: Vec<String> = Vec::new();
        let mut unknown: Vec<String> = Vec::new();

        for lib in &missing {
            match pkg_for(lib) {
                Some((human, names)) => {
                    let pkg = names[pkg_index(pm)];
                    if pkg.is_empty() {
                        println!("    • {lib}");
                        println!("        ↳ {human}  (pas de paquet connu pour {})", pm.name());
                        unknown.push(lib.clone());
                    } else {
                        println!("    • {lib}");
                        println!("        ↳ {human}  (paquet : {pkg})");
                        pkgs.push(pkg.to_string());
                    }
                }
                None => {
                    println!("    • {lib}");
                    println!("        ↳ (paquet inconnu)");
                    unknown.push(lib.clone());
                }
            }
        }
        println!();

        if pm == Pm::Unknown {
            log.err("Gestionnaire de paquets non détecté — installation automatique impossible.");
            log.note("Installez manuellement les bibliothèques listées ci-dessus.");
            return Err("Dépendances manquantes (PM inconnu)".into());
        }

        pkgs.sort();
        pkgs.dedup();

        if pkgs.is_empty() {
            log.err("Aucun paquet installable automatiquement n'a pu être déterminé.");
            return Err("Dépendances manquantes (paquets inconnus)".into());
        }

        let manual = {
            let (prog, fixed) = pm.install_cmd().unwrap();
            format!(
                "{}{} {} {}",
                if is_root() { "" } else { "sudo " },
                prog,
                fixed.join(" "),
                pkgs.join(" ")
            )
        };
        log.note(&format!("Commande : {manual}"));
        println!();

        if !ask_yes_no(log, "Installer ces dépendances maintenant ?") {
            log.warn("Installation refusée par l'utilisateur.");
            log.note(&format!("Vous pouvez l'exécuter manuellement :\n      {manual}"));
            return Err("Dépendances manquantes (installation refusée)".into());
        }

        log.section("Installation des dépendances");
        run_install(pm, &pkgs, log)?;
        log.ok("Paquets installés");

        let still = missing_libs(binary);
        if still.is_empty() {
            log.ok("Toutes les dépendances sont désormais satisfaites");
            Ok(())
        } else {
            log.err(&format!(
                "{} bibliothèque(s) toujours manquante(s) après installation :",
                still.len()
            ));
            for l in &still {
                println!("    • {l}");
            }
            if !unknown.is_empty() {
                log.note("Certaines libs n'avaient pas de paquet connu — voir la liste plus haut.");
            }
            Err("Dépendances toujours manquantes après installation".into())
        }
    }
}

#[cfg(not(any(target_os = "linux", target_os = "freebsd")))]
mod deps {
    use super::*;
    pub fn check(_: &Path, _: &Log) -> Result<(), String> {
        Ok(())
    }
}

fn bin_dir() -> PathBuf {
    #[cfg(windows)]
    let base = PathBuf::from(std::env::var_os("APPDATA").expect("APPDATA manquant"));
    #[cfg(not(windows))]
    let base = PathBuf::from(std::env::var_os("HOME").expect("HOME manquant"));

    let dir = base.join(".ceroclient");
    fs::create_dir_all(&dir).expect("Impossible de créer .ceroclient");
    dir
}

fn cleanup_old_self() {
    if let Ok(exe) = std::env::current_exe() {
        if let Some(dir) = exe.parent() {
            let old = dir.join("bootstrapper_old.exe");
            let _ = fs::remove_file(&old);

            let old2 = dir.join("bootstrapper_old");
            let _ = fs::remove_file(&old2);
        }
    }
}

fn self_update(agent: &ureq::Agent, log: &Log) -> Result<bool, String> {
    log.section("Vérification du bootstrapper");

    let current_exe = std::env::current_exe()
        .map_err(|e| format!("current_exe: {e}"))?;
    let dir = current_exe.parent()
        .ok_or("dossier de l'exe introuvable")?
        .to_path_buf();

    let zip_name = bootstrapper_zip_name();

    let checksums = match http_get_text(agent, &format!("{GITHUB_RELEASES_BASE}/checksums.txt")) {
        Ok(s) => s,
        Err(e) => {
            log.warn(&format!("checksums.txt indisponible : {e}"));
            return Ok(false);
        }
    };
    let remote_hash = match lookup_checksum(&checksums, &zip_name) {
        Some(h) => h,
        None => {
            log.warn(&format!("aucune entrée checksums.txt pour {zip_name}"));
            return Ok(false);
        }
    };

    let stamp_file = dir.join(format!("{zip_name}.sha256"));
    let previous_zip_hash = fs::read_to_string(&stamp_file).unwrap_or_default().trim().to_lowercase();
    if previous_zip_hash == remote_hash.to_lowercase() {
        log.ok("Bootstrapper à jour");
        return Ok(false);
    }

    log.info("Nouvelle version détectée, téléchargement...");

    let new_zip = dir.join(&zip_name);
    let _ = fs::remove_file(&new_zip);
    http_download(agent, &format!("{GITHUB_RELEASES_BASE}/{zip_name}"), &new_zip, log)?;

    let dl_hash = sha256_file(&new_zip)
        .ok_or("hash post-DL échoué")?
        .to_lowercase();
    if dl_hash != remote_hash.to_lowercase() {
        let _ = fs::remove_file(&new_zip);
        return Err(format!("checksum mismatch (attendu {remote_hash}, obtenu {dl_hash})"));
    }

    let extract_dir = dir.join("bootstrapper_update");
    let _ = fs::remove_dir_all(&extract_dir);
    fs::create_dir_all(&extract_dir).map_err(|e| format!("mkdir {extract_dir:?}: {e}"))?;
    extract_zip(&new_zip, &extract_dir, log)?;
    let _ = fs::remove_file(&new_zip);

    let extracted_exe = extract_dir.join(BOOTSTRAPPER_NAME);
    if !extracted_exe.exists() {
        return Err(format!("{BOOTSTRAPPER_NAME} introuvable dans {zip_name}"));
    }

    let old_exe = dir.join(if cfg!(windows) { "bootstrapper_old.exe" } else { "bootstrapper_old" });
    let _ = fs::remove_file(&old_exe);
    fs::rename(&current_exe, &old_exe)
        .map_err(|e| format!("rename current→old: {e}"))?;

    if let Err(e) = fs::rename(&extracted_exe, &current_exe) {
        let _ = fs::rename(&old_exe, &current_exe);
        return Err(format!("échec du remplacement : {e}"));
    }
    let _ = fs::remove_dir_all(&extract_dir);
    fs::write(&stamp_file, &remote_hash).ok();

    make_executable(&current_exe);

    log.ok("Bootstrapper mis à jour, relancement...");

    Command::new(&current_exe)
        .spawn()
        .map_err(|e| format!("spawn relance: {e}"))?;

    Ok(true)
}

fn main() {
    cleanup_old_self();

    let log = Log::new();
    let agent = agent();

    match self_update(&agent, &log) {
        Ok(true) => std::process::exit(0),
        Ok(false) => {}
        Err(e) => log.warn(&format!("Self-update échoué : {e} (on continue)")),
    }

    log.banner();

    let bin = bin_dir();

    log.section("Mise à jour du launcher et des assets");
    if let Err(e) = ensure_zip_asset(&agent, &client_zip_name(), &bin, &bin, &log) {
        log.err(&format!("Launcher/assets : {e}"));
        std::process::exit(1);
    }
    make_executable(&bin.join(LAUNCHER_NAME));

    #[cfg(windows)]
    {
        log.section("Mise à jour des DLLs");
        if let Err(e) = ensure_zip_asset(&agent, "CeroClient_windows_dll.zip", &bin, &bin, &log) {
            log.err(&format!("DLLs : {e}"));
            std::process::exit(1);
        }
    }

    let launcher_path = bin.join(LAUNCHER_NAME);
    if let Err(e) = deps::check(&launcher_path, &log) {
        log.err(&e);
        std::process::exit(2);
    }

    log.section("Lancement");
    log.info(&format!("{}", launcher_path.display()));

    if let Err(e) = spawn_detached(&launcher_path, &bin) {
        log.err(&e);
        std::process::exit(1);
    }

    log.done("Launcher démarré !");
    std::process::exit(0);
}