use bitmask_enum::bitmask;
use serde::{Serialize, Deserialize, Serializer, Deserializer};
use serde::de::{Visitor, Error};
use std::fmt;
use std::str::FromStr;

const RAIN: &str = "rain";
const FREEZING_RAIN: &str = "ice";
const SNOW: &str = "snow";
const NO_PRECIPITATION: &str = "";

#[bitmask(u8)]
pub enum PrecipitationType {
    Rain,
    FreezingRain,
    Snow,
    NoPrecipitation = 0
}

impl fmt::Display for PrecipitationType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.contains(PrecipitationType::Snow) {
            write!(f, "{}", SNOW)
        }
        else if self.contains(PrecipitationType::FreezingRain) {
            write!(f, "{}", FREEZING_RAIN)
        }
        else if self.contains(PrecipitationType::Rain) {
            write!(f, "{}", RAIN)
        }
        else {
            write!(f, "{}", NO_PRECIPITATION)
        }
    }
}

impl FromStr for PrecipitationType {
    type Err = String;
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            RAIN => Ok(PrecipitationType::Rain),
            FREEZING_RAIN => Ok(PrecipitationType::FreezingRain),
            SNOW => Ok(PrecipitationType::Snow),
            _ => return Ok(PrecipitationType::NoPrecipitation)
        }
    }
}

impl Serialize for PrecipitationType {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where S: Serializer {
        serializer.serialize_str(&self.to_string())
    }
}

struct PrecipTypeVisitor;

impl<'de> Visitor<'de> for PrecipTypeVisitor {
    type Value = PrecipitationType;
    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_str("a precipitation type string")
    }
    fn visit_str<E>(self, value: &str) -> Result<PrecipitationType, E>
    where E: Error {
        value.parse().map_err(E::custom)
    }
}

impl<'de> Deserialize<'de> for PrecipitationType {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where D: Deserializer<'de> {
        deserializer.deserialize_str(PrecipTypeVisitor)
    }
}