#![allow(dead_code, unused_variables)]
use crate::error::{DeliveryFailure, RouteError};
use crate::provided::{self, GeoCoord, StreetSegment};
use crate::street_map::StreetMap;
use ordered_float::OrderedFloat;
use std::cmp::Ordering;
use std::collections::{BinaryHeap, HashMap};

// Not sure if deriving PartialEq and Eq is okay here.
#[derive(Clone, Copy, Debug)]
struct Node<'a> {
    coord: &'a GeoCoord,
    cost: OrderedFloat<f64>,
}
// With this lifetime, impossible to create empty Node.
impl<'a> Node<'a> {
    pub fn from(coord: &GeoCoord, f_cost: f64) -> Node {
        Node {
            coord,
            cost: OrderedFloat::from(f_cost),
        }
    }
}
impl PartialEq for Node<'_> {
    fn eq(&self, other: &Self) -> bool {
        self.coord == other.coord
    }
}
impl Eq for Node<'_> {}
impl Ord for Node<'_> {
    fn cmp(&self, other: &Self) -> Ordering {
        other.cost.cmp(&self.cost)
    }
}
impl PartialOrd for Node<'_> {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

// Lifetimes: You have a value, and a reference to a value.
// The reference must be destroyed before the value is.
// To ensure at compile time the reference is destroyed first,
// you specify its lifetime in code.
pub struct PointToPointRouter<'a> {
    street_map: &'a StreetMap,
}

impl<'a> PointToPointRouter<'a> {
    pub fn from(sm: &StreetMap) -> PointToPointRouter {
        PointToPointRouter { street_map: &sm }
    }
    pub fn generate_route(
        &self,
        start: &GeoCoord,
        end: &GeoCoord,
    ) -> Result<(Vec<StreetSegment>, f64), RouteError> {
        if self.street_map.get_segments_from(start) == None
            || self.street_map.get_segments_from(end) == None
        {
            return Err(RouteError::new(DeliveryFailure::BadCoord));
        }

        // need map and priority queue
        let mut prevs: HashMap<GeoCoord, GeoCoord> = HashMap::new();
        let mut g_costs: HashMap<GeoCoord, f64> = HashMap::new();
        // Nodes are used solely to compare f-costs in the priority queue.
        let mut nodes: BinaryHeap<Node> = BinaryHeap::new();

        let start_node = Node::from(start, 0.);
        prevs.insert(start.clone(), start.clone());
        g_costs.insert(start.clone(), 0.);
        nodes.push(start_node.clone());

        // FIXME: The g_costs are going wack.  Discover what inflates them so.
        while !nodes.is_empty() {
            let current = nodes.pop().unwrap();

            if current.coord == end {
                let mut current: GeoCoord = current.coord.clone();
                let mut route = Vec::new();
                let mut total_distance = 0.;
                while &current != start {
                    let segs = self.street_map.get_segments_from(&current).unwrap();
                    let trace = prevs.get(&current).unwrap();
                    if let Some(seg) = segs.iter().find(|&seg| &seg.end == trace) {
                        current = seg.end.clone();
                        route.push(seg.reverse_segment());
                        total_distance += seg.length();
                    }
                }
                route.reverse();
                // ! FIXME: g_costs has wack values in its map.
                // let total_distance = g_costs.get(end).unwrap().clone();
                return Ok((route, total_distance));
            }
            // Remember: g-cost is distance from start to node.
            // h-cost is distance from node to end. f-cost is g + h.
            if let Some(segs) = self.street_map.get_segments_from(current.coord) {
                for seg in segs {
                    let new_gcost = current.cost.into_inner()
                        + provided::distance_earth_miles(current.coord, &seg.end);
                    // Not finding seg.end means it is an untravelled node.
                    if !g_costs.contains_key(&seg.end)
                        || new_gcost < g_costs.get(&seg.end).unwrap().clone()
                    {
                        g_costs.insert(seg.end.clone(), new_gcost);
                        let f = new_gcost + provided::distance_earth_miles(&seg.end, end);
                        nodes.push(Node::from(&seg.end, f));
                        prevs.insert(seg.end.clone(), current.coord.clone());
                    }
                }
            } else {
                return Err(RouteError::new(DeliveryFailure::BadCoord));
            }
        }
        return Err(RouteError::new(DeliveryFailure::NoRoute));
    }
}
