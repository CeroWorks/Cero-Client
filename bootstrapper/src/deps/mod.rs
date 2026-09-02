#[cfg(any(target_os = "linux", target_os = "freebsd"))]
mod linux;

#[cfg(any(target_os = "linux", target_os = "freebsd"))]
pub use linux::check;

#[cfg(not(any(target_os = "linux", target_os = "freebsd")))]
pub fn check(_: &std::path::Path, _: &crate::log::Log) -> Result<(), String> {
    Ok(())
}
