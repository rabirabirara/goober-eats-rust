mod optimizer;
mod planner;
mod error;
mod point_router;
mod provided;
mod street_map;
use planner::DeliveryPlanner;
use provided::{DeliveryRequest, GeoCoord};
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;
use street_map::StreetMap;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect::<Vec<String>>();

    if args.len() != 3 {
        println!("Usage: {} [MAP-DATA] [DELIVERIES]", args[0]);
        return Ok(());
    }

    // Consider rewriting street_map so that file opening is done in main.

    let sm = StreetMap::load_from(&args[1])?;

    let (depot, deliveries) = load_deliveries(&args[2])?;

    println!("Generating route...\n\n");

    let planner = DeliveryPlanner::new(&sm);

    let result = planner.generate_plan(depot, deliveries);

    match result {
        Ok((commands, distance_travelled)) => {
            for command in commands {
                println!("{}", command);
            }
            println!("You are back at the depot and your deliveries are done!");
            println!("{:.2} miles travelled for all deliveries.", distance_travelled);
        }
        Err(e) => {
            eprintln!("Error: {:?}", e);
        }
    }
    // * No harmful exit.
    Ok(())
}

fn load_deliveries(del_file: &str) -> Result<(GeoCoord, Vec<DeliveryRequest>), std::io::Error> {
    let file_path = Path::new(del_file);
    let file_handle = File::open(file_path)?;
    let contents = BufReader::new(file_handle);

    let mut coords = contents
        .lines()
        .map(|line| line.unwrap())
        .collect::<Vec<String>>()
        .into_iter();

    let first = coords.next().unwrap();
    let dep_coords: Vec<&str> = first.split_whitespace().collect();
    let depot = GeoCoord::from(dep_coords[0], dep_coords[1]);

    let mut deliveries: Vec<DeliveryRequest> = Vec::new();

    while let Some(request) = coords.next() {
        if !request.contains(':') {
            eprintln!("Missing colon in deliveries - line: {}.", request);
            continue;
        }
        let parts: Vec<&str> = request.split(':').collect();
        if parts[1] == "" {
            eprintln!("Missing item in deliveries - line: {}", request);
            continue;
        }
        let coords: Vec<&str> = parts[0].split_whitespace().collect();
        if coords.len() != 2 {
            eprintln!("Bad formatting in deliveries - line: {}", request);
            continue;
        }
        let coord = GeoCoord::from(coords[0], coords[1]);
        deliveries.push(DeliveryRequest::from(parts[1], &coord));
    }

    Ok((depot, deliveries))
}
