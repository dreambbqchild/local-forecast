use serde::{Deserialize, Serialize};

#[repr(C)]
pub struct WxSingle {
    pub dewpoint: i16,
    pub gust: i16,
    pub lightning: i16,
    pub new_precip: f64,
    pub precip_rate: f64,
    pub precip_type: u8,
    pub pressure: f64,
    pub temperature: i16,
    pub total_cloud_cover: i16,
    pub total_precip: f64,
    pub total_snow: f64,
    pub vis: i16,
    pub wind_dir: i16,
    pub wind_spd: i16
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