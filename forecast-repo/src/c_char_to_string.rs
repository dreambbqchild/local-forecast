use std::ffi::{CStr, CString};
use std::os::raw::c_char;

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
