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

#[no_mangle]
pub unsafe extern "C" fn deserialize_layout(array: *mut u8, size: u64) -> Layout {
    let contenidos = {
        assert!(!array.is_null());
        std::slice::from_raw_parts(array, size as usize)
    };

    let result: Layout = bincode::deserialize(contenidos).unwrap();

    return result;
}
