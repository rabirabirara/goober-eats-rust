use crate::error::{DeliveryFailure, RouteError};
use crate::optimizer::DeliveryOptimizer;
use crate::point_router::PointToPointRouter;
use crate::provided::*;
use crate::street_map::StreetMap;
use std::collections::VecDeque;

pub struct DeliveryPlanner<'a> {
    point_router: PointToPointRouter<'a>,
}

impl<'a> DeliveryPlanner<'a> {
    pub fn new(street_map: &'a StreetMap) -> Self {
        DeliveryPlanner {
            point_router: PointToPointRouter::from(street_map),
        }
    }
    pub fn generate_plan(
        &self,
        depot: GeoCoord,
        deliveries: Vec<DeliveryRequest>,
    ) -> Result<(Vec<DeliveryCommand>, f64), RouteError> {
        let mut commands = Vec::<DeliveryCommand>::new();

        let optimizer = DeliveryOptimizer::new(deliveries.clone()/*self.street_map, &self.point_router*/);
        let (new_deliveries, _new_crow) = optimizer.optimize_order(&depot, deliveries);

        let mut current = depot.clone();
        let mut routes: Vec<Vec<StreetSegment>> = Vec::new();
        let mut total_distance = 0.;

        for delivery in &new_deliveries {
            let (route, cost) = self
                .point_router
                .generate_route(&current, &delivery.location)?;
            current = delivery.location.clone();
            routes.push(route);
            total_distance += cost;
        }
        let (route, cost) = self.point_router.generate_route(&current, &depot)?;
        routes.push(route);
        total_distance += cost;

        // The constructions are different.
        let mut here;
        let mut last = StreetSegment::new();
        let delivery_count = new_deliveries.len();
        let mut delivered: usize = 0;

        for route in &routes {
            let mut moves = VecDeque::<DeliveryCommand>::new();
            for segment in route {
                here = segment.clone();

                if moves.is_empty() {
                    // * A starting command.
                    moves.push_back(DeliveryCommand::new_proceed(
                        proceed_dir(angle_of_line(&here)).to_string(),
                        here.name.clone(),
                        here.length(),
                    ));
                    last = here;
                    continue;
                }

                if here.name == last.name {
                    if let Some(continue_this) = moves.front_mut() {
                        continue_this.increase_distance(here.length());
                        continue;
                    }
                } else {
                    commands.push(moves.pop_front().unwrap());
                    let direction = angle_between_2_lines(&last, &here);

                    if direction < 1. || direction > 359. {
                        moves.push_back(DeliveryCommand::new_proceed(
                            proceed_dir(direction).to_string(),
                            here.name.clone(),
                            here.length(),
                        ));
                        last = here;
                        continue;
                    } else {
                        commands.push(DeliveryCommand::new_turn(
                            turn_dir(direction).to_string(),
                            here.name.clone(),
                        ));
                        last = here;
                    }
                }
            }
            if delivered != delivery_count {
                // * Push in the final proceed command.
                if !moves.is_empty() {
                    commands.push(moves.front().unwrap().clone());
                }
                commands.push(DeliveryCommand::new_deliver(
                    new_deliveries[delivered].item.clone(),
                ));
                delivered += 1;
            } else if delivered == delivery_count {
                if !moves.is_empty() {
                    commands.push(moves.front().unwrap().clone());
                }
                return Ok((commands, total_distance));
            }
        }
        Err(RouteError::new(DeliveryFailure::NoRoute))
    }
}

// * Do we need to modulo the direction angle?
fn proceed_dir(direction: f64) -> &'static str {
    if direction >= 0.0 && direction < 22.5 {
        "east"
    } else if direction >= 22.5 && direction < 67.5 {
        "northeast"
    } else if direction >= 67.5 && direction < 112.5 {
        "north"
    } else if direction >= 112.5 && direction < 157.5 {
        "northwest"
    } else if direction >= 157.5 && direction < 202.5 {
        "west"
    } else if direction >= 202.5 && direction < 247.5 {
        "southwest"
    } else if direction > 247.5 && direction < 292.5 {
        "south"
    } else if direction >= 292.5 && direction < 337.5 {
        "southeast"
    } else {
        "east"
    }
}
fn turn_dir(direction: f64) -> &'static str {
    if direction > 1. && direction < 180. {
        "left"
    } else if direction > 180. && direction < 359. {
        "right"
    } else {
        eprintln!("Improper direction received.");
        ""
    }
}
