#![allow(clippy::missing_safety_doc)]

use path_absolutize::*;
use std::ffi::{c_char, CStr, CString};

#[no_mangle]
pub unsafe fn drop_csring(c_str: *mut c_char) {
    if c_str.is_null() {
        return;
    }

    let _ = CString::from_raw(c_str);
}

#[no_mangle]
pub unsafe fn exists(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            path.exists()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn is_dir(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            path.is_dir()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn is_file(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            path.is_file()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn is_symlink(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            path.is_symlink()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn is_absolute(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            path.is_absolute()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn is_relative(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            path.is_relative()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn get_absolute(path: *const c_char) -> *mut c_char {
    if path.is_null() {
        return std::ptr::null_mut();
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    let result = match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            let absolute = path.absolutize();

            match absolute {
                Ok(p) => {
                    let s = p.to_str();
                    match s {
                        Some(s) => CString::new(s),
                        None => {
                            return std::ptr::null_mut();
                        }
                    }
                }
                Err(_) => {
                    return std::ptr::null_mut();
                }
            }
        }
        Err(_) => {
            return std::ptr::null_mut();
        }
    };

    match result {
        Ok(r) => r.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub unsafe fn get_parent(path: *const c_char) -> *mut c_char {
    if path.is_null() {
        return std::ptr::null_mut();
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    let result = match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            let parent = path.parent();

            match parent {
                Some(p) => {
                    let s = p.to_str();
                    match s {
                        Some(s) => CString::new(s),
                        None => {
                            return std::ptr::null_mut();
                        }
                    }
                }
                None => {
                    return std::ptr::null_mut();
                }
            }
        }
        Err(_) => {
            return std::ptr::null_mut();
        }
    };

    match result {
        Ok(r) => r.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub unsafe fn get_file_name(path: *const c_char) -> *mut c_char {
    if path.is_null() {
        return std::ptr::null_mut();
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    let result = match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            let file_name = path.file_name();

            match file_name {
                Some(p) => {
                    let s = p.to_str();
                    match s {
                        Some(s) => CString::new(s),
                        None => {
                            return std::ptr::null_mut();
                        }
                    }
                }
                None => {
                    return std::ptr::null_mut();
                }
            }
        }
        Err(_) => {
            return std::ptr::null_mut();
        }
    };

    match result {
        Ok(r) => r.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub unsafe fn get_extension(path: *const c_char) -> *mut c_char {
    if path.is_null() {
        return std::ptr::null_mut();
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    let result = match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            let extension = path.extension();

            match extension {
                Some(p) => {
                    let s = p.to_str();
                    match s {
                        Some(s) => CString::new(s),
                        None => {
                            return std::ptr::null_mut();
                        }
                    }
                }
                None => {
                    return std::ptr::null_mut();
                }
            }
        }
        Err(_) => {
            return std::ptr::null_mut();
        }
    };

    match result {
        Ok(r) => r.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub unsafe fn get_user_home() -> *mut c_char {
    directories::UserDirs::new()
        .and_then(|dirs| dirs.home_dir().to_str().map(CString::new))
        .map(|s| match s {
            Ok(s) => s.into_raw(),
            Err(_) => std::ptr::null_mut(),
        })
        .unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub unsafe fn get_data_folder() -> *mut c_char {
    directories::BaseDirs::new()
        .and_then(|dirs| dirs.data_dir().to_str().map(CString::new))
        .map(|s| match s {
            Ok(s) => s.into_raw(),
            Err(_) => std::ptr::null_mut(),
        })
        .unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub unsafe fn get_project_dir(
    qualifier: *mut c_char,
    org: *mut c_char,
    app: *mut c_char,
) -> *mut c_char {
    // Check if qualifier is a null pointer
    let qualifier = if qualifier.is_null() {
        return std::ptr::null_mut();
    } else {
        let c_str = CStr::from_ptr(qualifier);
        let str = c_str.to_str();
        match str {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    // Check if org is a null pointer
    let org = if org.is_null() {
        return std::ptr::null_mut();
    } else {
        let c_str = CStr::from_ptr(org);
        let str = c_str.to_str();
        match str {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    // Check if app is a null pointer
    let app = if app.is_null() {
        return std::ptr::null_mut();
    } else {
        let c_str = CStr::from_ptr(app);
        let str = c_str.to_str();
        match str {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    directories::ProjectDirs::from(qualifier, org, app)
        .and_then(|dirs| dirs.data_dir().to_str().map(CString::new))
        .map(|s| match s {
            Ok(s) => s.into_raw(),
            Err(_) => std::ptr::null_mut(),
        })
        .unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub unsafe fn get_config_folder() -> *mut c_char {
    directories::BaseDirs::new()
        .and_then(|dirs| dirs.config_dir().to_str().map(CString::new))
        .map(|s| match s {
            Ok(s) => s.into_raw(),
            Err(_) => std::ptr::null_mut(),
        })
        .unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub unsafe fn remove_dir(path: *const c_char) -> bool {
    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            std::fs::remove_dir_all(path).is_ok()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn remove_file(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            std::fs::remove_file(path).is_ok()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn create_dir(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            std::fs::create_dir_all(path).is_ok()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn create_file(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            std::fs::File::create(path).is_ok()
        }
        Err(_) => false,
    }
}

#[no_mangle]
pub unsafe fn rename_file(old_path: *const c_char, new_path: *const c_char) -> bool {
    if old_path.is_null() || new_path.is_null() {
        return false;
    }

    let old_c_str = CStr::from_ptr(old_path);
    let old_str = old_c_str.to_str();

    let new_c_str = CStr::from_ptr(new_path);
    let new_str = new_c_str.to_str();

    match (old_str, new_str) {
        (Ok(old_s), Ok(new_s)) => {
            let old_path = std::path::Path::new(old_s);
            let new_path = std::path::Path::new(new_s);
            std::fs::rename(old_path, new_path).is_ok()
        }
        _ => false,
    }
}

#[no_mangle]
pub unsafe fn rename_dir(old_path: *const c_char, new_path: *const c_char) -> bool {
    if old_path.is_null() || new_path.is_null() {
        return false;
    }

    let old_c_str = CStr::from_ptr(old_path);
    let old_str = old_c_str.to_str();

    let new_c_str = CStr::from_ptr(new_path);
    let new_str = new_c_str.to_str();

    match (old_str, new_str) {
        (Ok(old_s), Ok(new_s)) => {
            let old_path = std::path::Path::new(old_s);
            let new_path = std::path::Path::new(new_s);
            std::fs::rename(old_path, new_path).is_ok()
        }
        _ => false,
    }
}

#[no_mangle]
pub unsafe fn copy_file(old_path: *const c_char, new_path: *const c_char) -> bool {
    if old_path.is_null() || new_path.is_null() {
        return false;
    }

    let old_c_str = CStr::from_ptr(old_path);
    let old_str = old_c_str.to_str();

    let new_c_str = CStr::from_ptr(new_path);
    let new_str = new_c_str.to_str();

    match (old_str, new_str) {
        (Ok(old_s), Ok(new_s)) => {
            let old_path = std::path::Path::new(old_s);
            let new_path = std::path::Path::new(new_s);
            std::fs::copy(old_path, new_path).is_ok()
        }
        _ => false,
    }
}

#[no_mangle]
pub unsafe fn get_file_size(path: *const c_char) -> u64 {
    if path.is_null() {
        return 0;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            match std::fs::metadata(path) {
                Ok(m) => m.len(),
                Err(_) => 0,
            }
        }
        Err(_) => 0,
    }
}

#[no_mangle]
pub unsafe fn append(path: *mut c_char, rhs: *mut c_char) -> *mut c_char {
    if path.is_null() || rhs.is_null() {
        return std::ptr::null_mut();
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    let result = match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            let c_str = CStr::from_ptr(rhs);
            let str = c_str.to_str();

            match str {
                Ok(s) => {
                    let rhs = std::path::Path::new(s);
                    let new_path = path.join(rhs);

                    let s = new_path.to_str();
                    match s {
                        Some(s) => CString::new(s),
                        None => {
                            return std::ptr::null_mut();
                        }
                    }
                }
                Err(_) => {
                    return std::ptr::null_mut();
                }
            }
        }
        Err(_) => {
            return std::ptr::null_mut();
        }
    };

    match result {
        Ok(r) => r.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub fn get_working_dir() -> *mut c_char {
    match std::env::current_dir() {
        Ok(p) => {
            let s = p.to_str();
            match s {
                Some(s) => CString::new(s),
                None => {
                    return std::ptr::null_mut();
                }
            }
        }
        Err(_) => {
            return std::ptr::null_mut();
        }
    }
    .map(|s| s.into_raw())
    .unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub unsafe fn set_working_dir(path: *const c_char) -> bool {
    if path.is_null() {
        return false;
    }

    let c_str = CStr::from_ptr(path);
    let str = c_str.to_str();

    match str {
        Ok(s) => {
            let path = std::path::Path::new(s);
            std::env::set_current_dir(path).is_ok()
        }
        Err(_) => false,
    }
}
