#include "tekari/points_stats.h"

#include <iostream>

TEKARI_NAMESPACE_BEGIN

using namespace nanogui;
using namespace std;

PointsStats::PointsStats()
    : m_PointCount(0)
    , m_AveragePoint{ 0.0f, 0.0f, 0.0f }
    , m_AverageRawPoint{ 0.0f, 0.0f, 0.0f }
    , m_MinIntensity(numeric_limits<float>::max())
    , m_MaxIntensity(numeric_limits<float>::min())
    , m_LowestPointIndex(0)
    , m_HighestPointIndex(0)
{}

void PointsStats::addPoint(unsigned int index, const Vector3f& rawPoint, const Vector3f& transformedPoint)
{
    if (rawPoint[2] < m_MinIntensity)
    {
        m_LowestPointIndex = index;
        m_MinIntensity = rawPoint[2];
    }
    if (rawPoint[2] > m_MaxIntensity)
    {
        m_HighestPointIndex = index;
        m_MaxIntensity = rawPoint[2];
    }

    m_AveragePoint += transformedPoint;
    m_AverageRawPoint += rawPoint;

    ++m_PointCount;
}

void PointsStats::normalize()
{
    if (m_PointCount == 0)
        return;
    m_AveragePoint /= m_PointCount;
    m_AverageRawPoint /= m_PointCount;
}

void update_selection_stats(
    PointsStats &selectionStats,
    const VectorXu8 &selectedPoints,
    const MatrixXf &rawPoints,
    const MatrixXf &V2D,
    const VectorXf &H)
{
    selectionStats = PointsStats();
    for (unsigned int i = 0; i < selectedPoints.size(); ++i)
    {
        if (selectedPoints[i])
        {
            selectionStats.addPoint(i, rawPoints.col(i), get3DPoint(V2D, H, i));
        }
    }
    selectionStats.normalize();
}

void update_points_stats(
    PointsStats &pointsStats,
    const MatrixXf &rawPoints,
    const MatrixXf &V2D
)
{
    pointsStats = PointsStats();

    for (Eigen::Index i = 0; i < rawPoints.cols(); ++i)
    {
        pointsStats.addPoint(i, rawPoints.col(i), Vector3f{ V2D(0, i), rawPoints(2, i), V2D(1, i) });
    }

    pointsStats.normalize();
    pointsStats.normalizeAverage();
}

TEKARI_NAMESPACE_END