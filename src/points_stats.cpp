#include "tekari/points_stats.h"

#include <iostream>
#include "tekari/selections.h"
#include "tekari/stop_watch.h"

TEKARI_NAMESPACE_BEGIN

using namespace nanogui;
using namespace std;

PointsStats::PointsStats()
    : mPointsCount(0)
{}

void PointsStats::setSize(unsigned int nWaveLengths)
{
    mAveragePoint.resize(3, nWaveLengths);
    mAverageRawPoint.resize(nWaveLengths + 2);
    mMinIntensity.resize(nWaveLengths);
    mMaxIntensity.resize(nWaveLengths);
    mLowestPointIndex.resize(nWaveLengths);
    mHighestPointIndex.resize(nWaveLengths);

    mAveragePoint.setConstant(0.0f);
    mAverageRawPoint.setConstant(0.0f);
    mMinIntensity.setConstant(numeric_limits<float>::max());
    mMaxIntensity.setConstant(numeric_limits<float>::min());
    mLowestPointIndex.setConstant(0);
    mHighestPointIndex.setConstant(0);
}

void PointsStats::addPoint(const VectorXf& rawPoint, const MatrixXf& transformedPoint)
{
    mAveragePoint += transformedPoint;
    mAverageRawPoint += rawPoint;
    ++mPointsCount;
}
void PointsStats::addIntensity(unsigned int index, const VectorXf &rawPoint)
{
    for (Eigen::Index i = 0; i < rawPoint.size() - 2; i++)
    {
        if (rawPoint(i + 2) < mMinIntensity(i))
        {
            mLowestPointIndex(i) = index;
            mMinIntensity(i) = rawPoint(i + 2);
        }
        if (rawPoint(i + 2) > mMaxIntensity(i))
        {
            mHighestPointIndex(i) = index;
            mMaxIntensity(i) = rawPoint(i + 2);
        }
    }
}

void PointsStats::normalize()
{
    if (mPointsCount == 0)
        return;
    mAveragePoint /= mPointsCount;
    mAverageRawPoint /= mPointsCount;
}

void PointsStats::normalizeAverage()
{
    for (Eigen::Index i = 0; i < mAveragePoint.cols(); i++)
    {
        mAveragePoint(1, i) = (mAveragePoint(1, i) - mMinIntensity(i)) / (mMaxIntensity(i) - mMinIntensity(i));
    }
}

void update_selection_stats(
    PointsStats &selectionStats,
    const VectorXu8 &selectedPoints,
    const MatrixXf &rawPoints,
    const MatrixXf &V2D,
    const vector<VectorXf> &H)
{
    START_PROFILING("Updating selection statistics");
    selectionStats.mPointsCount = 0;
    selectionStats.setSize(rawPoints.rows() - 2);
    for (Eigen::Index i = 0; i < selectedPoints.size(); ++i)
    {
        if (selectedPoints[i])
        {
            selectionStats.addPoint(rawPoints.col(i), get3DPoints(V2D, H, i));
        }
    }
    selectionStats.normalize();
    END_PROFILING();
}

void compute_min_max_intensities(
    PointsStats &pointsStats,
    const MatrixXf &rawPoints
)
{
    START_PROFILING("Computing minimum/maximum intensities");

    pointsStats.setSize(rawPoints.rows() - 2);
    for (Eigen::Index i = 0; i < rawPoints.cols() - 2; ++i)
    {
        pointsStats.addIntensity(i, rawPoints.col(i));
    }

    END_PROFILING();
}

void update_points_stats(
    PointsStats &pointsStats,
    const MatrixXf &rawPoints,
    const MatrixXf &V2D,
    const vector<VectorXf> &H
)
{
    START_PROFILING("Updating points statistics");

    pointsStats.mPointsCount = 0;
    for (Eigen::Index i = 0; i < rawPoints.cols(); ++i)
    {
        pointsStats.addPoint(rawPoints.col(i), get3DPoints(V2D, H, i));
    }

    pointsStats.normalize();
    pointsStats.normalizeAverage();

    END_PROFILING();
}

TEKARI_NAMESPACE_END