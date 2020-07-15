#![allow(dead_code)]
use std::cmp::Ordering;
use std::hash::{Hash, Hasher};

const MILES_PER_KM: f64 = 1. / 1.609344;
const EARTH_RADIUS_KM: f64 = 6371.0;

pub enum DeliveryResult {
    DeliverySuccess,
    NoRoute,
    BadCoord,
}
// #[derive(Hash)]     // Not possible because of float hash.
#[derive(Clone, Debug)]
pub struct GeoCoord {
    lat_text: String,
    lon_text: String,
    latitude: f64,
    longitude: f64,
}

impl Hash for GeoCoord {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.lat_text.hash(state);
        self.lon_text.hash(state);
    }
}

impl GeoCoord {
    pub fn new() -> GeoCoord {
        GeoCoord {
            lat_text: "0".to_string(),
            lon_text: "0".to_string(),
            latitude: 0.,
            longitude: 0.,
        }
    }
    pub fn from(lat: &str, lon: &str) -> GeoCoord {
        GeoCoord {
            lat_text: lat.to_string(),
            lon_text: lon.to_string(),
            latitude: lat.trim().parse::<f64>().unwrap(),
            longitude: lon.trim().parse::<f64>().unwrap(),
        }
    }
}

impl PartialEq for GeoCoord {
    fn eq(&self, other: &Self) -> bool {
        self.lat_text == other.lat_text && self.lon_text == other.lon_text
    }
}

impl Eq for GeoCoord {}

impl PartialOrd for GeoCoord {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for GeoCoord {
    fn cmp(&self, other: &Self) -> Ordering {
        match self.lat_text.cmp(&other.lat_text) {
            Ordering::Equal => self.lon_text.cmp(&other.lon_text),
            order => order,
        }
    }
}

#[derive(Clone, Debug)]
pub struct StreetSegment {
    pub start: GeoCoord,
    pub end: GeoCoord,
    pub name: String,
}

impl StreetSegment {
    pub fn new() -> StreetSegment {
        StreetSegment {
            start: GeoCoord::new(),
            end: GeoCoord::new(),
            name: String::new(),
        }
    }
    pub fn from(s: &GeoCoord, e: &GeoCoord, street_name: &str) -> StreetSegment {
        StreetSegment {
            start: s.clone(),
            end: e.clone(),
            name: street_name.to_string(),
        }
    }
    pub fn length(&self) -> f64 {
        distance_earth_miles(&self.start, &self.end)
    }
    pub fn reverse_segment(&self) -> StreetSegment {
        StreetSegment::from(&self.end, &self.start, &self.name)
    }
}

impl PartialEq for StreetSegment {
    fn eq(&self, other: &Self) -> bool {
        self.start == other.start && self.end == other.end
    }
}

impl Eq for StreetSegment {}

// class PointToPointRouterImpl;

// class PointToPointRouter
// {
// public:
//     PointToPointRouter(const StreetMap* sm);
//     ~PointToPointRouter();
//     DeliveryResult generatePointToPointRoute(
//         const GeoCoord& start,
//         const GeoCoord& end,
//         std::list<StreetSegment>& route,
//         double& totalDistanceTravelled) const;
//       // We prevent a PointToPointRouter object from being copied or assigned.
//     PointToPointRouter(const PointToPointRouter&) = delete;
//     PointToPointRouter& operator=(const PointToPointRouter&) = delete;
// private:
//     PointToPointRouterImpl* m_impl;
// };

// struct DeliveryRequest
// {
//     DeliveryRequest(std::string it, const GeoCoord& loc)
//      : item(it), location(loc)
//     {}
//     std::string item;
//     GeoCoord location;
// };

#[derive(Clone, Debug)]
pub struct DeliveryRequest {
    pub item: String,
    pub location: GeoCoord,
}

impl DeliveryRequest {
    pub fn from(it: &str, loc: &GeoCoord) -> DeliveryRequest {
        DeliveryRequest {
            item: it.to_string(),
            location: loc.clone(),
        }
    }
}

#[derive(Clone, Debug)]
enum CommandType {
    Invalid,
    Proceed,
    Turn,
    Deliver,
}

#[derive(Clone, Debug)]
pub struct DeliveryCommand {
    command: CommandType,
    direction: String,
    street_name: String,
    item: String,
    distance: f64,
}

impl DeliveryCommand {
    pub fn new_proceed(direction: String, street_name: String, distance: f64) -> Self {
        DeliveryCommand {
            command: CommandType::Proceed,
            direction,
            street_name,
            item: String::default(),
            distance,
        }
    }
    pub fn init_proceed(&mut self, direction: String, street_name: String, distance: f64) {
        self.command = CommandType::Proceed;
        self.direction = direction;
        self.street_name = street_name;
        self.distance = distance;
    }
    pub fn new_turn(direction: String, street_name: String) -> Self {
        DeliveryCommand {
            command: CommandType::Turn,
            direction,
            street_name,
            item: String::default(),
            distance: f64::default(),
        }
    }
    pub fn init_turn(&mut self, direction: String, street_name: String) {
        self.command = CommandType::Turn;
        self.direction = direction;
        self.street_name = street_name;
    }
    pub fn new_deliver(item: String) -> Self {
        DeliveryCommand {
            command: CommandType::Deliver,
            direction: String::default(),
            street_name: String::default(),
            item,
            distance: f64::default(),
        }
    }
    pub fn init_deliver(&mut self, item: String) {
        self.command = CommandType::Deliver;
        self.item = item;
    }
    pub fn increase_distance(&mut self, more_distance: f64) {
        self.distance += more_distance;
    }
    pub fn street_name(&self) -> &str {
        &self.street_name
    }
}

impl std::fmt::Display for DeliveryCommand {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match &self.command {
            CommandType::Invalid => write!(f, "<invalid>"),
            CommandType::Proceed => write!(
                f,
                "Proceed {} on {} for {:.2} miles",
                self.direction, self.street_name, self.distance
            ),
            CommandType::Turn => write!(f, "Turn {} on {}", self.direction, self.street_name),
            CommandType::Deliver => write!(f, "Deliver {}", self.item),
        }
    }
}

