use std::fs::File;
use std::io::{BufReader, BufWriter};
use std::path::Path;
use std::collections::HashMap;
use std::os::raw::c_char;
use std::{ptr, slice};
use std::sync::Mutex;
use interop::c_structs;
use interop::string_tools::CCharToString;
use wx_enums::lunar_phases::LunarPhase;
use once_cell::sync::Lazy;

use rust_structs::{Forecast, Location};

pub mod interop;
pub mod rust_structs;
pub mod wx_enums;

static FORECAST_DATA: Lazy<Mutex<HashMap<String, Forecast>>> = Lazy::new(|| Mutex::new(HashMap::new()));

fn with_forecast<F>(forecast_c_str: *const c_char, callback: F) where F: FnOnce(&mut Forecast) {
    if forecast_c_str.is_null() {
        return;        
    }

    let forecast_str = forecast_c_str.to_string_safe();

    let mut map = FORECAST_DATA.lock().unwrap();
    map.entry(forecast_str).and_modify(callback);
}

fn with_forecast_location<F>(forecast_c_str: *const c_char, location_c_str: *const c_char, callback: F) where F: FnOnce(&mut Location) {
    if location_c_str.is_null() {
        return;
    }

    with_forecast(forecast_c_str, |f| {
        let location_str = location_c_str.to_string_safe();

        if !f.locations.contains_key(&location_str) {
            f.locations.insert(location_str.clone(), Location::new(f.forecast_times.len()));
        }

        let map = f.locations.get_mut(&location_str).expect("To get the location data");
        callback(map);
    });
}

fn to_slice<'a, T>(ptr: *const T, len: usize) -> &'a [T] {
    unsafe {
        if ptr.is_null() {
            &[]
        } else {
            slice::from_raw_parts(ptr, len)
        }
    }
}

fn load_forecast_from_file<P: AsRef<Path>>(path: P) -> std::io::Result<Forecast> {
    let file = File::open(path)?;
    let reader = BufReader::new(file);
    let f = serde_json::from_reader(reader).expect("to load the json");
    Ok(f)
}

fn safe_forecast_to_file<P: AsRef<Path>>(forecast: &Forecast, path: P) -> std::io::Result<()> {
    let file = File::create(path)?;
    let writer = BufWriter::new(file);
    serde_json::to_writer(writer, forecast)?;
    Ok(())
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_load_forecast(forecast_c_str: *const c_char, path_c_str: *const c_char) -> bool {
    let forecast_str = forecast_c_str.to_string_safe();
    match load_forecast_from_file(path_c_str.to_str_safe()) {
        Ok(forecast) => {
            let mut map = FORECAST_DATA.lock().unwrap();
            map.insert(forecast_str, forecast);
            true
        },
        _ => false 
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_save_forecast(forecast_c_str: *const c_char, path_c_str: *const c_char) {
    with_forecast(forecast_c_str, |f| {
        safe_forecast_to_file(f, path_c_str.to_str_safe()).expect("To save the updated json");
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_init_forecast(forecast_c_str: *const c_char) {
    let forecast_str = forecast_c_str.to_string_safe();

    let mut map = FORECAST_DATA.lock().unwrap();
    map.insert(forecast_str, Forecast::new());
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_add_forecast_start_time(forecast_c_str: *const c_char, forecast_time: u64) {
    with_forecast(forecast_c_str, |f| f.forecast_times.push(forecast_time));
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_add_lunar_phase(forecast_c_str: *const c_char, day: i32, lunar_phase: u8) {
    with_forecast(forecast_c_str, |f| {
        let phase = LunarPhase::try_from(lunar_phase).unwrap();
        f.phases.insert(format!("{:02}", day), phase );
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_add_location(forecast_c_str: *const c_char, location_c_str: *const c_char, location_ptr: *const c_structs::Location) {
    with_forecast_location(forecast_c_str, location_c_str, |l| {
        let location = unsafe{&*location_ptr};

        l.coords = location.coords.clone();
        l.is_city = location.is_city;
        
        let sun_slice = to_slice(location.suns, location.suns_len);
        let wx_slice = to_slice(location.wx, location.wx_len);

        for labeled_sun in sun_slice.iter() {
            l.sun.insert(format!("{:02}", labeled_sun.day), labeled_sun.sun.clone());
        }

        for (index, wx) in wx_slice.iter().enumerate() {
            l.wx.add(wx, index);
        }
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_get_forecast(forecast_c_str: *const c_char) -> *mut c_structs::Forecast {
    let mut has_result = false;
    let mut result = c_structs::Forecast {now: 0, forecast_times: ptr::null_mut(), forecast_times_len: 0, locations: ptr::null_mut(), locations_len: 0, phases: ptr::null_mut(), phases_len: 0 };
    with_forecast(forecast_c_str, |f| {        
        has_result = true;
        result = c_structs::Forecast::from(f);
    });

    if has_result { Box::into_raw(Box::new(result)) } else { ptr::null_mut() }
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_free_forecast(forecast: *mut c_structs::Forecast) {
    if forecast.is_null() {
        return;
    }
    unsafe { drop(Box::from_raw(forecast)); }
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_free(forecast_c_str: *const c_char) {
    if forecast_c_str.is_null() {
        return;
    }

    let forecast_str = forecast_c_str.to_string_safe();

    let mut map = FORECAST_DATA.lock().unwrap();
    map.remove(&forecast_str);
}