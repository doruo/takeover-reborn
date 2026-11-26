#include "game/sim/level/RoadGraph.h"

namespace takeover {

double Road::computeLength() const {
    double total = 0;
    for (std::size_t i = 1; i < wayPoints.size(); ++i) total += wayPoints[i].getDistanceTo(wayPoints[i - 1]);
    return total;
}

std::string RoadPoint::toString() const {
    return "point " + point.toString() + " roadIndex0 " + std::to_string(roadIndex0) + " roadIndex1 " +
           std::to_string(roadIndex1) + " dist2 " + std::to_string(distance2) + " roadDescription " +
           std::to_string(road->description->getIndex());
}

} // namespace takeover
