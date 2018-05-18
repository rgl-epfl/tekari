#include "tekari/raw_data_processing.h"
#include "tekari/stop_watch.h"

#include <tbb/parallel_for.h>

TEKARI_NAMESPACE_BEGIN

#define MAX_SAMPLING_DISTANCE 0.05f

using namespace std;
using namespace nanogui;

void compute_normals(
    const MatrixXu &F,
    const MatrixXf &V2D,
    const VectorXf &H,
    const VectorXf &LH,
    MatrixXf &N,
    MatrixXf &LN
);

void compute_triangle_normal(
    const MatrixXf &V2D,
    const VectorXf &H, 
    MatrixXf &N,
    unsigned int i0,
    unsigned int i1,
    unsigned int i2
);

void compute_normalized_heights(
    const MatrixXf &rawPoints,
    const PointsStats &pointsStats,
    VectorXf &H,
    VectorXf &LH
);

void triangulate_data(
    MatrixXu &F,
    MatrixXf &V2D
);

void compute_path_segments(
    VectorXu &pathSegments,
    const MatrixXf &V2D
);

void recompute_data(
    const MatrixXf &rawPoints,
    PointsStats &pointsStats,
    VectorXu &pathSegments,
    MatrixXu &F,
    MatrixXf &V2D,
    VectorXf &H, VectorXf &LH,
    MatrixXf &N, MatrixXf &LN
)
{
    PROFILE(update_points_stats(pointsStats, rawPoints, V2D));
    PROFILE(compute_normalized_heights(rawPoints, pointsStats, H, LH));
    PROFILE(triangulate_data(F, V2D));
    PROFILE(compute_path_segments(pathSegments, V2D));
    PROFILE(compute_normals(F, V2D, H, LH, N, LN));
}

void compute_normals(
    const MatrixXu &F,
    const MatrixXf &V2D,
    const VectorXf &H, const VectorXf &LH,
    MatrixXf &N, MatrixXf &LN
)
{
    N.resize(3, V2D.cols());
    N.setZero();
    LN.resize(3, V2D.cols());
    LN.setZero();
    
    for (unsigned int i = 0; i < F.cols(); ++i)
    {
        const unsigned int &i0 = F(0, i);
        const unsigned int &i1 = F(1, i);
        const unsigned int &i2 = F(2, i);

        compute_triangle_normal(V2D, H, N, i0, i1, i2);
        compute_triangle_normal(V2D, LH, LN, i0, i1, i2);
    }

    tbb::parallel_for(tbb::blocked_range<uint32_t>(0u, (uint32_t)N.cols(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range) {
            for (uint32_t i = range.begin(); i != range.end(); ++i) {
                N.col(i).normalize();
                LN.col(i).normalize();
            }
        }
    );
}

void compute_triangle_normal(
    const MatrixXf &V2D,
    const VectorXf &H,
    MatrixXf &N,
    unsigned int i0,
    unsigned int i1,
    unsigned int i2
)
{
    const Vector3f e01 = (get3DPoint(V2D, H, i1) - get3DPoint(V2D, H, i0)).normalized();
    const Vector3f e12 = (get3DPoint(V2D, H, i2) - get3DPoint(V2D, H, i1)).normalized();
    const Vector3f e20 = (get3DPoint(V2D, H, i0) - get3DPoint(V2D, H, i2)).normalized();

    Vector3f faceNormal = e12.cross(-e01).normalized();

    float w0 = (float)acos(max(-1.0f, min(1.0f, e01.dot(-e20))));
    float w1 = (float)acos(max(-1.0f, min(1.0f, e12.dot(-e01))));
    float w2 = (float)acos(max(-1.0f, min(1.0f, e20.dot(-e12))));

    N.col(i0) += w0 * faceNormal;
    N.col(i1) += w1 * faceNormal;
    N.col(i2) += w2 * faceNormal;
}

void compute_normalized_heights(
    const MatrixXf &rawPoints,
    const PointsStats &pointsStats,
    VectorXf &H, VectorXf &LH
)
{
    H.resize(rawPoints.cols());
    LH.resize(rawPoints.cols());

    // normalize intensities
    float min_intensity = pointsStats.minIntensity();
    float max_intensity = pointsStats.maxIntensity();
    float correction_factor = 0.0f;
    if (min_intensity <= 0.0f)
        correction_factor = -min_intensity + 1e-10f;
    float min_log_intensity = log(min_intensity + correction_factor);
    float max_log_intensity = log(max_intensity);

    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)rawPoints.cols(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range)
        {
            for (uint32_t i = range.begin(); i < range.end(); ++i)
            {
                H(i) = (rawPoints(2, i) - min_intensity) / (max_intensity - min_intensity);
                LH(i) = (log(rawPoints(2, i) + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
            }
        }
    );
}

void triangulate_data(MatrixXu &F, MatrixXf &V2D)
{
    // triangulate vertx data
    delaunay2d_t *delaunay = delaunay2d_from((del_point2d_t*)V2D.data(), V2D.cols());
    tri_delaunay2d_t *tri_delaunay = tri_delaunay2d_from(delaunay);

    F.resize(3, tri_delaunay->num_triangles);
    memcpy(F.data(), tri_delaunay->tris, tri_delaunay->num_triangles * 3 * sizeof(unsigned int));

    delaunay2d_release(delaunay);
    tri_delaunay2d_release(tri_delaunay);
}

void compute_path_segments(VectorXu &pathSegments, const MatrixXf &V2D)
{
    vector<unsigned int> pathSegs;
    // path segments must always contain the first point
    pathSegs.push_back(0);
    for (unsigned int i = 1; i < V2D.cols(); ++i)
    {
        // if two last points are too far appart, a new path segments begins
        const Vector2f& current = V2D.col(i);
        const Vector2f& prev = V2D.col(i - 1);
        if ((prev - current).squaredNorm() > MAX_SAMPLING_DISTANCE)
        {
            pathSegs.push_back(i);
        }
    }
    // path segments must always contain the last point
    pathSegs.push_back(V2D.cols());

    pathSegments.resize(pathSegs.size());
    memcpy(pathSegments.data(), pathSegs.data(), pathSegs.size() * sizeof(unsigned int));
}


TEKARI_NAMESPACE_END