#pragma once

#include <nanogui/opengl.h>
#include <limits>

#include "common.h"

TEKARI_NAMESPACE_BEGIN

class PointSampleInfo
{
public:
    PointSampleInfo()
        : m_PointCount(0)
        , m_AveragePoint{ 0.0f, 0.0f, 0.0f }
        , m_AverageRawPoint{ 0.0f, 0.0f, 0.0f }
        , m_MinIntensity(std::numeric_limits<float>::max())
        , m_MaxIntensity(std::numeric_limits<float>::min())
        , m_LowestPointIndex(0)
        , m_HighestPointIndex(0)
    {}

    void addPoint(unsigned int index, const nanogui::Vector3f& rawPoint, const nanogui::Vector3f& transformedPoint)
    {
        float new_min_intensity = std::min(m_MinIntensity, rawPoint[2]);
        float new_max_intensity = std::max(m_MaxIntensity, rawPoint[2]);
        if (new_min_intensity > m_MinIntensity)
        {
            m_LowestPointIndex = index;
        }
        if (new_max_intensity > m_MaxIntensity)
        {
            m_HighestPointIndex = index;
        }
        m_MinIntensity = new_min_intensity;
        m_MaxIntensity = new_max_intensity;

        m_AveragePoint += transformedPoint;
        m_AverageRawPoint += rawPoint;

        ++m_PointCount;
    }

    void normalize()
    {
        if (m_PointCount == 0)
            return;
        m_AveragePoint /= m_PointCount;
        m_AverageRawPoint /= m_PointCount;

        m_AveragePoint[1] = (m_AveragePoint[1] - m_MinIntensity) / (m_MaxIntensity - m_MinIntensity);
    }

    nanogui::Vector3f averagePoint()    const { return m_AveragePoint; }
    unsigned int pointsCount()   const { return m_PointCount; }
    float averageIntensity()    const { return m_AverageRawPoint[2]; }
    float minIntensity()        const { return m_MinIntensity; }
    float maxIntensity()        const { return m_MaxIntensity; }

    unsigned int highestPointIndex() const { return m_HighestPointIndex; }

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