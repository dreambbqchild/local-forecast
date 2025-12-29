use serde::{Serialize, Deserialize, Serializer, Deserializer};
use serde::de::{Visitor, Error};
use std::fmt;
use std::str::FromStr;

const NEW_LABEL: &str = "New";
const WAXING_CRESCENT_LABEL: &str = "Waxing Crescent";
const FIRST_QUARTER_LABEL: &str = "First Quarter";
const WAXING_GIBBOUS_LABEL: &str = "Waxing Gibbous";

const WOLF_MOON_LABEL: &str = "Wolf Moon";
const SNOW_MOON_LABEL: &str = "Snow Moon";
const PINK_MOON_LABEL: &str = "Pink Moon";
const WORM_MOON_LABEL: &str = "Worm Moon";
const FLOWER_MOON_LABEL: &str = "Flower Moon";
const STRAWBERRY_MOON_LABEL: &str = "Strawberry Moon";
const BUCK_MOON_LABEL: &str = "Buck Moon";
const STURGEON_MOON_LABEL: &str = "Sturgeon Moon";
const CORN_MOON_LABEL: &str = "Corn Moon";
const HUNTER_MOON_LABEL: &str = "Hunter Moon";
const BEAVER_MOON_LABEL: &str = "Beaver Moon";
const COLD_MOON_LABEL: &str = "Cold Moon";
const BLUE_MOON_LABEL: &str = "Blue Moon";

const HARVEST_MOON_LABEL: &str = "Harvest Moon";
const WANING_GIBBOUS_LABEL: &str = "Waning Gibbous";
const LAST_QUARTER_LABEL: &str = "Last Quarter";
const WANING_CRESCENT_LABEL: &str = "Waning Crescent";

#[repr(u8)]
#[derive(Clone)]
pub enum LunarPhase {
    New,
    WaxingCrescent,
    FirstQuarter,
    WaxingGibbous,
    WolfMoon,
    SnowMoon,
    WormMoon,
    PinkMoon,
    FlowerMoon,
    StrawberryMoon,
    BuckMoon,
    SturgeonMoon,
    CornMoon,
    HunterMoon,
    BeaverMoon,
    ColdMoon,
    BlueMoon,
    HarvestMoon,
    WaningGibbous,
    LastQuarter,
    WaningCrescent
}

impl TryFrom<u8> for LunarPhase {
    type Error = &'static str;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(LunarPhase::New),
            1 => Ok(LunarPhase::WaxingCrescent),
            2 => Ok(LunarPhase::FirstQuarter),
            3 => Ok(LunarPhase::WaxingGibbous),
            4 => Ok(LunarPhase::WolfMoon),
            5 => Ok(LunarPhase::SnowMoon),
            6 => Ok(LunarPhase::WormMoon),
            7 => Ok(LunarPhase::PinkMoon),
            8 => Ok(LunarPhase::FlowerMoon),
            9 => Ok(LunarPhase::StrawberryMoon),
            10 => Ok(LunarPhase::BuckMoon),
            11 => Ok(LunarPhase::SturgeonMoon),
            12 => Ok(LunarPhase::CornMoon),
            13 => Ok(LunarPhase::HunterMoon),
            14 => Ok(LunarPhase::BeaverMoon),
            15 => Ok(LunarPhase::ColdMoon),
            16 => Ok(LunarPhase::BlueMoon),
            17 => Ok(LunarPhase::HarvestMoon),
            18 => Ok(LunarPhase::WaningGibbous),
            19 => Ok(LunarPhase::LastQuarter),
            20 => Ok(LunarPhase::WaningCrescent),
            _ => Err("Invalid value for LunarPhase"),
        }
    }
}

