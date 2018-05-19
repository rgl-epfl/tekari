#include "tekari/points_stats.h"

#include <iostream>

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

void PointsStats::addPoint(unsigned int index, const VectorXf& rawPoint, const MatrixXf& transformedPoint)
{
    for (size_t i = 0; i < rawPoint.size() - 2; i++)
    {
        if (rawPoint(2 + i) < mMinIntensity(i))
        {
            mLowestPointIndex(i) = index;
            mMinIntensity(i) = rawPoint[2];
        }
        if (rawPoint(2 + i) > mMaxIntensity(i))
        {
            mHighestPointIndex(i) = index;
            mMaxIntensity(i) = rawPoint[2];
        }
    }

    mAveragePoint += transformedPoint;
    mAverageRawPoint += rawPoint;

    ++mPointsCount;
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
    for (size_t i = 0; i < mAveragePoint.cols(); i++)
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
    selectionStats.mPointsCount = 0;
    selectionStats.setSize(rawPoints.rows() - 2);
    for (unsigned int i = 0; i < selectedPoints.size(); ++i)
    {
        if (selectedPoints[i])
        {
            selectionStats.addPoint(i, rawPoints.col(i), get3DPoints(V2D, H, i));
        }
    }
    selectionStats.normalize();
}

void update_points_stats(
    PointsStats &pointsStats,
    const MatrixXf &rawPoints,
    const MatrixXf &V2D,
    unsigned int waveLengthIndex
)
{
    pointsStats.mPointsCount = 0;
    pointsStats.setSize(rawPoints.rows() - 2);

    for (Eigen::Index i = 0; i < rawPoints.cols(); ++i)
    {
        MatrixXf result;
        result.resize(3, rawPoints.rows() - 2);
        for (size_t j = 0; j < result.cols(); j++)
        {
            result.col(j) = Vector3f{ V2D(0, j), rawPoints(2 + waveLengthIndex, j), V2D(1, j) };
        }
        pointsStats.addPoint(i, rawPoints.col(i), result);
    }

    pointsStats.normalize();
    pointsStats.normalizeAverage();
}

TEKARI_NAMESPACE_END