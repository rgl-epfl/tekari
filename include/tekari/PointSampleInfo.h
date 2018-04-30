#pragma once

#include <nanogui/opengl.h>
#include <limits>

#include "common.h"

TEKARI_NAMESPACE_BEGIN

class PointSampleInfo
{
public:
    PointSampleInfo();

    void addPoint(  unsigned int index,
                    const nanogui::Vector3f& rawPoint,
                    const nanogui::Vector3f& transformedPoint);

    void normalizeAverage() { m_AveragePoint[1] = (m_AveragePoint[1] - m_MinIntensity) / (m_MaxIntensity - m_MinIntensity); }
    void normalize();

    nanogui::Vector3f   averagePoint()      const { return m_AveragePoint; }
    unsigned int        pointsCount()       const { return m_PointCount; }
    float               averageIntensity()  const { return m_AverageRawPoint[2]; }
    float               minIntensity()      const { return m_MinIntensity; }
    float               maxIntensity()      const { return m_MaxIntensity; }
    unsigned int        highestPointIndex() const { return m_HighestPointIndex; }

private:
    unsigned int m_PointCount;
    nanogui::Vector3f m_AveragePoint;
    nanogui::Vector3f m_AverageRawPoint;
    float m_MinIntensity;
    float m_MaxIntensity;
    unsigned int m_LowestPointIndex;
    unsigned int m_HighestPointIndex;
};

TEKARI_NAMESPACE_END