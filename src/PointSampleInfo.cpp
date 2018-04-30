#include "tekari/PointSampleInfo.h"

#include <iostream>

TEKARI_NAMESPACE_BEGIN

using namespace nanogui;
using namespace std;

PointSampleInfo::PointSampleInfo()
    : m_PointCount(0)
    , m_AveragePoint{ 0.0f, 0.0f, 0.0f }
    , m_AverageRawPoint{ 0.0f, 0.0f, 0.0f }
    , m_MinIntensity(numeric_limits<float>::max())
    , m_MaxIntensity(numeric_limits<float>::min())
    , m_LowestPointIndex(0)
    , m_HighestPointIndex(0)
{}

void PointSampleInfo::addPoint(unsigned int index, const Vector3f& rawPoint, const Vector3f& transformedPoint)
{
    float new_min_intensity = min(m_MinIntensity, rawPoint[2]);
    float new_max_intensity = max(m_MaxIntensity, rawPoint[2]);
    if (new_min_intensity < m_MinIntensity)
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

void PointSampleInfo::normalize()
{
    if (m_PointCount == 0)
        return;
    m_AveragePoint /= m_PointCount;
    m_AverageRawPoint /= m_PointCount;
}

TEKARI_NAMESPACE_END