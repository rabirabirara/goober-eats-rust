#include "provided.h"
#include <queue>
#include <vector>
using namespace std;

class DeliveryPlannerImpl {
  public:
    DeliveryPlannerImpl(const StreetMap* sm);
    ~DeliveryPlannerImpl();
    DeliveryResult
    generateDeliveryPlan(const GeoCoord& depot,
                         const vector<DeliveryRequest>& deliveries,
                         vector<DeliveryCommand>& commands,
                         double& totalDistanceTravelled) const;

  private:
    const StreetMap* m_streetMap;
    const PointToPointRouter* m_router;
    const DeliveryOptimizer* m_optimizer;

    double getLength(const StreetSegment& street) const {
        return (distanceEarthMiles(street.start, street.end));
    }

    string generateProceed(const double& direction) const {
        if (direction >= 0 && direction < 22.5)
            return "east";
        else if (direction >= 22.5 && direction < 67.5)
            return "northeast";
        else if (direction >= 67.5 && direction < 112.5)
            return "north";
        else if (direction >= 112.5 && direction < 157.5)
            return "northwest";
        else if (direction >= 157.5 && direction < 202.5)
            return "west";
        else if (direction >= 202.5 && direction < 247.5)
            return "southwest";
        else if (direction > 247.5 && direction < 292.5)
            return "south";
        else if (direction >= 292.5 && direction < 337.5)
            return "southeast";
        else
            return "east";
    }

    string generateTurn(const double& direction) const {
        // if (direction < 1 || direction > 359)
        //     return generateProceed(direction);

        if (direction >= 1 && direction < 180)
            return "left";
        if (direction >= 180 && direction <= 359)
            return "right";
        cerr << "generateTurn() received an improper direction.";
        return "";
    }
};

DeliveryPlannerImpl::DeliveryPlannerImpl(const StreetMap* sm)
    : m_streetMap(sm) {
    m_router = new PointToPointRouter(sm);
    m_optimizer = new DeliveryOptimizer(sm);
}

DeliveryPlannerImpl::~DeliveryPlannerImpl() {
    delete m_router;
    delete m_optimizer;
}

DeliveryResult DeliveryPlannerImpl::generateDeliveryPlan(
    const GeoCoord& depot, const vector<DeliveryRequest>& deliveries,
    vector<DeliveryCommand>& commands, double& totalDistanceTravelled) const {
    if (!commands.empty())
        commands.clear();

    double oldCrowDistance;
    double newCrowDistance;
    vector<DeliveryRequest> newDeliveries = deliveries;

    m_optimizer->optimizeDeliveryOrder(depot, newDeliveries, oldCrowDistance,
                                       newCrowDistance);

    GeoCoord current = depot;
    vector<list<StreetSegment>> routes;
    list<StreetSegment> route;
    double cost;
    totalDistanceTravelled = 0;

    for (auto it = newDeliveries.begin(); it != newDeliveries.end(); ++it) {
        DeliveryResult result = m_router->generatePointToPointRoute(
            current, it->location, route, cost);
        if (result == BAD_COORD)
            return BAD_COORD;
        if (result == NO_ROUTE)
            return NO_ROUTE;
        current = it->location;
        routes.push_back(list<StreetSegment>(route));
        totalDistanceTravelled += cost;
    }
    DeliveryResult result =
        m_router->generatePointToPointRoute(current, depot, route, cost);
    if (result == BAD_COORD)
        return BAD_COORD;
    if (result == NO_ROUTE)
        return NO_ROUTE;
    routes.push_back(list<StreetSegment>(route));
    totalDistanceTravelled += cost;

    // Default constructed.  Different in Rust.
    StreetSegment here;
    StreetSegment last;

    int deliveryCount = newDeliveries.size();
    int delivered = 0;
    for (auto it = routes.begin(); it != routes.end(); ++it) {
        queue<DeliveryCommand> moves;
        for (auto segment = it->begin(); segment != it->end(); ++segment) {
            here = *segment;

            if (moves.empty()) {
                DeliveryCommand start;
                start.initAsProceedCommand(generateProceed(angleOfLine(here)),
                                           here.name, getLength(here));
                moves.push(start);
                last = here;
                continue;
            }

            // compare names.  if good, then follow.
            // if not same, check angle.
            // if angle is right, pop. proceed and break.
            // else, turn. pop. proceed.

            if (here.name == last.name) {
                moves.front().increaseDistance(getLength(here));
                continue;
            } else {
                commands.push_back(moves.front());
                moves.pop();
                double direction = angleBetween2Lines(
                    last, here); // Note: the angle is (last, here) and not
                                 // (here, last).
                if (direction < 1 || direction > 359) {
                    DeliveryCommand command;
                    command.initAsProceedCommand(
                        generateProceed(angleOfLine(here)), here.name,
                        getLength(here));
                    moves.push(command);
                    last = here;
                    continue;
                } else {
                    DeliveryCommand turn;
                    turn.initAsTurnCommand(generateTurn(direction), here.name);
                    commands.push_back(turn);
                    last = here;
                }
            }

            DeliveryCommand command;
            command.initAsProceedCommand(generateProceed(angleOfLine(here)),
                                         here.name, getLength(here));
            moves.push(command);
        }
        if (delivered != deliveryCount) { // There is always one more route than
                                          // there are deliveries.
            if (!moves.empty())
                commands.push_back(moves.front());
            DeliveryCommand deliver;
            deliver.initAsDeliverCommand(newDeliveries[delivered].item);
            commands.push_back(deliver);
            delivered++;
        } else if (delivered == deliveryCount) {
            if (!moves.empty())
                commands.push_back(moves.front());
            return DELIVERY_SUCCESS;
        }
    }
    return NO_ROUTE;
}

//******************** DeliveryPlanner functions ******************************

// These functions simply delegate to DeliveryPlannerImpl's functions.
// You probably don't want to change any of this code.

DeliveryPlanner::DeliveryPlanner(const StreetMap* sm) {
    m_impl = new DeliveryPlannerImpl(sm);
}

DeliveryPlanner::~DeliveryPlanner() { delete m_impl; }

DeliveryResult DeliveryPlanner::generateDeliveryPlan(
    const GeoCoord& depot, const vector<DeliveryRequest>& deliveries,
    vector<DeliveryCommand>& commands, double& totalDistanceTravelled) const {
    return m_impl->generateDeliveryPlan(depot, deliveries, commands,
                                        totalDistanceTravelled);
}
