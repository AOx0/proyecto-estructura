#![allow(clippy::missing_safety_doc)]

use path_absolutize::*;
use std::ffi::{c_char, CStr, CString};

macro_rules! null_guard {
    [$($var:expr$(,)*),* => $return_var:expr] => {
        $(
            if $var.is_null() { return $return_var ; }
        )*
    };
}

macro_rules! try_str {
    [$var:expr => $return_var:expr] => {
        match CStr::from_ptr($var).to_str() {
            Ok(s) => s,
            Err(_) => return $return_var,
        }
    };
}

macro_rules! try_cstring {
    [$var:expr => $return_var:expr] => {
        match CString::new($var) {
            Ok(s) => s,
            Err(_) => return $return_var,
        }
    };
}

macro_rules! path_info_functions {
    [$($name:ident$(,)*),*] => {
        $(
            #[no_mangle]
            pub unsafe fn $name(path: *const c_char) -> bool  {
                null_guard![path => false];
                std::path::Path::new(try_str![path => false]).$name()
            }
        )*
    };
}

macro_rules! path_methods {
    [$($name:ident:$method:ident$(,)*),*] => {
        $(
            #[no_mangle]
            pub unsafe fn $name(path: *const c_char) -> *mut c_char {
                null_guard![path => std::ptr::null_mut()];
                let path = std::path::Path::new(try_str![path => std::ptr::null_mut()]);
                if let Some(path) = path.$method() {
                    if let Some(str) = path.to_str() {
                        return try_cstring![str => std::ptr::null_mut()].into_raw();
                    }
                }
                std::ptr::null_mut()
            }
        )*
    };
    [$($name:ident$(,)*),*] => {
        path_methods![$($name:$name,)*];
    };
}

macro_rules! operations_between_files {
    [$($name:ident: $method:ident$(,)*),*] => {
        $(
            #[no_mangle]
            pub unsafe fn $name(old_path: *const c_char, new_path: *const c_char) -> bool {
                null_guard![old_path, new_path => false];
                let old = std::path::Path::new(try_str![old_path => false]);
                let new = std::path::Path::new(try_str![new_path => false]);
                std::fs::$method(old, new).is_ok()
            }
        )*
    };
}

macro_rules! get_path_operations {
    [$(($from:ident)$name:ident$(,)*),*] => {
        $(
            #[no_mangle]
            pub unsafe fn $name() -> *mut c_char {
                directories::$from::new()
                    .and_then(|dirs| dirs.$name().to_str().map(CString::new))
                    .map(|s| match s {
                        Ok(s) => s.into_raw(),
                        Err(_) => std::ptr::null_mut(),
                    })
                    .unwrap_or(std::ptr::null_mut())
            }
        )*
    };
}

path_info_functions![exists, is_dir, is_file, is_symlink, is_absolute, is_relative];
operations_between_files![rename_: rename, copy: copy];
path_methods![parent, file_name, extension];
get_path_operations![
    (BaseDirs) data_dir,
    (UserDirs) home_dir,
    (BaseDirs) config_dir,
];

#[no_mangle]
pub unsafe fn absolute(path: *const c_char) -> *mut c_char {
    null_guard![path => std::ptr::null_mut()];

    let path = std::path::Path::new(try_str![path => std::ptr::null_mut()]);

    if let Ok(path) = path.absolutize() {
        if let Some(str) = path.to_str() {
            return try_cstring![str => std::ptr::null_mut()].into_raw();
        }
    }

    std::ptr::null_mut()
}

#[no_mangle]
pub unsafe fn drop_cstring(c_str: *mut c_char) {
    null_guard![c_str => {}];

    let _ = CString::from_raw(c_str);
}

#[no_mangle]
pub unsafe fn project_dir(
    qualifier: *mut c_char,
    org: *mut c_char,
    app: *mut c_char,
) -> *mut c_char {
    null_guard![qualifier, org, app => std::ptr::null_mut()];

    let qua = try_str![qualifier => std::ptr::null_mut()];
    let org = try_str![org => std::ptr::null_mut()];
    let app = try_str![app => std::ptr::null_mut()];

    directories::ProjectDirs::from(qua, org, app)
        .and_then(|dirs| dirs.data_dir().to_str().map(CString::new))
        .map(|s| match s {
            Ok(s) => s.into_raw(),
            Err(_) => std::ptr::null_mut(),
        })
        .unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub unsafe fn remove_dir(path: *const c_char) -> bool {
    null_guard![path => false];

    let path = std::path::Path::new(try_str![path => false]);
    std::fs::remove_dir_all(path).is_ok()
}

#[no_mangle]
pub unsafe fn remove_file(path: *const c_char) -> bool {
    null_guard![path => false];

    let path = std::path::Path::new(try_str![path => false]);
    std::fs::remove_file(path).is_ok()
}

#[no_mangle]
pub unsafe fn create_dir(path: *const c_char) -> bool {
    null_guard![path => false];

    let path = std::path::Path::new(try_str![path => false]);
    std::fs::create_dir_all(path).is_ok()
}

#[no_mangle]
pub unsafe fn create_file(path: *const c_char) -> bool {
    null_guard![path => false];

    let path = std::path::Path::new(try_str![path => false]);
    std::fs::File::create(path).is_ok()
}

#[no_mangle]
pub unsafe fn get_file_size(path: *const c_char) -> u64 {
    null_guard![path => 0];

    let path = std::path::Path::new(try_str![path => 0]);
    match std::fs::metadata(path) {
        Ok(m) => m.len(),
        Err(_) => 0,
    }
}

#[no_mangle]
pub unsafe fn append(path: *mut c_char, rhs: *mut c_char) -> *mut c_char {
    null_guard![path, rhs => std::ptr::null_mut()];

    let path = std::path::Path::new(try_str![path => std::ptr::null_mut()]);
    let rhs = std::path::Path::new(try_str![rhs => std::ptr::null_mut()]);

    if let Some(s) = path.join(rhs).to_str() {
        return try_cstring![s => std::ptr::null_mut()].into_raw();
    }

    std::ptr::null_mut()
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
    null_guard![path => false];

    std::env::set_current_dir(std::path::Path::new(try_str![path => false])).is_ok()
}
