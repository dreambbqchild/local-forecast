
use libc::{c_void, calloc, free, memcpy};
use std::mem;

pub fn create_c_array<T>(num_elements: usize) -> *mut T {
    let size = mem::size_of::<T>();

    let ptr = unsafe {calloc(num_elements, size) as *mut T};

    if ptr.is_null() {
        panic!("Failed to allocate memory via calloc.");
    }
    ptr
}

pub fn copy_c_array<T>(dest: *mut T,  src: *const T, num_elements: usize) -> *mut T {
    unsafe { memcpy(dest as *mut c_void, src as *const c_void, num_elements * mem::size_of::<T>()) };
    dest
}

pub fn free_c_array<T>(ptr: *mut T) {
    unsafe { free(ptr as *mut c_void) } ;
}