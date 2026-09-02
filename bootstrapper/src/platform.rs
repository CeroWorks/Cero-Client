use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

pub const GITHUB_RELEASES_BASE: &str =
    "https://github.com/CeroWorks/Cero-Client/releases/latest/download";

#[cfg(target_arch = "x86_64")]
pub const ARCH_SUFFIX: &str = "x86_64";

#[cfg(target_arch = "aarch64")]
pub const ARCH_SUFFIX: &str = "arm64";

#[cfg(not(any(target_arch = "x86_64", target_arch = "aarch64")))]
compile_error!("Architecture non supportée");

#[cfg(target_os = "windows")]
pub const LAUNCHER_NAME: &str = "CeroClient.exe";

#[cfg(not(target_os = "windows"))]
pub const LAUNCHER_NAME: &str = "CeroClient";

#[cfg(target_os = "windows")]
pub const BOOTSTRAPPER_NAME: &str = "ceroclient-bootstrapper.exe";

#[cfg(not(target_os = "windows"))]
pub const BOOTSTRAPPER_NAME: &str = "ceroclient-bootstrapper";

#[cfg(target_os = "windows")]
pub const OS_SUFFIX: &str = "windows";

#[cfg(target_os = "linux")]
pub const OS_SUFFIX: &str = "linux";

#[cfg(target_os = "freebsd")]
pub const OS_SUFFIX: &str = "freebsd";

#[cfg(target_os = "macos")]
pub const OS_SUFFIX: &str = "macos";

#[cfg(not(any(
    target_os = "windows",
    target_os = "linux",
    target_os = "freebsd",
    target_os = "macos"
)))]
compile_error!("OS non supporté");

pub fn target_triplet() -> String {
    format!("{OS_SUFFIX}-{ARCH_SUFFIX}")
}

pub fn client_zip_name() -> String {
    format!("CeroClient-{}.zip", target_triplet())
}

pub fn bootstrapper_zip_name() -> String {
    format!("CeroClient-bootstrapper-{}.zip", target_triplet())
}

pub fn bin_dir() -> PathBuf {
    #[cfg(windows)]
    let base = PathBuf::from(
        std::env::var_os("APPDATA")
            .unwrap_or_else(|| panic!("Variable d'environnement APPDATA manquante")),
    );
    #[cfg(not(windows))]
    let base = PathBuf::from(
        std::env::var_os("HOME")
            .unwrap_or_else(|| panic!("Variable d'environnement HOME manquante")),
    );

    let dir = base.join(".ceroclient");
    fs::create_dir_all(&dir)
        .unwrap_or_else(|e| panic!("Impossible de créer {dir:?}: {e}"));
    dir
}

pub fn cleanup_old_self() {
    if let Ok(exe) = std::env::current_exe() {
        if let Some(dir) = exe.parent() {
            let old = dir.join("bootstrapper_old.exe");
            let _ = fs::remove_file(&old);

            let old2 = dir.join("bootstrapper_old");
            let _ = fs::remove_file(&old2);
        }
    }
}

#[cfg(unix)]
pub fn make_executable(path: &Path) {
    use std::os::unix::fs::PermissionsExt;
    if let Ok(meta) = fs::metadata(path) {
        let mut perms = meta.permissions();
        perms.set_mode(0o755);
        let _ = fs::set_permissions(path, perms);
    }
}
#[cfg(not(unix))]
pub fn make_executable(_: &Path) {}

pub fn spawn_detached(path: &Path, bin: &Path) -> Result<(), String> {
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
