#ifndef TASE2_DATAPOINT_H
#define TASE2_DATAPOINT_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "datapoint.h"
#include "libtase2/tase2_common.h"
#include "libtase2/tase2_model.h"
#include "tase2_utility.hpp"

typedef enum
{
    REAL,
    REALQ,
    REALQTIME,
    REALQTIMEEXT,
    STATE,
    STATEQ,
    STATEQTIME,
    STATEQTIMEEXT,
    DISCRETE,
    DISCRETEQ,
    DISCRETEQTIME,
    DISCRETEQTIMEEXT,
    STATESUP,
    STATESUPQ,
    STATESUPQTIME,
    STATESUPQTIMEEXT,
    COMMAND,
    SETPOINTREAL,
    SETPOINTDISCRETE,
    DP_TYPE_UNKNOWN = -1
} DPTYPE;

class TASE2Datapoint
{
  public:
    TASE2Datapoint (const std::string& label, DPTYPE type);
    ~TASE2Datapoint ();

    static DPTYPE getDpTypeFromString (const std::string& type);

    static Tase2_IndicationPointType
    toIndicationPointType (DPTYPE type)
    {
        return static_cast<Tase2_IndicationPointType> (type / 4);
    };

    static Tase2_ControlPointType
    toControlPointType (DPTYPE type)
    {
        return static_cast<Tase2_ControlPointType> (type % 4);
    };

    static Tase2_QualityClass
    getQualityClass (DPTYPE type)
    {
        return static_cast<Tase2_QualityClass> (type % 4 > 0);
    };

    static Tase2_TimeStampClass
    getTimeStampClass (DPTYPE type)
    {
        return static_cast<Tase2_TimeStampClass> (type % 4 < 2 ? 0
                                                               : type % 4 - 1);
    };

    DPTYPE
    getType () { return m_type; };

    const std::string
    getLabel ()
    {
        return m_label;
    };

    static bool
    isCommand (DPTYPE type)
    {
        return type != DP_TYPE_UNKNOWN && type >= COMMAND;
    }

    Tase2_ControlPoint
    getControlPoint ()
    {
        return m_dp.ControlPoint;
    }

    Tase2_IndicationPoint
    getIndicationPoint ()
    {
        return m_dp.IndPoint;
    }

    void
    setControlPoint (Tase2_ControlPoint cont)
    {
        m_dp.ControlPoint = cont;
    };

    void
    setIndicationPoint (Tase2_IndicationPoint ind)
    {
        m_dp.IndPoint = ind;
    }

    const Tase2_QualityClass
    getQuality ()
    {
        return m_quality;
    };

    void setQuality (Datapoint* qualityDp);

    const long
    getIntVal ()
    {
        return m_intVal;
    };

    const float
    getFloatVal ()
    {
        return m_floatVal;
    };

    const bool
    hasIntVal ()
    {
        return m_hasIntVal;
    };

    bool updateDatapoint (Datapoint* value, Datapoint* timestamp,
                          Datapoint* quality);

    void
    setCheckBackId (int16_t id)
    {
        m_checkBackId = id;
    };

    int16_t
    getCheckBackId ()
    {
        return m_checkBackId;
    };

    bool
    inExchangedDefinitions ()
    {
        return m_inExchangedDefinitions;
    }

    void
    setInExchangedDefinitions (bool value)
    {
        m_inExchangedDefinitions = value;
    }

    bool hasTimedOut (uint64_t currentTime);

  private:
    std::string m_label;

    bool isTransient;

    long m_intVal;
    float m_floatVal;

    bool m_inExchangedDefinitions = false;

    bool m_hasIntVal;

    using dp = union
    {
        Tase2_IndicationPoint IndPoint;
        Tase2_ControlPoint ControlPoint;
    };

    uint64_t m_nextTimeout = 0;

    dp m_dp;

    int16_t m_checkBackId;

    Tase2_QualityClass m_quality;
    Tase2_TimeStampClass m_timestamp;

    DPTYPE m_type;
};

#endif
