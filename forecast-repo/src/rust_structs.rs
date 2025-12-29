use std::collections::{BTreeMap, HashMap};

use serde::{Serialize, Deserialize};

use crate::{c_structs::{Coords, Sun, WxSingle}, wx_serialization::{in_hg::InHg, lunar_phases::LunarPhase, precipitation_type::PrecipitationType}};

#[derive(Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Wx {
    pub dewpoint: Vec<i16>,
    pub gust: Vec<u16>,
    pub lightning: Vec<u16>,
    pub new_precip: Vec<f64>,
    pub precip_rate: Vec<f64>,
    pub precip_type: Vec<PrecipitationType>,
    pub pressure: Vec<InHg>,
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
            dewpoint: vec![0; size],
            gust: vec![0; size],
            lightning: vec![0; size],
            new_precip: vec![0.0; size],
            precip_rate: vec![0.0; size],
            precip_type: vec![PrecipitationType::NoPrecipitation; size],
            pressure: vec![InHg(0.0); size],
            temperature: vec![0; size],
            total_cloud_cover: vec![0; size],
            total_precip: vec![0.0; size],
            total_snow: vec![0.0; size],
            vis: vec![0; size],
            wind_dir: vec![0; size],
            wind_spd: vec![0; size]
        }
    }

    pub fn add(&mut self, single: &WxSingle, at: usize) {
        self.dewpoint[at] = single.dewpoint;
        self.gust[at] = single.gust;
        self.lightning[at] = single.lightning;
        self.new_precip[at] = single.new_precip;
        self.precip_rate[at] = single.precip_rate;
        self.precip_type[at] = PrecipitationType::from(single.precip_type);
        self.pressure[at] = InHg(single.pressure);
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
    pub sun: BTreeMap<String, Sun>,
    pub wx: Wx
}

impl Location {
    pub fn new(size: usize) -> Location {
        Location { coords: Coords { lat: 0.0, lon: 0.0, x: 0, y: 0 }, is_city: false, sun: BTreeMap::new(), wx: Wx::new(size) }
    }
}

#[derive(Clone, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Forecast {
    pub forecast_times: Vec<u64>,
    pub locations: HashMap<String, Location>,
    pub phases: BTreeMap<String, LunarPhase>
}

impl Forecast {
    pub fn new() -> Self {
        Self { forecast_times: Vec::new(), locations: HashMap::new(), phases: BTreeMap::new() }
    }
}