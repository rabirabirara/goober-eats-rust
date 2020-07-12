#include "provided.h"
#include <vector>
#include <string>       // for implementing the swap of two coords/requests, because an assignment operator isn't defined
#include <algorithm>
#include <utility>
#include <cfloat>


using namespace std;

void swapGeoCoords(GeoCoord& first, GeoCoord& second) {
    GeoCoord temp(first.latitudeText, first.longitudeText);
    first.latitudeText = second.latitudeText;
    first.latitude = second.latitude;
    first.longitudeText = second.longitudeText;
    first.longitude = second.longitude;
    second.latitudeText = temp.latitudeText;
    second.latitude = temp.latitude;
    second.longitudeText = temp.longitudeText;
    second.longitude = temp.longitude;
}

void swapRequests(DeliveryRequest& first, DeliveryRequest& second) {
    DeliveryRequest temp(first.item, first.location);
    first.item = second.item;
    swapGeoCoords(first.location, second.location);
    second.item = temp.item;
    swapGeoCoords(second.location, temp.location);
}

class DeliveryOptimizerImpl
{
public:
    DeliveryOptimizerImpl(const StreetMap* sm);
    ~DeliveryOptimizerImpl();
    void optimizeDeliveryOrder(
        const GeoCoord& depot,
        vector<DeliveryRequest>& deliveries,
        double& oldCrowDistance,
        double& newCrowDistance) const;
private:
    const StreetMap* m_streetMap;
    const PointToPointRouter* m_router;

    double costOf(const GeoCoord& depot, const vector<DeliveryRequest>& deliveries) const {
        GeoCoord current = depot;
        list<StreetSegment> route;
        double cost = 0;
        double totalDistanceTravelled = 0;

        for (auto it = deliveries.begin(); it != deliveries.end(); ++it) {
            m_router->generatePointToPointRoute(current, it->location, route, cost);
            current = it->location;
            totalDistanceTravelled += cost;
        }
        m_router->generatePointToPointRoute(current, depot, route, cost);
        totalDistanceTravelled += cost;

        return totalDistanceTravelled;
    }

    vector<DeliveryRequest> permute(vector<DeliveryRequest>& deliveries) const {
        int deliveryCount = deliveries.size();
        if (deliveryCount == 1) // In case of a mistaken call on a single delivery
            return deliveries;
        int rand1, rand2;
        do 
        {
            rand1 = rand() % deliveryCount;
            rand2 = rand() % deliveryCount;
        } while (rand1 == rand2);

        swapRequests(deliveries[rand1], deliveries[rand2]);
        return deliveries;
    }

    double findCrowDistance(const GeoCoord& depot, const vector<DeliveryRequest>& deliveries) const {
        double crowDistance = 0;
        GeoCoord current = depot;
        for (auto it = deliveries.begin(); it != deliveries.end(); ++it) {
            crowDistance += distanceEarthMiles(current, it->location);
            current = it->location;
        }
        crowDistance += distanceEarthMiles(current, depot);
        return crowDistance;
    }
};

DeliveryOptimizerImpl::DeliveryOptimizerImpl(const StreetMap* sm)
    : m_streetMap(sm)
{
    m_router = new PointToPointRouter(sm);
}

DeliveryOptimizerImpl::~DeliveryOptimizerImpl()
{
    delete m_router;
}

void DeliveryOptimizerImpl::optimizeDeliveryOrder(
    const GeoCoord& depot,
    vector<DeliveryRequest>& deliveries,
    double& oldCrowDistance,
    double& newCrowDistance) const
{
    oldCrowDistance = 0;
    newCrowDistance = 0;
    oldCrowDistance = findCrowDistance(depot, deliveries);
    
    // This implementation finds the full cost of the A* found route from one point to another, instead of using the crow's distance.  This gives the real cost of a path.
    // If the user desires cost to be calculated by crow's distance, then degrade all costOf() calls into findCrowDistance() calls.  This is default.

    /// Using Simulated Annealing: ///
    // Take the current path.
    // Change it randomly.

    vector<DeliveryRequest> p = deliveries;         // Current path being checked
    vector<DeliveryRequest> newP;                   // New path permuted from the current path
    double curCost;                                 // Cost of current path
    double minCost = DBL_MAX;                       // Cost of the current best path
    double newCost;                                 // Cost of new path permuted from the current path
    vector<DeliveryRequest> bestTour;               // Current best path
    int noImprove = 0;                              // Check if there have been no improvements over the last n iterations.  
    int size = deliveries.size() * 2;               // n should depend on somewhat on delivery size...
    int n = min(size, 15);                          // ... but it should not grow too large.
    double chance = 90.0;                           // Roll to replace current path with new path, even if the new path is more costly.  Percent chance.

    while (noImprove < n)
    {
        curCost = findCrowDistance(depot, p);
        newP = permute(p);
        newCost = findCrowDistance(depot, newP);
        if (newCost < curCost) {
            p = newP;
            if (newCost < minCost) {
                minCost = newCost;
                bestTour = newP;
                noImprove = 0;
            }
        }
        else {
            int rand = std::rand() % 100;
            if (rand < chance) {
                p = newP;
            }
            noImprove++;
            chance *= 0.9;
        }
    }
    deliveries = bestTour;
    newCrowDistance = findCrowDistance(depot, deliveries);
}

//******************** DeliveryOptimizer functions ****************************

// These functions simply delegate to DeliveryOptimizerImpl's functions.
// You probably don't want to change any of this code.

DeliveryOptimizer::DeliveryOptimizer(const StreetMap* sm)
{
    m_impl = new DeliveryOptimizerImpl(sm);
}

DeliveryOptimizer::~DeliveryOptimizer()
{
    delete m_impl;
}

void DeliveryOptimizer::optimizeDeliveryOrder(
        const GeoCoord& depot,
        vector<DeliveryRequest>& deliveries,
        double& oldCrowDistance,
        double& newCrowDistance) const
{
    return m_impl->optimizeDeliveryOrder(depot, deliveries, oldCrowDistance, newCrowDistance);
}
