use directories::*;
use std::fs::{create_dir_all, OpenOptions};
use std::io::Write;
use std::path::PathBuf;

#[cfg(any(target_os = "macos", target_os = "linux"))]
use std::fs;

#[cfg(any(target_os = "macos", target_os = "linux"))]
use std::os::unix::fs::PermissionsExt;

#[cfg(any(target_os = "macos", target_os = "linux"))]
const BIN: &[u8] = include_bytes!("./../../cmake-build/toidb");
#[cfg(any(target_os = "windows"))]
const BIN: &[u8] = include_bytes!("./../../cmake-build/toidb.exe");

#[cfg(any(target_os = "windows"))]
const DEP1: &[u8] = include_bytes!("./../../cmake-build/fm.dll");
#[cfg(any(target_os = "windows"))]
const DEP2: &[u8] = include_bytes!("./../../cmake-build/serializer.dll");
#[cfg(any(target_os = "windows"))]
const DEP3: &[u8] = include_bytes!("./../../cmake-build/tcpserver.dll");

macro_rules! write_to_path {
    ($bytes:expr, $path:expr) => {{
        let mut file = OpenOptions::new()
            .write(true)
            .append(false)
            .create(true)
            .truncate(true)
            .open($path)?;
        file.write_all($bytes)?;
    }};
}

pub fn get_data_dir() -> Result<PathBuf, Box<dyn std::error::Error>> {
    let Some(p) = ProjectDirs::from(
        "com", // qualifier
        "up",  // organization
        "toi", // application
    ) else {
        return Err("Failed to get data directory".into());
    };

    let p = PathBuf::from(p.data_local_dir());

    if !&p.exists() {
        create_dir_all(&p).unwrap();
    };

    Ok(p)
}

pub fn colocar_dependencias() -> Result<(), Box<dyn std::error::Error>> {
    let data_folder = get_data_dir()?;
    let bin_folder = data_folder.join("bin");

    if !bin_folder.exists() {
        std::fs::create_dir_all(&bin_folder)?;
    }

    write_to_path!(BIN, &bin_folder.join("toidb"));

    #[cfg(any(target_os = "windows"))]
    {
        write_to_path!(DEP1, &bin_folder.join("fm.dll"));
        write_to_path!(DEP2, &bin_folder.join("serializer.dll"));
        write_to_path!(DEP3, &bin_folder.join("tcpserver.dll"));
    }

    #[cfg(any(target_os = "macos", target_os = "linux"))]
    {
        let mut perms = fs::metadata(&bin_folder.join("toidb"))
            .unwrap()
            .permissions();
        perms.set_mode(0o744);
        std::fs::set_permissions(&bin_folder.join("toidb"), perms).unwrap();
    };

    Ok(())
}
