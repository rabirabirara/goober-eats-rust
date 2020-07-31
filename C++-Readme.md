# Goober-Eats - Old Readme
Delivery routing application in C++.

Written Winter 2020.  This project cannot be built without the auxiliary files written by Prof. Smallberg and Nachenberg.

Admittedly, it is a bad idea for this project to be available on GitHub in the event that we see a repeat.  The general design of the project can be found in my Rust rewrite of it
([here](https://github.com/rabirabirara/goober-eats-rust)).  I have removed one source file and in the future, may decide to remove more, if not all of them as needed.  (EDIT: evidently I have
removed the entire repository from the public view.)
But the feeling clashes with my desire for transparency about my development skill!  I can guarantee that the deleted files will be "best-first", with the most flawed files saved for last.

Usage: GooberEats.exe mapdata.txt deliveries.txt

# Writing Deliveries

Coordinates are given in mapdata.txt.  Any addition of coordinate data should adhere to a similar format.

To add a delivery, append a colon to a coordinate as formatted in mapdata.txt, then immediately append the name of the delivery associated with it.


# An honest analysis

I missed points in my hashmap implementation and in my TSP approximation file.  The reason for the loss of points on the Optimizer class is not clearly explained (test cases were never revealed), but I can see that my implementation of the hashmap wasn't as sound as it could be, due to my decision to use list::splice to preserve list order when resizing the hashmap.  I encountered many problems during development because of this.
At any rate, a STL hashmap might've been replaced in this scenario; negating the need for me to lose points for this in the first place!

As for optimality points, I imagine that I could've improved my rating had I allowed the calculations to take place with more iterations.  I limited the calculations to a maximum of 15 iterations - hardly enough to provide the best route!  Like all routing applications, it may be preferable to take a little more time to produce a better route, demanding more iterations.
However, for only 15 iterations (sometimes less), 4/5 optimality points is... not bad.  But we could do better.  One consideration for an improved optimizer is to implement the Stem and Cycle reference structure, which recent research has explicated in great detail (though not in great clarity).

Nevertheless, the style of commenting is evidently inexperienced and distracting.  Code I would later write is much cleaner and more informative, rather than overbearing.  

This object is prime for rewriting in Rust, not just because of a craze for a new language - but because Rust genuinely streamlines many of the objectives in the project, especially the parsing of data in StreetMap.cpp.

A grade markup:

<pre>
Your total score for Project 4 is 87/100.
This breaks down as:
        ExpandableHashMap:      16/21
        StreetMap:              18/18
        PointToPointRouter:     20/20
        DeliveryOptimizer:      13/20
        DeliveryPlanner:        11/11
        Optimality points:      4/5
        Report:                 5/5
The median total score on this project was 76 out of 100.
</pre>
