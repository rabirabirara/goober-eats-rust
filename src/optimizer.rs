use crate::provided::*;
use fastrand;
use std::collections::HashMap;

trait SimulatedAnnealing {
    type BaseNode;
    type VisitNode;

    fn simulated_annealing(
        &self,
        depot: &Self::BaseNode,
        deliveries: Vec<Self::VisitNode>,
    ) -> (Vec<Self::VisitNode>, f64);
    fn iterate(&self, depot: &Self::BaseNode, deliveries: &Vec<usize>) -> (Vec<usize>, f64);
    fn permute(deliveries: Vec<usize>) -> Vec<usize>;
    fn reconstruct(&self, order: Vec<usize>) -> Vec<Self::VisitNode>;
    fn crow_cost_indices(&self, depot: &Self::BaseNode, deliveries: &Vec<usize>) -> f64;
}

trait StemCycle {
    // * Defines an interface for working with my stem_cycle graph library.
}

pub struct DeliveryOptimizer<T> {
    map: HashMap<usize, T>,
}

impl<T> DeliveryOptimizer<T> {
    pub fn new(deliveries: Vec<T>) -> Self {
        let mut map = HashMap::new();
        for (i, city) in deliveries.into_iter().enumerate() {
            map.insert(i, city);
        }
        DeliveryOptimizer::<T> { map }
    }
    pub fn optimize_order(
        &self,
        depot: &GeoCoord,
        deliveries: Vec<DeliveryRequest>,
    ) -> (Vec<DeliveryRequest>, f64) {
        self.simulated_annealing(depot, deliveries)
    }
    // * Calculating the real cost of a route, with the point_router, is expensive.  Add the functionality in if desired.
    fn crow_cost(&self, depot: &GeoCoord, deliveries: &Vec<DeliveryRequest>) -> f64 {
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

impl SimulatedAnnealing for DeliveryOptimizer<DeliveryRequest> {
    type BaseNode = GeoCoord;
    type VisitNode = DeliveryRequest;

    fn simulated_annealing(
        &self,
        depot: &GeoCoord,
        deliveries: Vec<DeliveryRequest>,
    ) -> (Vec<DeliveryRequest>, f64) {
        let tour = (0..deliveries.len()).collect::<Vec<usize>>();

        // TODO: Implement multi-threading on this, so that we can take the best of a few runs.
        let (mut best_tour, mut best_cost) = self.iterate(depot, &tour);
        for _ in 0..100 {
            let (new_tour, new_cost) = self.iterate(depot, &best_tour);
            if new_cost < best_cost {
                best_tour = new_tour;
                best_cost = new_cost;
            }
        }
        let best_tour = self.reconstruct(best_tour);
        (best_tour, best_cost)
    }
    fn iterate(&self, depot: &GeoCoord, deliveries: &Vec<usize>) -> (Vec<usize>, f64) {
        let mut no_improvements = 0;
        let size = deliveries.len();

        let mut current_path = deliveries.clone();
        let mut current_cost = self.crow_cost_indices(depot, &current_path);
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
            let new_path = Self::permute(current_path);
            let new_cost = self.crow_cost_indices(depot, &new_path);
            if new_cost < current_cost {
                current_path = new_path.clone();
                current_cost = new_cost;
                if new_cost < best_cost {
                    best_cost = new_cost;
                    best_tour = new_path;
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
    fn permute(deliveries: Vec<usize>) -> Vec<usize> {
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
        // ! Check if this actually permutes anything.
        deliveries.clone()
    }
    fn reconstruct(&self, order: Vec<usize>) -> Vec<DeliveryRequest> {
        let mut tour = Vec::new();
        for city in order {
            tour.push(self.map[&city]);
        }
        tour
    }
    fn crow_cost_indices(&self, depot: &GeoCoord, deliveries: &Vec<usize>) -> f64 {
        self.crow_cost(depot, &self.reconstruct(deliveries.clone()))
    }
}

impl<'a> SimulatedAnnealing for &DeliveryOptimizer<DeliveryRequest> {
    type BaseNode = GeoCoord;
    type VisitNode = DeliveryRequest;

    fn simulated_annealing(
        &self,
        depot: &GeoCoord,
        deliveries: Vec<DeliveryRequest>,
    ) -> (Vec<DeliveryRequest>, f64) {
        let tour = (0..deliveries.len()).collect::<Vec<usize>>();

        // TODO: Implement multi-threading on this, so that we can take the best of a few runs.
        let (mut best_tour, mut best_cost) = self.iterate(depot, &tour);
        for _ in 0..100 {
            let (new_tour, new_cost) = self.iterate(depot, &best_tour);
            if new_cost < best_cost {
                best_tour = new_tour;
                best_cost = new_cost;
            }
        }
        let best_tour = self.reconstruct(best_tour);
        (best_tour, best_cost)
    }
    fn iterate(&self, depot: &GeoCoord, deliveries: &Vec<usize>) -> (Vec<usize>, f64) {
        let mut no_improvements = 0;
        let size = deliveries.len();

        let mut current_path = deliveries.clone();
        let mut current_cost = self.crow_cost_indices(depot, &current_path);
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
            let new_path = Self::permute(current_path);
            let new_cost = self.crow_cost_indices(depot, &new_path);
            if new_cost < current_cost {
                current_path = new_path.clone();
                current_cost = new_cost;
                if new_cost < best_cost {
                    best_cost = new_cost;
                    best_tour = new_path;
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
    fn permute(deliveries: Vec<usize>) -> Vec<usize> {
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
        // ! Check if this actually permutes anything.
        deliveries.clone()
    }
    fn reconstruct(&self, order: Vec<usize>) -> Vec<DeliveryRequest> {
        let mut tour = Vec::new();
        for city in order {
            tour.push(self.map[&city]);
        }
        tour
    }
    fn crow_cost_indices(&self, depot: &GeoCoord, deliveries: &Vec<usize>) -> f64 {
        self.crow_cost(depot, &self.reconstruct(deliveries.clone()))
    }
}
