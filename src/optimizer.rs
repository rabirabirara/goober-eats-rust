use crate::provided::*;
// use std::thread;
use fastrand;

trait SimulatedAnnealing {
    // ? Look into making this more efficient with Vec<usize> instead of Vec<DeliveryRequest>.
    // ? This was attempted, but one error failed the build (trait not impl'd for &T).
    // ? Look in the commit history.  A building attempt, later, turned out slow.
    type BaseNode;
    type VisitNode;

    fn simulated_annealing(
        depot: &Self::BaseNode,
        deliveries: &Vec<Self::VisitNode>,
    ) -> (Vec<Self::VisitNode>, f64);
    fn iterate(
        depot: &Self::BaseNode,
        deliveries: &Vec<Self::VisitNode>,
    ) -> (Vec<Self::VisitNode>, f64);
    fn permute(deliveries: Vec<Self::VisitNode>) -> Vec<Self::VisitNode>;
}

trait StemCycle {
    // * Defines an interface for working with my stem_cycle graph library.
}

pub struct DeliveryOptimizer;

impl DeliveryOptimizer {
    pub fn new() -> Self {
        DeliveryOptimizer {}
    }
    pub fn optimize_order(
        &self,
        depot: &GeoCoord,
        deliveries: Vec<DeliveryRequest>,
    ) -> (Vec<DeliveryRequest>, f64) {
        Self::simulated_annealing(depot, &deliveries)
    }
    // * Calculating the real cost of a route, with the point_router, is expensive.  Add the functionality in if desired.
    fn crow_cost(depot: &GeoCoord, deliveries: &Vec<DeliveryRequest>) -> f64 {
        let mut crow_distance = 0.;
        let mut current = depot;
        for delivery in deliveries {
            crow_distance += distance_earth_miles(current, &delivery.location);
            current = &delivery.location;
        }
		crow_distance += distance_earth_miles(current, depot);
        crow_distance
    }
}

impl SimulatedAnnealing for DeliveryOptimizer {
    type BaseNode = GeoCoord;
    type VisitNode = DeliveryRequest;

    fn simulated_annealing(
        depot: &GeoCoord,
        deliveries: &Vec<DeliveryRequest>,
    ) -> (Vec<DeliveryRequest>, f64) {
        // TODO: Implement multi-threading on this, so that we can take the best of a few runs.
        // ? Will need Arc<Mutex<T>>.
        let (mut best_tour, mut best_cost) = Self::iterate(depot, deliveries);
        for _ in 0..100 {
            let (new_tour, new_cost) = Self::iterate(depot, &best_tour);
            if new_cost < best_cost {
                best_tour = new_tour;
                best_cost = new_cost;
            }
        }
        (best_tour, best_cost)
    }
    fn iterate(depot: &GeoCoord, deliveries: &Vec<DeliveryRequest>) -> (Vec<DeliveryRequest>, f64) {
        let mut no_improvements = 0;
        let size = deliveries.len();

        let mut current_path = deliveries.clone();
        let mut current_cost = Self::crow_cost(depot, &current_path);
        let mut best_tour = deliveries.clone();
        let mut best_cost = f64::MAX;

        /* TODO: We can improve the accuracy better by not revisiting
        old paths, by encoding vector order into a set.  It's better than
        simply performing thousands of iterations. */
        let limit = match size {
            0..=10 => 50,
            11..=100 => 1000,
            _ => 2000,
        };
        let mut temperature = 0.9;
        let rng = fastrand::Rng::new();

        while no_improvements < limit {
            let new_path = Self::permute(current_path.clone());
            let new_cost = Self::crow_cost(depot, &new_path);
            if new_cost < current_cost {
                current_path = new_path;
                current_cost = new_cost;
                if new_cost < best_cost {
                    best_cost = new_cost;
                    best_tour = current_path.clone();
                    no_improvements = 0;
                }
            } else {
                let rand = rng.f32();
                if rand < temperature {
                    current_path = new_path;
                }
                no_improvements += 1;
                temperature *= 0.99;
            }
        }
        (best_tour, best_cost)
    }
    fn permute(mut deliveries: Vec<DeliveryRequest>) -> Vec<DeliveryRequest> {
        let delivery_count = deliveries.len();
        if delivery_count == 1 {
            return deliveries.clone();
        }
        let (mut rand1, mut rand2) = (0, 0);
        let rng = fastrand::Rng::new();
        while rand1 == rand2 {
            rand1 = rng.usize(..delivery_count);
            rand2 = rng.usize(..delivery_count);
        }
        deliveries.swap(rand1, rand2);
        deliveries
    }
}
