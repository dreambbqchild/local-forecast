use std::fs::File;
use std::io::{BufReader, BufWriter};
use std::path::Path;
use std::{collections::HashMap, ffi::CString};
use std::os::raw::c_char;
use std::slice;
use std::sync::Mutex;
use c_char_to_string::CCharToString;
use once_cell::sync::Lazy;

use c_structs::LabeledSun;
use rust_structs::{Forecast, Location, Moon};
use serde::de::value::Error;

pub mod c_structs;
pub mod c_char_to_string;
pub mod rust_structs;
pub mod precipitation_type;

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
    let f = serde_json::from_reader(reader)?;
    Ok(f)
}

fn safe_forecast_to_file<P: AsRef<Path>>(forecast: &Forecast, path: P) -> std::io::Result<()> {
    let file = File::open(path)?;
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
        _ => {
            false
        }
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
pub extern "C" fn forecast_repo_add_lunar_phase(forecast_c_str: *const c_char, day: i32, moon_ptr: *const c_structs::Moon) {
    with_forecast(forecast_c_str, |f| {
        let moon: &c_structs::Moon = unsafe { &*moon_ptr };

        let age = moon.age;
        let name = moon.name.to_string_safe();
        let emoji = moon.emoji.to_string_safe();

        f.moon.insert(day.to_string(), Moon{ age, name, emoji });
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_add_location(forecast_c_str: *const c_char, location_c_str: *const c_char, location_ptr: *const c_structs::Location) {
    with_forecast_location(forecast_c_str, location_c_str, |l| {
        let location = unsafe{&*location_ptr};
        let coords = unsafe{&*location.coords};

        l.coords = coords.clone();
        l.is_city = location.is_city;
        
        let sun_slice = to_slice(location.suns, location.suns_len);
        let wx_slice = to_slice(location.wx, location.wx_len);

        for labeled_sun in sun_slice.iter() {
            let sun = unsafe {&*labeled_sun.sun};
            l.sun.insert(labeled_sun.day.to_string(), sun.clone());
        }

        for (index, wx) in wx_slice.iter().enumerate() {
            l.wx.add(wx, index);
        }
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_get_forecast_length(forecast_c_str: *const c_char) -> usize {
    let mut result: usize = 0;
    with_forecast(forecast_c_str, |f| {
        result = f.forecast_times.len();
    });

    result
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_get_forecast_time_at(forecast_c_str: *const c_char, index: usize) -> u64 {
    let mut result: u64 = 0;
    with_forecast(forecast_c_str, |f| {
        result = f.forecast_times[index];
    });

    result
}

pub type MoonCallback = extern "C" fn(day: i32, moon: c_structs::Moon);
pub extern "C" fn forecast_repo_forecast_moon_on_day(forecast_c_str: *const c_char, day: i32, callback: MoonCallback) {
    with_forecast(forecast_c_str, |f| {
        match f.moon.get(&day.to_string()) {
            Some(moon) => {
                let name = CString::new(moon.name.as_str()).expect("Failed to create CString for name");
                let emoji = CString::new(moon.emoji.as_str()).expect("Failed to create CString for emoji");

                let c_moon = c_structs::Moon {
                    age: moon.age,
                    name: name.as_ptr(),
                    emoji: emoji.as_ptr()
                };

                callback(day, c_moon);
            },
            None => {}
        }
    });
}

pub type LocationCallback = extern "C" fn(key: *const c_char, location: c_structs::Location);
pub extern "C" fn forecast_repo_get_forecast_location(forecast_c_str: *const c_char, location_c_str: *const c_char, callback: LocationCallback) {
    with_forecast_location(forecast_c_str, location_c_str, |l|{
        let mut wx = Vec::new();
        for index in 0..l.wx.length() {
            wx.push(l.wx.extract(index));
        }
        
        let mut suns = Vec::new();
        for (day, sun) in l.sun.iter() {
            suns.push(LabeledSun {
                day: day.parse::<i32>().expect("To convert sun key to i32"),
                sun: sun
            });
        }
        
        let location = c_structs::Location {
            coords: &l.coords,
            is_city: l.is_city,
            wx: wx.as_ptr(),
            suns: suns.as_ptr(),
            suns_len: suns.len(),
            wx_len: wx.len()
        };

        callback(location_c_str, location);
    });
}