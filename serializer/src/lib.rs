#![allow(non_camel_case_types)]

use serde::{Deserialize, Serialize};
use std::ffi::CStr;

#[repr(C)]
pub struct DynArray {
    array: *mut u8,
    length: u64,
}

#[repr(C)]
#[derive(PartialEq, Debug, Serialize, Deserialize)]
pub enum ColumnType {
    u8 = 1,
    u16 = 2,
    u32 = 11,
    u64 = 3,
    i8 = 4,
    i16 = 5,
    i32 = 10,
    i64 = 6,
    f32 = 7,
    f64 = 8,
    str = 9,
    rbool = 12,
}

#[repr(C)]
#[derive(PartialEq, Debug, Serialize, Deserialize)]
pub struct Layout {
    size: u64,
    optional: bool,
    r#type: ColumnType,
}

#[no_mangle]
pub unsafe extern "C" fn serialize_layout(arg: Layout) -> DynArray {
    let mut result: Vec<u8> = bincode::serialize(&arg).unwrap();

    let r = DynArray {
        array: result.as_mut_ptr(),
        length: result.len() as u64,
    };

    std::mem::forget(result);

    return r;
}

macro_rules! make_serializers {
    ($($name:ident;$ty:tt),*) => {
        $(#[no_mangle]
        pub unsafe extern "C" fn $name(arg: $ty) -> DynArray {
            let mut result: Vec<u8> = bincode::serialize(&arg).unwrap();

            let r = DynArray {
                array: result.as_mut_ptr(),
                length: result.len() as u64,
            };

            std::mem::forget(result);

            return r;
        })*
    };
}

macro_rules! make_deserializers {
    ($($name:ident;$ty:tt),*) => {
        $(#[no_mangle]
        pub unsafe extern "C" fn $name(array: *mut u8, size: u64) -> $ty {
            let contenidos = {
                assert!(!array.is_null());
                std::slice::from_raw_parts(array, size as usize)
            };

            let result: $ty = bincode::deserialize(contenidos).unwrap();

            return result;
        })*
    };
}

make_serializers!( su8;u8, su16;u16, su32;u32, su64;u64, si8;i8, si16;i16, si32;i32, si64;i64, sf64;f64, sbool;bool );
make_deserializers!( du8;u8, du16;u16, du32;u32, du64;u64, di8;i8, di16;i16, di32;i32, di64;i64, df64;f64, dbool;bool );

#[no_mangle]
pub unsafe extern "C" fn drop_dyn_array(arg: DynArray) {
    Vec::from_raw_parts(arg.array, arg.length as usize, arg.length as usize);
}

#[no_mangle]
pub unsafe extern "C" fn deserialize_layout(array: *mut u8, size: u64) -> Layout {
    let contenidos = {
        assert!(!array.is_null());
        std::slice::from_raw_parts(array, size as usize)
    };

    let result: Layout = bincode::deserialize(contenidos).unwrap();

    return result;
}

#[no_mangle]
pub unsafe extern "C" fn serialize_str(arg: *const std::ffi::c_char, size: u64) -> DynArray {
    let chars = CStr::from_ptr(arg).to_bytes().to_owned();

    // Push 0x0 while chars size is smaller than size
    let chars = chars
        .into_iter()
        .chain(std::iter::repeat(0x0))
        .take(size as usize)
        .collect::<Vec<u8>>();

    let mut result: Vec<u8> = bincode::serialize(&chars).unwrap();

    let r = DynArray {
        array: result.as_mut_ptr(),
        length: result.len() as u64,
    };

    std::mem::forget(result);

    return r;
}

#[no_mangle]
pub unsafe extern "C" fn deserialize_str(array: *mut u8, size: u64) -> *mut std::ffi::c_char {
    let contenidos = {
        assert!(!array.is_null());
        std::slice::from_raw_parts(array, size as usize)
    };

    let result: Vec<u8> = bincode::deserialize(contenidos).unwrap();
    let mut result_with_one_null = vec![];

    for value in result {
        if value == 0x0 {
            break;
        }

        result_with_one_null.push(value);
    }

    let r = std::ffi::CString::new(result_with_one_null)
        .unwrap()
        .into_raw();

    return r;
}

#[no_mangle]
pub unsafe extern "C" fn drop_str(arg: *mut std::ffi::c_char) {
    let _ = std::ffi::CString::from_raw(arg);
}

#[cfg(test)]
mod test {
    use super::*;
    #[test]
    pub fn str_serialize() {
        let string = std::ffi::CString::new(b"foo".to_vec()).expect("CString::new failed");

        unsafe {
            let serialized = serialize_str(string.as_ptr() as *const std::ffi::c_char, 150);
            let cmp = deserialize_str(serialized.array, serialized.length);
            let result = std::ffi::CStr::from_ptr(cmp).to_str().unwrap();

            let original = string.to_str().unwrap();

            assert_eq!(result, original);
        }
    }
}
