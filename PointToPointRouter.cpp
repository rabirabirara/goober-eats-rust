#include "provided.h"
#include "ExpandableHashMap.h"
#include "support.h"
#include <list>
#include <queue>
#include <unordered_map>
#include <functional>   // std::greater
#include <algorithm>

using namespace std;

inline double heuristic(const GeoCoord& one, const GeoCoord& two) {
    return distanceEarthMiles(one, two);
}

struct newGreater
{
    bool operator() (const pair<GeoCoord, double>& left, const pair<GeoCoord, double>& right) 
    {
        return (left.second > right.second);
    }
};

class PointToPointRouterImpl
{
public:
    PointToPointRouterImpl(const StreetMap* sm);
    ~PointToPointRouterImpl();
    DeliveryResult generatePointToPointRoute(
        const GeoCoord& start,
        const GeoCoord& end,
        list<StreetSegment>& route,
        double& totalDistanceTravelled) const;
private:
    const StreetMap* m_streetMap;
};

PointToPointRouterImpl::PointToPointRouterImpl(const StreetMap* sm)
    : m_streetMap(sm)
{}

PointToPointRouterImpl::~PointToPointRouterImpl()
{}

DeliveryResult PointToPointRouterImpl::generatePointToPointRoute(
        const GeoCoord& start,
        const GeoCoord& end,
        list<StreetSegment>& route,
        double& totalDistanceTravelled) const
{
    /*
    My original attempt at implementing this A* algorithm was arcane and complex, with pairs of pairs and top().first.end.blah everywhere.
    After doing some more research, I found a better structure.  We only define a simple heuristic function to calculate h, and 
    a simple newGreater to compare only pairs put into the priority queue.  (Before, the heuristic was complicated and not well organized.)

    StreetMap provides a graph representation of the lat/lon data of coordinates on Earth.  Each GeoCoord is a vertex.  StreetSegments can be described as edges.
    Vertices will be referred to as nodes.
        
    The A* algorithm:
    Every node on a graph representation has a weight.  This is determined as f(node).
    Every node has a "cost" to reach it from the starting point.  This is determined as g(node).
    Every node has a "heuristic" to give it weight with respect to the goal.  Commonly, this is distance to the goal measured in some form.  This is determined as h(node).
    f(n) = g(n) + h(n).  weight = cost_of_travel + distance_from_goal.
    This means a node with less cost and/or less distance away from the target is better than a node with more cost and/or more distance away from the target, and thus weighted less.

    We need two maps:
    One - A map of GeoCoords to GeoCoords.  This map maps a GeoCoord that has been travelled to, to the GeoCoord from which it was travelled to.  This map is used to produce our route.
    Two - A map of GeoCoords to doubles.  This map maps a node on the graph to its g(n) cost.  This map is used to produce our totalDistanceTravelled.  
    These both will be demonstrated necessary when this class is used for Dijkstra's algorithm in DeliveryOptimizer.

    We need a priority queue:
    This minheap queue sorts pairs of GeoCoords to doubles.  The pair<GeoCoord, double> structure groups a node on the graph with its weight, f(n).
    This way, the queue is arranged with the least weight at the top, and the highest weights near the bottom.
    Significant operations on the priority queue are typically O(log n), making them efficient for our usage.
    A custom comparator is defined in newGreater.

    We apply our A* algorithm to all the nearby nodes and find the best one (the one at the top of the priority_queue).
    We move there, updating our current location.  We search for the best node to move to using our algorithm again, and continue.
    We repeat this until our current node is the end node.  Once we have reached the end node, we prepare the route.
    We use our map #1 to trace back the path of StreetSegments that ultimately led from start to end.  (We reverse the segments and list as needed.)
    We use our map #2 to give us the g(n) cost of travelling from start to end.
    */


    vector<StreetSegment> segs;
    route.clear();
    totalDistanceTravelled = 0;

    if (!m_streetMap->getSegmentsThatStartWith(start, segs) || !m_streetMap->getSegmentsThatStartWith(end, segs))
        return BAD_COORD;

    ExpandableHashMap<GeoCoord, GeoCoord> prevs;
    ExpandableHashMap<GeoCoord, double> gCost;

    priority_queue<pair<GeoCoord, double>, vector<pair<GeoCoord, double> >, newGreater> nodes;  // The queue takes a pair of location to its "f" value.  When we push in a pair, we call our custom newGreater comparator, which compares their f scores.

    prevs.associate(start, start);
    gCost.associate(start, 0);

    nodes.push(pair<GeoCoord, double>(start, 0));
    
    while (!nodes.empty()) {
        GeoCoord current = nodes.top().first;
        nodes.pop();

        if (current == end) {            
            while (current != start) {
                m_streetMap->getSegmentsThatStartWith(current, segs);
                GeoCoord trace = (*(prevs.find(current)));    
                for (auto it = segs.begin(); it != segs.end(); ++it) {
                    if (it->end == trace) {
                        current = it->end;
                        route.push_back(reverseSegment(*it));
                        break;
                    }
                }
            }
            route.reverse();
            totalDistanceTravelled = (*gCost.find(end));
            return DELIVERY_SUCCESS;
        }

        if (!m_streetMap->getSegmentsThatStartWith(current, segs))
            return BAD_COORD;

        for (auto it = segs.begin(); it != segs.end(); ++it) {
            double newCost = *gCost.find(current) + distanceEarthMiles(current, it->end);

            if (gCost.find(it->end) == nullptr || newCost < *gCost.find(it->end)) {     // Note that if we find it->end in gCost, that means we have visited it already.  So we do not yet add it to the queue.
                gCost.associate(it->end, newCost);
                double f = newCost + heuristic(it->end, end);
                nodes.push(pair<GeoCoord, double>(it->end, f));
                prevs.associate(it->end, current);
            }
        }
    }
    return NO_ROUTE;
}

//******************** PointToPointRouter functions ***************************

// These functions simply delegate to PointToPointRouterImpl's functions.
// You probably don't want to change any of this code.

PointToPointRouter::PointToPointRouter(const StreetMap* sm)
{
    m_impl = new PointToPointRouterImpl(sm);
}

PointToPointRouter::~PointToPointRouter()
{
    delete m_impl;
}

DeliveryResult PointToPointRouter::generatePointToPointRoute(
        const GeoCoord& start,
        const GeoCoord& end,
        list<StreetSegment>& route,
        double& totalDistanceTravelled) const
{
    return m_impl->generatePointToPointRoute(start, end, route, totalDistanceTravelled);
}
