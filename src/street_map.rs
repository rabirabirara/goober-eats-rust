use crate::provided::{GeoCoord, StreetSegment};
use std::collections::HashMap;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;

pub struct StreetMap {
    streets: HashMap<GeoCoord, Vec<StreetSegment>>,
}

impl StreetMap {
    pub fn new() -> StreetMap {
        StreetMap {
            streets: HashMap::new(),
        }
    }
    pub fn load_from(map_file: &str) -> Result<StreetMap, std::io::Error> {
        let mut sm = StreetMap::new();
        sm.load(map_file)?;
        Ok(sm)
    }
    pub fn load(&mut self, map_file: &str) -> Result<(), std::io::Error> {
        let file_path = Path::new(map_file);
        let file_handle = File::open(file_path)?;
        let file_read = BufReader::new(file_handle);

        let mut street_count: i32;
        let mut coords: String;

        let liter: Vec<String> = file_read.lines().filter_map(|line| line.ok()).collect();
        let mut liter = liter.into_iter();
        while let Some(name) = liter.next() {
            street_count = liter
                .next()
                .unwrap()
                .parse()
                .unwrap_or(0);

            for _i in 0..street_count {
                coords = liter.next().expect("Expected coords.");
                let each_coord: Vec<String> = coords
                    .split_ascii_whitespace()
                    .map(|word| word.to_string())
                    .collect();

                let (coord_lat1, coord_lon1, coord_lat2, coord_lon2) = (
                    &each_coord[0],
                    &each_coord[1],
                    &each_coord[2],
                    &each_coord[3],
                );

                let start = GeoCoord::from(&coord_lat1, &coord_lon1);
                let end = GeoCoord::from(&coord_lat2, &coord_lon2);
                let segment = StreetSegment::from(&start, &end, name.as_str());
                let rev_segment = segment.reverse_segment();

                if let Some(start_vec) = self.streets.get_mut(&start) {
                    start_vec.push(segment);
                } else {
                    let segs = vec![segment];
                    self.streets.insert(start, segs);
                }
                if let Some(end_vec) = self.streets.get_mut(&end) {
                    end_vec.push(rev_segment);
                } else {
                    let segs = vec![rev_segment];
                    self.streets.insert(end, segs);
                }
            }
        }

        Ok(())
    }
    pub fn get_segments_from(&self, gc: &GeoCoord) -> Option<&Vec<StreetSegment>> {
        self.streets.get(gc)
    }
}

/*
Recap:
-if let and while let are meant to be used with let Some().
*/