impl Default for DeliveryCommand {
    fn default() -> Self {
        DeliveryCommand {
            command: CommandType::Invalid,
            direction: String::default(),
            street_name: String::default(),
            item: String::default(),
            distance: f64::default(),
        }
    }
}

// Tools for computing distance between GeoCoords, angle of a StreetSegment,
// and angle between two StreetSegments

/**
* Returns the distance between two points on the Earth.
* Direct translation from http://en.wikipedia.org/wiki/Haversine_formula
* @param lat1d Latitude of the first point in degrees
* @param lon1d Longitude of the first point in degrees
* @param lat2d Latitude of the second point in degrees
* @param lon2d Longitude of the second point in degrees
* @return The distance between the two points in kilometers
*/

pub fn distance_earth_km(g1: &GeoCoord, g2: &GeoCoord) -> f64 {
    let lat1r = g1.latitude.to_radians();
    let lon1r = g1.longitude.to_radians();
    let lat2r = g2.latitude.to_radians();
    let lon2r = g2.longitude.to_radians();
    let u = f64::sin((lat2r - lat1r) / 2.0);
    let v = f64::sin((lon2r - lon1r) / 2.0);
    return 2.0
        * EARTH_RADIUS_KM
        * f64::asin((u * u + f64::cos(lat1r) * f64::cos(lat2r) * v * v).sqrt());
}

pub fn distance_earth_miles(g1: &GeoCoord, g2: &GeoCoord) -> f64 {
    distance_earth_km(g1, g2) * MILES_PER_KM
}

pub fn angle_between_2_lines(line1: &StreetSegment, line2: &StreetSegment) -> f64 {
    let angle1 = f64::atan2(
        line1.end.latitude - line1.start.latitude,
        line1.end.longitude - line1.start.longitude,
    );
    let angle2 = f64::atan2(
        line2.end.latitude - line2.start.latitude,
        line2.end.longitude - line2.start.longitude,
    );
    let result = (angle2 - angle1).to_degrees();
    if result < 0. {
        result + 360.
    } else if result > 360. {
        result - 360.
    } else {
        result
    }
}

// Should latitude and longitude be switched in the method call?
pub fn angle_of_line(line: &StreetSegment) -> f64 {
    let result = f64::atan2(
        line.end.latitude - line.start.latitude,
        line.end.longitude - line.start.longitude,
    )
    .to_degrees();
    if result < 0. {
        result + 360.
    } else {
        result
    }
}
