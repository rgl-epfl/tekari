#include "tekari/selections.h"

#include "tekari/stop_watch.h"

TEKARI_NAMESPACE_BEGIN

#define MAX_SELECTION_DISTANCE 30.0f

using namespace std;
using namespace nanogui;

void select_points(
    const MatrixXf &V2D,
    const VectorXf &H,
    VectorXu8 &selectedPoints,
    const Matrix4f & mvp,
    const SelectionBox& selectionBox,
    const Vector2i & canvasSize,
    SelectionMode mode)
{
    START_PROFILING("Selecting points");
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)V2D.cols(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i)
        {
            Vector3f point = get3DPoint(V2D, H, i);
            Vector4f projPoint = projectOnScreen(point, canvasSize, mvp);

            bool inSelection = selectionBox.contains(Vector2i{ projPoint[0], projPoint[1] });

            switch (mode)
            {
            case STANDARD:
                selectedPoints(i) = inSelection;
                break;
            case ADD:
                selectedPoints(i) = inSelection || selectedPoints[i];
                break;
            case SUBTRACT:
                selectedPoints(i) = !inSelection && selectedPoints[i];
                break;
            }
        }
    });
    END_PROFILING();
}

void select_closest_point(
    const MatrixXf &V2D,
    const VectorXf &H,
    VectorXu8 &selectedPoints,
    const Matrix4f& mvp,
    const Vector2i & mousePos,
    const Vector2i & canvasSize)
{
    START_PROFILING("Selecting closest point");
    size_t n_threads = V2D.cols() / GRAIN_SIZE + ((V2D.cols() % GRAIN_SIZE) > 0);
    vector<float> smallestDistances(n_threads, MAX_SELECTION_DISTANCE * MAX_SELECTION_DISTANCE);
    vector<int> closestPointIndices(n_threads, -1);

    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)V2D.cols(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range) {
        uint32_t threadId = range.begin() / GRAIN_SIZE;
        for (uint32_t i = range.begin(); i < range.end(); ++i)
        {
            Vector3f point = get3DPoint(V2D, H, i);
            Vector4f projPoint = projectOnScreen(point, canvasSize, mvp);

            float distSqr = Vector2f{ projPoint[0] - mousePos[0], projPoint[1] - mousePos[1] }.squaredNorm();

            if (smallestDistances[threadId] > distSqr)
            {
                closestPointIndices[threadId]   = i;
                smallestDistances[threadId]     = distSqr;
            }

            selectedPoints(i) = false;
        }
    });

    float smallestDistance = MAX_SELECTION_DISTANCE * MAX_SELECTION_DISTANCE;
    int closestPointIndex = -1;

    for (size_t i = 0; i < smallestDistances.size(); i++)
    {
        if (smallestDistance > smallestDistances[i])
        {
            closestPointIndex = closestPointIndices[i];
            smallestDistance = smallestDistances[i];
        }
    }

    if (closestPointIndex != -1)
    {
        selectedPoints(closestPointIndex) = true;
    }
    END_PROFILING();
}

void select_highest_point(
    const PointsStats &pointsInfo,
    const PointsStats &selectionInfo,
    VectorXu8 &selectedPoints,
    unsigned int waveLengthIndex
)
{
    START_PROFILING("Selecting highest point");
    int highestPointIndex = selectionInfo.pointsCount() == 0 ?
                            pointsInfo.highestPointIndex(waveLengthIndex) :
                            selectionInfo.highestPointIndex(waveLengthIndex);

    deselect_all_points(selectedPoints);
    selectedPoints(highestPointIndex) = 1;
    END_PROFILING();
}

void select_all_points(VectorXu8 &selectedPoints)
{
    START_PROFILING("Selecting all points");
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)selectedPoints.size(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i)
            selectedPoints(i) = 1;
    });
    END_PROFILING();
}

void deselect_all_points(VectorXu8 &selectedPoints)
{
    START_PROFILING("Deselecting all points");
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)selectedPoints.size(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i)
            selectedPoints(i) = 0;
    });
    END_PROFILING();
}

void move_selection_along_path(bool up, VectorXu8 &selectedPoints
)
{
    START_PROFILING("Moving selection along path");
    uint8_t extremity;
    if (up)
    {
        extremity = selectedPoints.head(1)(0);
        memmove(selectedPoints.data() + 1, selectedPoints.data(), selectedPoints.size() - 1);
        selectedPoints.tail(1)(0) = extremity;
    }
    else
    {
        extremity = selectedPoints.head(1)(0);
        memmove(selectedPoints.data(), selectedPoints.data() + 1, selectedPoints.size() - 1);
        selectedPoints.tail(1)(0) = extremity;
    }
    END_PROFILING();
}

void delete_selected_points(
    VectorXu8 &selectedPoints,
    MatrixXf &rawPoints,
    MatrixXf &V2D,
    PointsStats &selectionInfo
)
{
    START_PROFILING("Deleting selection");
    selectionInfo = PointsStats();

    Eigen::Index lastValid = 0;
    for (Eigen::Index i = 0; i < selectedPoints.size(); ++i)
    {
        if (!selectedPoints(i))
        {
            if (lastValid != i)         // prevent unnecessary copies
            {
                // move undeleted point to last valid position
                V2D.col(lastValid) = V2D.col(i);
                rawPoints.col(lastValid) = rawPoints.col(i);
            }
            ++lastValid;
        }
    }

    // resize vectors
    V2D.conservativeResize(2, lastValid);
    rawPoints.conservativeResize(3, lastValid);
    selectedPoints.conservativeResize(lastValid);
    selectedPoints.setZero();

    END_PROFILING();
}

unsigned int count_selected_points(const VectorXu8 &selectedPoints)
{
    unsigned int count = 0;
    START_PROFILING("Counting selected points");
    for (Eigen::Index i = 0; i < selectedPoints.size(); ++i) {
        count += (selectedPoints(i) != 0);
    }
    END_PROFILING();
    return count;
}

TEKARI_NAMESPACE_END