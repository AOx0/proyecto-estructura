#![allow(non_camel_case_types)]

use serde::{Deserialize, Serialize};

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
    u64 = 3,
    i8 = 4,
    i16 = 5,
    i64 = 6,
    f32 = 7,
    f64 = 8,
    str = 9,
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

make_serializers!( su8;u8, su16;u16, su32;u32, su64;u64, si8;i8, si16;i16, si32;i32, si64;i64, sf64;f64 );
make_deserializers!( du8;u8, du16;u16, du32;u32, du64;u64, di8;i8, di16;i16, di32;i32, di64;i64, df64;f64 );

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
