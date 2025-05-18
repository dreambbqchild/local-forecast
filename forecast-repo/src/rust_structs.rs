use std::collections::HashMap;

use serde::{Serialize, Deserialize};

use crate::{c_structs::{Coords, Sun, WxSingle}, wx_enums::{moon_phases::MoonPhase, precipitation_type::PrecipitationType}};

#[derive(Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Wx {
    pub dewpoint: Vec<i16>,
    pub gust: Vec<u16>,
    pub lightning: Vec<u16>,
    pub new_precip: Vec<f64>,
    pub precip_rate: Vec<f64>,
    pub precip_type: Vec<PrecipitationType>,
    pub pressure: Vec<f64>,
    pub temperature: Vec<i16>,
    pub total_cloud_cover: Vec<u16>,
    pub total_precip: Vec<f64>,
    pub total_snow: Vec<f64>,
    pub vis: Vec<u16>,
    pub wind_dir: Vec<u16>,
    pub wind_spd: Vec<u16>
}

impl Wx {
    pub fn new(size: usize) -> Self {
        Self {
            dewpoint: Vec::with_capacity(size),
            gust: Vec::with_capacity(size),
            lightning: Vec::with_capacity(size),
            new_precip: Vec::with_capacity(size),
            precip_rate: Vec::with_capacity(size),
            precip_type: Vec::with_capacity(size),
            pressure: Vec::with_capacity(size),
            temperature: Vec::with_capacity(size),
            total_cloud_cover: Vec::with_capacity(size),
            total_precip: Vec::with_capacity(size),
            total_snow: Vec::with_capacity(size),
            vis: Vec::with_capacity(size),
            wind_dir: Vec::with_capacity(size),
            wind_spd: Vec::with_capacity(size)
        }
    }

    pub fn add(&mut self, single: &WxSingle, at: usize) {
        self.dewpoint[at] = single.dewpoint;
        self.gust[at] = single.gust;
        self.lightning[at] = single.lightning;
        self.new_precip[at] = single.new_precip;
        self.precip_rate[at] = single.precip_rate;
        self.precip_type[at] = PrecipitationType::from(single.precip_type);
        self.pressure[at] = single.pressure;
        self.temperature[at] = single.temperature;
        self.total_cloud_cover[at] = single.total_cloud_cover;
        self.total_precip[at] = single.total_precip;
        self.total_snow[at] = single.total_snow;
        self.vis[at] = single.vis;
        self.wind_dir[at] = single.wind_dir;
        self.wind_spd[at] = single.wind_spd
    }

    pub fn length(&self) -> usize {
        self.dewpoint.len()
    }
}

#[derive(Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Location {
    pub coords: Coords,
    pub is_city: bool,
    pub sun: HashMap<String, Sun>,
    pub wx: Wx
}

impl Location {
    pub fn new(size: usize) -> Location {
        Location { coords: Coords { lat: 0.0, lon: 0.0, x: 0, y: 0 }, is_city: false, sun: HashMap::new(), wx: Wx::new(size) }
    }
}

#[derive(Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Moon {
    pub phase: MoonPhase
}

#[derive(Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Forecast {
    pub forecast_times: Vec<u64>,
    pub locations: HashMap<String, Location>,
    pub moon: HashMap<String, Moon>
}

impl Forecast {
    pub fn new() -> Self {
        Self { forecast_times: Vec::new(), locations: HashMap::new(), moon: HashMap::new() }
    }
}