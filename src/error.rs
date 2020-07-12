use std::fmt;

#[allow(dead_code)]

#[derive(Debug, Clone)]
pub enum DeliveryFailure {
    BadCoord,
    NoRoute,
    Other,
}

#[derive(Debug, Clone)]
pub struct RouteError {
    kind: DeliveryFailure,
}

impl RouteError {
    pub fn new(kind: DeliveryFailure) -> Self {
        RouteError {
            kind,
        }
    }
}

impl fmt::Display for RouteError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match &self.kind {
            DeliveryFailure::BadCoord => {
                writeln!(f, "One or more depot/delivery coordinates are invalid.")
            }
            DeliveryFailure::NoRoute => {
                writeln!(f, "No route can be found to deliver all items.")
            }
            DeliveryFailure::Other => {
                writeln!(f, "An unknown error has occured.")
            }
        }
    }
}
