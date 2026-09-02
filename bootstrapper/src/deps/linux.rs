use crate::log::Log;
use std::path::Path;
use std::process::Command;
use std::fs;

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
    if have("pkg") {
        Pm::Pkg
    } else {
        Pm::Unknown
    }
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
        (
            "libwebkit2gtk-4.1.so",
            (
                "WebKit2GTK 4.1",
                [
                    "libwebkit2gtk-4.1-0",
                    "webkit2gtk4.1",
                    "webkit2gtk-4.1",
                    "webkit2gtk3-soup2",
                    "net-libs/webkit-gtk:4.1",
                    "webkit2-gtk3",
                ],
            ),
        ),
        (
            "libwebkit2gtk-4.0.so",
            (
                "WebKit2GTK 4.0",
                [
                    "libwebkit2gtk-4.0-37",
                    "webkit2gtk3",
                    "webkit2gtk",
                    "webkit2gtk3",
                    "net-libs/webkit-gtk:4",
                    "webkit2-gtk3",
                ],
            ),
        ),
        (
            "libgtk-3.so",
            (
                "GTK 3",
                ["libgtk-3-0", "gtk3", "gtk3", "gtk3", "x11-libs/gtk+:3", "gtk3"],
            ),
        ),
        (
            "libgtk-4.so",
            (
                "GTK 4",
                ["libgtk-4-1", "gtk4", "gtk4", "gtk4", "gui-libs/gtk:4", "gtk4"],
            ),
        ),
        (
            "libglib-2.0.so",
            (
                "GLib",
                [
                    "libglib2.0-0",
                    "glib2",
                    "glib2",
                    "glib2",
                    "dev-libs/glib",
                    "glib",
                ],
            ),
        ),
        (
            "libgio-2.0.so",
            (
                "GIO",
                [
                    "libglib2.0-0",
                    "glib2",
                    "glib2",
                    "glib2",
                    "dev-libs/glib",
                    "glib",
                ],
            ),
        ),
        (
            "libsoup-3.0.so",
            (
                "libsoup 3",
                [
                    "libsoup-3.0-0",
                    "libsoup3",
                    "libsoup3",
                    "libsoup-3_0-0",
                    "net-libs/libsoup:3.0",
                    "libsoup3",
                ],
            ),
        ),
        (
            "libsoup-2.4.so",
            (
                "libsoup 2",
                [
                    "libsoup2.4-1",
                    "libsoup",
                    "libsoup",
                    "libsoup-2_4-1",
                    "net-libs/libsoup:2.4",
                    "libsoup",
                ],
            ),
        ),
        (
            "libssl.so",
            (
                "OpenSSL",
                [
                    "libssl3",
                    "openssl-libs",
                    "openssl",
                    "libopenssl3",
                    "dev-libs/openssl",
                    "openssl",
                ],
            ),
        ),
        (
            "libcrypto.so",
            (
                "OpenSSL crypto",
                [
                    "libssl3",
                    "openssl-libs",
                    "openssl",
                    "libopenssl3",
                    "dev-libs/openssl",
                    "openssl",
                ],
            ),
        ),
        (
            "libcurl.so",
            (
                "cURL",
                ["libcurl4", "libcurl", "curl", "libcurl4", "net-misc/curl", "curl"],
            ),
        ),
        (
            "libX11.so",
            (
                "libX11",
                ["libx11-6", "libX11", "libx11", "libX11-6", "x11-libs/libX11", "libX11"],
            ),
        ),
        (
            "libxdo.so",
            (
                "libxdo",
                ["libxdo3", "libxdo", "xdotool", "libxdo3", "x11-misc/xdotool", "xdotool"],
            ),
        ),
        (
            "libayatana-appindicator3.so",
            (
                "AppIndicator",
                [
                    "libayatana-appindicator3-1",
                    "libayatana-appindicator-gtk3",
                    "libayatana-appindicator",
                    "libayatana-appindicator3-1",
                    "dev-libs/libayatana-appindicator",
                    "libayatana-appindicator",
                ],
            ),
        ),
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
