#![allow(clippy::missing_safety_doc)]

use path_absolutize::*;
use std::ffi::{c_char, CStr, CString};

#[no_mangle]
pub unsafe fn exists(path: *const c_char) -> bool {
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
pub unsafe fn append(path: *mut c_char, rhs: *mut c_char) -> *mut c_char {
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
    }.map(|s| s.into_raw())
      .unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub unsafe fn set_working_dir(path: *const c_char) -> bool {
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