impl fmt::Display for LunarPhase {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let result = match self {
            LunarPhase::New => NEW_LABEL,
            LunarPhase::WaxingCrescent =>  WAXING_CRESCENT_LABEL,
            LunarPhase::FirstQuarter => FIRST_QUARTER_LABEL,
            LunarPhase::WaxingGibbous => WAXING_GIBBOUS_LABEL,
            LunarPhase::WolfMoon => WOLF_MOON_LABEL,
            LunarPhase::SnowMoon => SNOW_MOON_LABEL,
            LunarPhase::WormMoon => WORM_MOON_LABEL,
            LunarPhase::PinkMoon => PINK_MOON_LABEL,
            LunarPhase::FlowerMoon => FLOWER_MOON_LABEL,
            LunarPhase::StrawberryMoon => STRAWBERRY_MOON_LABEL,
            LunarPhase::BuckMoon => BUCK_MOON_LABEL,
            LunarPhase::SturgeonMoon => STURGEON_MOON_LABEL,
            LunarPhase::CornMoon => CORN_MOON_LABEL,
            LunarPhase::HunterMoon => HUNTER_MOON_LABEL,
            LunarPhase::BeaverMoon => BEAVER_MOON_LABEL,
            LunarPhase::ColdMoon => COLD_MOON_LABEL,
            LunarPhase::BlueMoon => BLUE_MOON_LABEL,
            LunarPhase::HarvestMoon => HARVEST_MOON_LABEL,
            LunarPhase::WaningGibbous => WANING_GIBBOUS_LABEL,
            LunarPhase::LastQuarter => LAST_QUARTER_LABEL,
            LunarPhase::WaningCrescent => WANING_CRESCENT_LABEL
        };

        write!(f, "{}", result)
    }
}

impl FromStr for LunarPhase {
    type Err = String;
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            NEW_LABEL => Ok(LunarPhase::New),
            WAXING_CRESCENT_LABEL => Ok(LunarPhase::WaxingCrescent),
            FIRST_QUARTER_LABEL => Ok(LunarPhase::FirstQuarter),
            WAXING_GIBBOUS_LABEL => Ok(LunarPhase::WaxingGibbous),
            WOLF_MOON_LABEL => Ok(LunarPhase::WolfMoon),
            SNOW_MOON_LABEL => Ok(LunarPhase::SnowMoon),
            WORM_MOON_LABEL => Ok(LunarPhase::WormMoon),
            PINK_MOON_LABEL => Ok(LunarPhase::PinkMoon),
            FLOWER_MOON_LABEL => Ok(LunarPhase::FlowerMoon),
            STRAWBERRY_MOON_LABEL => Ok(LunarPhase::StrawberryMoon),
            BUCK_MOON_LABEL => Ok(LunarPhase::BuckMoon),
            STURGEON_MOON_LABEL => Ok(LunarPhase::SturgeonMoon),
            CORN_MOON_LABEL => Ok(LunarPhase::CornMoon),
            HUNTER_MOON_LABEL => Ok(LunarPhase::HunterMoon),
            BEAVER_MOON_LABEL => Ok(LunarPhase::BeaverMoon),
            COLD_MOON_LABEL => Ok(LunarPhase::ColdMoon),
            BLUE_MOON_LABEL => Ok(LunarPhase::BlueMoon),
            HARVEST_MOON_LABEL => Ok(LunarPhase::HarvestMoon),
            WANING_GIBBOUS_LABEL => Ok(LunarPhase::WaningGibbous),
            LAST_QUARTER_LABEL => Ok(LunarPhase::LastQuarter),
            WANING_CRESCENT_LABEL => Ok(LunarPhase::WaningCrescent),
            _ => Err(String::from("Invalid Value"))
        }
    }
}

impl Serialize for LunarPhase {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where S: Serializer {
        serializer.serialize_str(&self.to_string())
    }
}

struct MoonPhasesVisitor;

impl<'de> Visitor<'de> for MoonPhasesVisitor {
    type Value = LunarPhase;
    fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        formatter.write_str("a MoonPhases string")
    }
    fn visit_str<E>(self, value: &str) -> Result<LunarPhase, E>
    where E: Error {
        value.parse().map_err(E::custom)
    }
}

impl<'de> Deserialize<'de> for LunarPhase {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where D: Deserializer<'de> {
        deserializer.deserialize_str(MoonPhasesVisitor)
    }
}