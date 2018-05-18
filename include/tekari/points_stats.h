#pragma once

#include <nanogui/opengl.h>
#include <limits>

#include "common.h"

TEKARI_NAMESPACE_BEGIN

class PointsStats
{
public:
    PointsStats();

    Vector3f   averagePoint()      const { return m_AveragePoint; }
    unsigned int        pointsCount()       const { return m_PointCount; }
    float               averageIntensity()  const { return m_AverageRawPoint[2]; }
    float               minIntensity()      const { return m_MinIntensity; }
    float               maxIntensity()      const { return m_MaxIntensity; }
    unsigned int        highestPointIndex() const { return m_HighestPointIndex; }

private:
    void addPoint(unsigned int index,
        const Vector3f& rawPoint,
        const Vector3f& transformedPoint);

    void normalizeAverage() { m_AveragePoint[1] = (m_AveragePoint[1] - m_MinIntensity) / (m_MaxIntensity - m_MinIntensity); }
    void normalize();

    unsigned int m_PointCount;
    Vector3f m_AveragePoint;
    Vector3f m_AverageRawPoint;
    float m_MinIntensity;
    float m_MaxIntensity;
    unsigned int m_LowestPointIndex;
    unsigned int m_HighestPointIndex;

    friend void update_selection_stats(
        PointsStats &selectionStats,
        const VectorXu8 &selectedPoints,
        const MatrixXf &rawPoints,
        const MatrixXf &V2D,
        const VectorXf &H
    );

    friend void update_points_stats(
        PointsStats &pointsStats,
        const MatrixXf &rawPoints,
        const MatrixXf &V2D
    );
};

extern void update_selection_stats(
    PointsStats &selectionStats,
    const VectorXu8 &selectedPoints,
    const MatrixXf &rawPoints,
    const MatrixXf &V2D,
    const VectorXf &H
);

extern void update_points_stats(
    PointsStats &pointsStats,
    const MatrixXf &rawPoints,
    const MatrixXf &V2D
);


TEKARI_NAMESPACE_END