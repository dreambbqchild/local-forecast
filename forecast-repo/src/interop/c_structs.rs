use std::{ffi::c_char, slice::from_raw_parts_mut, u8, time::SystemTime};
use serde::{Deserialize, Serialize};

use crate::{interop::string_tools::ToByteString, rust_structs, wx_enums::{precipitation_type::PrecipitationType}};

use super::c::{copy_c_array, create_c_array, free_c_array};

#[repr(C)]
pub struct WxSingle {
    pub dewpoint: i16,
    pub gust: u16,
    pub lightning: u16,
    pub new_precip: f64,
    pub precip_rate: f64,
    pub precip_type: u8,
    pub pressure: f64,
    pub temperature: i16,
    pub total_cloud_cover: u16,
    pub total_precip: f64,
    pub total_snow: f64,
    pub vis: u16,
    pub wind_dir: u16,
    pub wind_spd: u16
}

impl WxSingle {
    pub fn from(wx: &rust_structs::Wx, at: usize) -> WxSingle {
        WxSingle { 
            dewpoint: wx.dewpoint[at],
            gust: wx.gust[at],
            lightning: wx.lightning[at],
            new_precip: wx.new_precip[at],
            precip_rate: wx.precip_rate[at],
            precip_type: PrecipitationType::bits(&wx.precip_type[at]),
            pressure: wx.pressure[at],
            temperature: wx.temperature[at],
            total_cloud_cover: wx.total_cloud_cover[at],
            total_precip: wx.total_precip[at],
            total_snow: wx.total_snow[at],
            vis: wx.vis[at],
            wind_dir: wx.wind_dir[at],
            wind_spd: wx.wind_spd[at]
        }
    }
}

#[repr(C)]
#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Coords {
    pub lat: f64,
    pub lon: f64,
    pub x: u16,
    pub y: u16
}

#[repr(C)]
#[derive(Default, Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Sun {
    pub rise: u64,
    pub set: u64
}

#[repr(C)]
pub struct LabeledSun {
    pub day: i32,
    pub sun: Sun
}

#[repr(C)]
pub struct LabeledLunarPhase {
    pub day: i32,
    pub phase: u8
}

#[repr(C)]
pub struct Location {
    pub coords: Coords,
    pub is_city: bool,
    pub suns: *mut LabeledSun,
    pub suns_len: usize,
    pub wx: *mut WxSingle,
    pub wx_len: usize
}

impl Location {
    pub fn from(l: &rust_structs::Location) -> Self {
        let coords = l.coords.clone();
        let is_city = l.is_city;
        let suns = create_c_array::<LabeledSun>(l.sun.len());
        let suns_len = l.sun.len();
        let wx = create_c_array::<WxSingle>(l.wx.length());
        let wx_len = l.wx.length();

        for (sun_index, (key, s)) in l.sun.iter().enumerate() {
            unsafe {
                let day = key.parse::<i32>().unwrap();
                let sun = s.clone();
                *suns.add(sun_index) = LabeledSun { day, sun };
            };
        }

        for wx_index in 0..l.wx.length() {
            unsafe {
                *wx.add(wx_index) = WxSingle::from(&l.wx, wx_index);
            };
        }

        Location { coords, is_city, suns, suns_len, wx, wx_len }
    }

    pub fn free(&mut self) {
        free_c_array(self.suns);
        free_c_array(self.wx);
    }
}

#[repr(C)]
pub struct LabeledLocation {
    pub key: *mut c_char,
    pub location: Location,
}

impl LabeledLocation {
    pub fn from<S: AsRef<str>>(l: &rust_structs::Location, str_key: &S) -> Self {
        let key = str_key.allocate_c_cstring_copy();
        let location = Location::from(l);
        LabeledLocation { key, location }
    }

    pub fn free(&mut self) {
        free_c_array(self.key);
        self.location.free();
    }
}

#[repr(C)]
pub struct Forecast {
    pub now: u64,
    pub forecast_times: *mut u64,
    pub forecast_times_len: usize,
    pub locations: *mut LabeledLocation,
    pub locations_len: usize,
    pub phases: *mut LabeledLunarPhase,
    pub phases_len: usize
}

impl Forecast {
    pub fn from(f: &rust_structs::Forecast) -> Self {
        let time = SystemTime::now().duration_since(SystemTime::UNIX_EPOCH).unwrap();

        let forecast_times = create_c_array::<u64>(f.forecast_times.len());
        let forecast_times_len = f.forecast_times.len();
        copy_c_array(forecast_times, f.forecast_times.as_ptr(), f.forecast_times.len());
        
        let locations = create_c_array::<LabeledLocation>(f.locations.len());
        let locations_len = f.locations.len();
        for (location_index, (key, l)) in f.locations.iter().enumerate() {
            unsafe {*locations.add(location_index) = LabeledLocation::from(l, key) };
        }

        let phases = create_c_array::<LabeledLunarPhase>(f.phases.len());
        let phases_len= f.phases.len();

        for (moon_index, (key, p)) in f.phases.iter().enumerate() {
            unsafe {*phases.add(moon_index) = LabeledLunarPhase {
                    day: key.parse::<i32>().unwrap(),
                    phase: p.clone() as u8
                };
            }
        }

        Forecast { now: time.as_secs(), forecast_times, forecast_times_len, locations, locations_len, phases, phases_len }    
    }

    pub fn free(&mut self) {
        free_c_array(self.forecast_times);

        let locations_slice = unsafe { from_raw_parts_mut(self.locations, self.locations_len) };

        for location in locations_slice {
            location.free();
        }

        free_c_array(self.phases);
        free_c_array(self.locations);
    }
}