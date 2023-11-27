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
    STATE,
    STATEQ,
    STATEQTIME,
    STATEQTIMEEXT,
    REAL,
    REALQ,
    REALQTIME,
    REALQTIMEEXT,
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
    SETPOINTDISCRETE
} DPTYPE;

class TASE2Datapoint
{
  public:
    TASE2Datapoint (const std::string& label, const std::string& objref,
                    DPTYPE type);
    ~TASE2Datapoint ();

    static int getDpTypeFromString (const std::string& type);

    DPTYPE
    getType () { return m_type; };

    const std::string
    getObjRef ()
    {
        return m_objref;
    };
    const std::string
    getLabel ()
    {
        return m_label;
    };

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

  private:
    std::string m_label;
    std::string m_objref;

    bool isTransient;

    long m_intVal;
    float m_floatVal;

    bool m_hasIntVal;

    Tase2_QualityClass m_quality;
    Tase2_TimeStampClass m_timestamp;

    DPTYPE m_type;
};

#endif
