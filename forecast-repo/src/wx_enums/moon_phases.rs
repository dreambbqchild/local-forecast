use serde::{Serialize, Deserialize, Serializer, Deserializer};
use serde::de::{Visitor, Error};
use std::fmt;
use std::str::FromStr;

const NEW_LABEL: &str = "New";
const WAXING_CRESCENT_LABEL: &str = "Waxing Crescent";
const FIRST_QUARTER_LABEL: &str = "First Quarter";
const WAXING_GIBBOUS_LABEL: &str = "Waxing Gibbous";
const FULL_LABEL: &str = "Full";
const WANING_GIBBOUS_LABEL: &str = "Waning Gibbous";
const LAST_QUARTER_LABEL: &str = "Last Quarter";
const WANING_CRESCENT_LABEL: &str = "Waning Crescent";

#[repr(u8)]
#[derive(Clone)]
pub enum MoonPhase {
    New,
    WaxingCrescent,
    FirstQuarter,
    WaxingGibbous,
    Full,
    WaningGibbous,
    LastQuarter,
    WaningCrescent
}

impl TryFrom<u8> for MoonPhase {
    type Error = &'static str;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(MoonPhase::New),
            1 => Ok(MoonPhase::WaxingCrescent),
            2 => Ok(MoonPhase::FirstQuarter),
            3 => Ok(MoonPhase::WaxingGibbous),
            4 => Ok(MoonPhase::Full),
            5 => Ok(MoonPhase::WaningGibbous),
            6 => Ok(MoonPhase::LastQuarter),
            7 => Ok(MoonPhase::WaningCrescent),
            _ => Err("Invalid value for MoonPhase"),
        }
    }
}

impl fmt::Display for MoonPhase {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let result = match self {
            MoonPhase::New => NEW_LABEL,
            MoonPhase::WaxingCrescent =>  WAXING_CRESCENT_LABEL,
            MoonPhase::FirstQuarter => FIRST_QUARTER_LABEL,
            MoonPhase::WaxingGibbous => WAXING_GIBBOUS_LABEL,
            MoonPhase::Full => FULL_LABEL,
            MoonPhase::WaningGibbous => WANING_GIBBOUS_LABEL,
            MoonPhase::LastQuarter => LAST_QUARTER_LABEL,
            MoonPhase::WaningCrescent => WANING_CRESCENT_LABEL
        };

        write!(f, "{}", result)
    }
}

impl FromStr for MoonPhase {
    type Err = String;
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            NEW_LABEL => Ok(MoonPhase::New),
            WAXING_CRESCENT_LABEL => Ok(MoonPhase::WaxingCrescent),
            FIRST_QUARTER_LABEL => Ok(MoonPhase::FirstQuarter),
            WAXING_GIBBOUS_LABEL => Ok(MoonPhase::WaxingGibbous),
            FULL_LABEL => Ok(MoonPhase::Full),
            WANING_GIBBOUS_LABEL => Ok(MoonPhase::WaningGibbous),
            LAST_QUARTER_LABEL => Ok(MoonPhase::LastQuarter),
            WANING_CRESCENT_LABEL => Ok(MoonPhase::WaningCrescent),
            _ => Err(String::from("Invalid Value"))
        }
    }
}

impl Serialize for MoonPhase {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where S: Serializer {
        serializer.serialize_str(&self.to_string())
    }
}

struct MoonPhasesVisitor;

impl<'de> Visitor<'de> for MoonPhasesVisitor {
    type Value = MoonPhase;
    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_str("a MoonPhases string")
    }
    fn visit_str<E>(self, value: &str) -> Result<MoonPhase, E>
    where E: Error {
        value.parse().map_err(E::custom)
    }
}

impl<'de> Deserialize<'de> for MoonPhase {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where D: Deserializer<'de> {
        deserializer.deserialize_str(MoonPhasesVisitor)
    }
}