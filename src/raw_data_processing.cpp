#include "tekari/normals.h"
#include "tekari/stop_watch.h"

#include <tbb/parallel_for.h>

TEKARI_NAMESPACE_BEGIN

#define MAX_SAMPLING_DISTANCE 0.05f

using namespace std;
using namespace nanogui;

void compute_normals(
    const tri_delaunay2d_t *triangulation,
    const vector<del_point2d_t> &V2D,
    const vector<float> &H,
    const vector<float> &LH,
    vector<Vector3f> &N,
    vector<Vector3f> &LN
);

void compute_triangle_normal(
    const vector<del_point2d_t> &V2D,
    const vector<float> &H,
    vector<Vector3f> &N,
    unsigned int i0,
    unsigned int i1,
    unsigned int i2
);

void compute_normalized_heights(
    const vector<Vector3f> &rawPoints,
    const PointsStats &pointsStats,
    vector<float> &H,
    vector<float> &LH
);

void triangulate_data(
    tri_delaunay2d_t **triangulation,
    vector<del_point2d_t> &V2D
);

void compute_path_segments(
    vector<unsigned int> &pathSegments,
    const vector<del_point2d_t> &V2D
);

void recompute_data(
    const vector<Vector3f> &rawPoints,
    PointsStats &pointsStats,
    tri_delaunay2d_t **triangulation,
    vector<unsigned int> &pathSegments,
    vector<del_point2d_t> &V2D,
    vector<float> &H,
    vector<float> &LH,
    vector<Vector3f> &N,
    vector<Vector3f> &LN
)
{
    PROFILE(update_points_stats(pointsStats, rawPoints, V2D));
    PROFILE(compute_normalized_heights(rawPoints, pointsStats, H, LH));
    PROFILE(triangulate_data(triangulation, V2D));
    PROFILE(compute_path_segments(pathSegments, V2D));
    PROFILE(compute_normals(*triangulation, V2D, H, LH, N, LN));
}

void compute_normals(
    const tri_delaunay2d_t *triangulation,
    const vector<del_point2d_t> &V2D,
    const vector<float> &H,
    const vector<float> &LH,
    vector<Vector3f> &N,
    vector<Vector3f> &LN
)
{
    N.resize(V2D.size());
    LN.resize(V2D.size());
    memset(N.data(), 0, sizeof(Vector3f) * N.size());
    memset(LN.data(), 0, sizeof(Vector3f) * LN.size());

    for (unsigned int i = 0; i < triangulation->num_triangles; ++i)
    {
        const unsigned int &i0 = triangulation->tris[3 * i];
        const unsigned int &i1 = triangulation->tris[3 * i + 1];
        const unsigned int &i2 = triangulation->tris[3 * i + 2];

        compute_triangle_normal(V2D, H, N, i0, i1, i2);
        compute_triangle_normal(V2D, LH, LN, i0, i1, i2);
    }

    for (size_t i = 0; i < N.size(); ++i)
    {
        N[i].normalize();
        LN[i].normalize();
    }
}

void compute_triangle_normal(
    const vector<del_point2d_t> &V2D,
    const vector<float> &H,
    vector<Vector3f> &N,
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

    N[i0] += w0 * faceNormal;
    N[i1] += w1 * faceNormal;
    N[i2] += w2 * faceNormal;
}

void compute_normalized_heights(
    const vector<Vector3f> &rawPoints,
    const PointsStats &pointsStats,
    vector<float> &H,
    vector<float> &LH
)
{
    H.resize(rawPoints.size());
    LH.resize(rawPoints.size());

    // normalize intensities
    float min_intensity = pointsStats.minIntensity();
    float max_intensity = pointsStats.maxIntensity();
    float correction_factor = 0.0f;
    if (min_intensity <= 0.0f)
        correction_factor = -min_intensity + 1e-10f;
    float min_log_intensity = log(min_intensity + correction_factor);
    float max_log_intensity = log(max_intensity);

    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)rawPoints.size(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t> &range) {
        for (uint32_t i = range.begin(); i < range.end(); ++i)
        {
            H[i] = (rawPoints[i][2] - min_intensity) / (max_intensity - min_intensity);
            LH[i] = (log(rawPoints[i][2] + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
        }
    }
    );
}

void triangulate_data(
    tri_delaunay2d_t **triangulation,
    vector<del_point2d_t> &V2D
)
{
    if (*triangulation)
    {
        tri_delaunay2d_release(*triangulation);
        *triangulation = nullptr;
    }

    // triangulate vertx data
    delaunay2d_t *delaunay;
    delaunay = delaunay2d_from(V2D.data(), V2D.size());
    *triangulation = tri_delaunay2d_from(delaunay);
    delaunay2d_release(delaunay);
}

void compute_path_segments(
    vector<unsigned int> &pathSegments,
    const vector<del_point2d_t> &V2D
)
{
    pathSegments.clear();
    // path segments must always contain the first point
    pathSegments.push_back(0);
    for (unsigned int i = 1; i < V2D.size(); ++i)
    {
        // if two last points are too far appart, a new path segments begins
        const del_point2d_t& current = V2D[i];
        const del_point2d_t& prev = V2D[i - 1];
        float dx = prev.x - current.x;
        float dz = prev.y - current.y;
        if (dx * dx + dz * dz > MAX_SAMPLING_DISTANCE)
        {
            pathSegments.push_back(i);
        }
    }
    // path segments must always contain the last point
    pathSegments.push_back(V2D.size());
}


TEKARI_NAMESPACE_END