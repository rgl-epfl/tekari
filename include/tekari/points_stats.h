#pragma once

#include <nanogui/opengl.h>
#include <limits>

#include "common.h"

TEKARI_NAMESPACE_BEGIN

class PointsStats
{
public:
    PointsStats();

    void setSize(unsigned int nWaveLengths);

    unsigned int    pointsCount () const { return mPointsCount; }
    Vector3f        averagePoint        (unsigned int waveLengthIndex) const { return mAveragePoint.col(waveLengthIndex); }
    float           averageIntensity    (unsigned int waveLengthIndex) const { return mAverageRawPoint(waveLengthIndex + 2); }
    float           minIntensity        (unsigned int waveLengthIndex) const { return mMinIntensity(waveLengthIndex); }
    float           maxIntensity        (unsigned int waveLengthIndex) const { return mMaxIntensity(waveLengthIndex); }
    unsigned int    highestPointIndex   (unsigned int waveLengthIndex) const { return mHighestPointIndex(waveLengthIndex); }

    void addIntensity(unsigned int index, const VectorXf &rawPoint);
private:
    void addPoint(const VectorXf& rawPoint, const MatrixXf& transformedPoint);

    void normalizeAverage();
    void normalize();

    unsigned int mPointsCount;
    MatrixXf mAveragePoint;
    VectorXf mAverageRawPoint;
    VectorXf mMinIntensity;
    VectorXf mMaxIntensity;
    VectorXu mLowestPointIndex;
    VectorXu mHighestPointIndex;

    friend void update_selection_stats(
        PointsStats &selectionStats,
        const VectorXu8 &selectedPoints,
        const MatrixXf &rawPoints,
        const MatrixXf &V2D,
        const std::vector<VectorXf> &H
    );

    friend void update_points_stats(
        PointsStats &pointsStats,
        const MatrixXf &rawPoints,
        const MatrixXf &V2D,
        const std::vector<VectorXf> &H
    );

    friend void compute_min_max_intensities(
        PointsStats &pointsStats,
        const MatrixXf &rawPoints
    );
};

extern void update_selection_stats(
    PointsStats &selectionStats,
    const VectorXu8 &selectedPoints,
    const MatrixXf &rawPoints,
    const MatrixXf &V2D,
    const std::vector<VectorXf> &H
);

extern void compute_min_max_intensities(
    PointsStats &pointsStats,
    const MatrixXf &rawPoints
);

extern void update_points_stats(
    PointsStats &pointsStats,
    const MatrixXf &rawPoints,
    const MatrixXf &V2D,
    const std::vector<VectorXf> &H
);


TEKARI_NAMESPACE_END