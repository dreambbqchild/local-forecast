use std::{collections::HashMap, ffi::CString};
use std::ffi::CStr;
use std::os::raw::c_char;
use std::slice;
use std::sync::Mutex;
use once_cell::sync::Lazy;

use c_structs::{Coords, LabeledSun, Sun, WxSingle};
use rust_structs::{Forecast, Location, Moon};

pub mod c_structs;
pub mod rust_structs;
pub mod precipitation_type;

static FORECAST_DATA: Lazy<Mutex<HashMap<&str, Forecast>>> = Lazy::new(|| Mutex::new(HashMap::new()));

fn with_forecast<F>(forecast_c_str: *const c_char, callback: F) where F: FnOnce(&mut Forecast) {
    if forecast_c_str.is_null() {
        return;        
    }

    let c_str = unsafe { CStr::from_ptr(forecast_c_str) };
    let forecast_str = c_str.to_str().expect("To convert forecast to a string");

    let mut map = FORECAST_DATA.lock().unwrap();
    map.entry(forecast_str).and_modify(callback);
}

fn with_forecast_location<F>(forecast_c_str: *const c_char, location_c_str: *const c_char, callback: F) where F: FnOnce(&mut Location) {
    if location_c_str.is_null() {
        return;
    }

    with_forecast(forecast_c_str, |f| {
        let c_str = unsafe { CStr::from_ptr(location_c_str) };
        let location_str = c_str.to_str().expect("To convert location to a string").to_string();

        if !f.locations.contains_key(&location_str) {
            f.locations.insert(location_str.clone(), Location::new(f.forecast_times.len()));
        }

        let map = f.locations.get_mut(&location_str).expect("To get the location data");
        callback(map);
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_init_forecast(forecast_c_str: *const c_char) {
    let c_str = unsafe { CStr::from_ptr(forecast_c_str) };
    let forecast_str = c_str.to_str().expect("To convert forecast to a string");

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
        let mut c_str = unsafe { CStr::from_ptr(moon.name) };
        let name = c_str.to_str().expect("To convert name to a string").to_string();

        c_str = unsafe { CStr::from_ptr(moon.emoji) };
        let emoji = c_str.to_str().expect("To convert emoji to a string").to_string();

        f.moon.insert(day.to_string(), Moon{ age, name, emoji });
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_init_location(forecast_c_str: *const c_char, location_c_str: *const c_char, is_city: bool, coords_ptr: *const Coords) {
    with_forecast_location(forecast_c_str, location_c_str, |l| {
        let coords: &Coords = unsafe { &*coords_ptr };

        l.is_city = is_city;
        l.coords = coords.clone();
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_add_sun_for_location(forecast_c_str: *const c_char, location_c_str: *const c_char, day: i32, sun_ptr: *const Sun) {
    with_forecast_location(forecast_c_str, location_c_str, |l| {
        let sun: &Sun = unsafe { &*sun_ptr };
        l.sun.insert(day.to_string(), sun.clone());
    });
}

#[unsafe(no_mangle)]
pub extern "C" fn forecast_repo_add_weather_for_location(forecast_c_str: *const c_char, location_c_str: *const c_char, wx_ptr: *const WxSingle, len: usize) {
    let wx_slice: &[WxSingle] = unsafe {
        if wx_ptr.is_null() {
            &[]
        } else {
            slice::from_raw_parts(wx_ptr, len)
        }
    };

    with_forecast_location(forecast_c_str, location_c_str, |l| {
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