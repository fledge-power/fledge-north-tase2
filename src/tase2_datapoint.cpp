#include "tase2_datapoint.hpp"
#include <unordered_map>

const static std::unordered_map<std::string, DPTYPE> dpTypeMap
    = { { "State", STATE },
        { "StateQ", STATEQ },
        { "StateQTime", STATEQTIME },
        { "StateQTimeExt", STATEQTIMEEXT },
        { "Real", REAL },
        { "RealQ", REALQ },
        { "RealQTime", REALQTIME },
        { "RealQTimeExt", REALQTIMEEXT },
        { "Discrete", DISCRETE },
        { "DiscreteQ", DISCRETEQ },
        { "DiscreteQTime", DISCRETEQTIME },
        { "DiscreteQTimeExt", DISCRETEQTIMEEXT },
        { "StateSup", STATESUP },
        { "StateSupQ", STATESUPQ },
        { "StateSupQTime", STATESUPQTIME },
        { "StateSupQTimeExt", STATESUPQTIMEEXT },
        { "Command", COMMAND },
        { "SetPointReal", SETPOINTREAL },
        { "SetPointDiscrete", SETPOINTDISCRETE } };

DPTYPE
TASE2Datapoint::getDpTypeFromString (const std::string& type)
{
    auto it = dpTypeMap.find (type);
    if (it != dpTypeMap.end ())
    {
        return it->second;
    }
    return DP_TYPE_UNKNOWN;
}

TASE2Datapoint::TASE2Datapoint (const std::string& label, DPTYPE type)
    : m_label (label), m_type (type)
{
}

TASE2Datapoint::~TASE2Datapoint () = default;