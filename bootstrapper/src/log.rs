pub struct Log {
    pub colors: bool,
}

impl Log {
    pub fn new() -> Self {
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

    pub fn banner(&self) {
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

    pub fn section(&self, msg: &str) {
        if self.colors {
            println!("\n\x1b[1m\x1b[34m»\x1b[0m \x1b[1m{}\x1b[0m", msg);
        } else {
            println!("\n» {}", msg);
        }
    }

    pub fn ok(&self, msg: &str) {
        if self.colors {
            println!("  \x1b[32m✓\x1b[0m {}", msg);
        } else {
            println!("  [OK] {}", msg);
        }
    }

    pub fn info(&self, msg: &str) {
        if self.colors {
            println!("  \x1b[36m›\x1b[0m {}", msg);
        } else {
            println!("  [..] {}", msg);
        }
    }

    pub fn warn(&self, msg: &str) {
        if self.colors {
            println!("  \x1b[33m!\x1b[0m {}", msg);
        } else {
            println!("  [!!] {}", msg);
        }
    }

    pub fn err(&self, msg: &str) {
        if self.colors {
            eprintln!("  \x1b[31m✗\x1b[0m {}", msg);
        } else {
            eprintln!("  [ERR] {}", msg);
        }
    }

    pub fn note(&self, msg: &str) {
        if self.colors {
            println!("  \x1b[2m{}\x1b[0m", msg);
        } else {
            println!("  {}", msg);
        }
    }

    pub fn done(&self, msg: &str) {
        if self.colors {
            println!("\n  \x1b[1m\x1b[32m✓ {}\x1b[0m\n", msg);
        } else {
            println!("\n  [DONE] {}\n", msg);
        }
    }

    pub fn progress(&self, downloaded: u64, total: Option<u64>) {
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

    pub fn progress_done(&self) {
        println!();
    }
}

pub fn fmt_bytes(b: u64) -> String {
    if b >= 1_048_576 {
        format!("{:.1} Mo", b as f64 / 1_048_576.0)
    } else if b >= 1_024 {
        format!("{:.0} Ko", b as f64 / 1_024.0)
    } else {
        format!("{} o", b)
    }
}
