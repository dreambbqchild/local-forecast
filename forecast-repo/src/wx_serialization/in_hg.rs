use serde::{Serialize, Serializer, Deserialize, Deserializer};
use serde::de::{self, Visitor};
use std::fmt;

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct InHg(pub f64);

impl Serialize for InHg {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where S: Serializer
    {        
        let rounded = (self.0 * 100.0).round() / 100.0;
        serializer.serialize_f64(rounded)
    }
}

impl<'de> Deserialize<'de> for InHg {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct InHgVisitor;

        impl<'de> Visitor<'de> for InHgVisitor {
            type Value = InHg;

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                formatter.write_str("a numeric value")
            }

            fn visit_f64<E>(self, v: f64) -> Result<Self::Value, E>
            where E: de::Error
            {
                Ok(InHg(v))
            }

            fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
            where E: de::Error
            {
                Ok(InHg(v as f64))
            }

            fn visit_i64<E>(self, v: i64) -> Result<Self::Value, E>
            where E: de::Error
            {
                Ok(InHg(v as f64))
            }
        }

        deserializer.deserialize_any(InHgVisitor)
    }
}
