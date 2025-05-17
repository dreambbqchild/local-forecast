use std::ffi::{CStr, CString};
use std::os::raw::c_char;

use crate::interop::c::{copy_c_array, create_c_array};

pub trait CCharToString {
    fn to_string_safe(self) -> String;
    fn to_str_safe<'a>(self) -> &'a str;
}

impl CCharToString for *const c_char {
    fn to_string_safe(self) -> String {
        if self.is_null() {
            return String::new();
        }

        unsafe {
            CStr::from_ptr(self)
                .to_string_lossy()
                .into_owned()
        }
    }

    fn to_str_safe<'a>(self) -> &'a str {
        if self.is_null() {
            return "";
        }

        unsafe  {
            CStr::from_ptr(self).to_str().unwrap()
        }
    }
}

pub trait ToByteString {
    fn allocate_c_cstring_copy(self) -> *mut c_char;
}

impl<T: AsRef<str>> ToByteString for T {
    fn allocate_c_cstring_copy(self) -> *mut c_char {
        let c_str = CString::new(self.as_ref()).unwrap();
        let bytes = c_str.as_bytes_with_nul();
        let c_array = create_c_array::<c_char>(bytes.len());

        copy_c_array(c_array, c_str.as_ptr(), bytes.len())
    }
}