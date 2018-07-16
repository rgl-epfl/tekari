#include <tekari/raw_data_processing.h>
#include <tekari/stop_watch.h>

#define REAL float
#define VOID void
#include <triangle.h>

#include <limits>
#include <tbb/parallel_for.h>

TEKARI_NAMESPACE_BEGIN

#define MAX_SAMPLING_DISTANCE 0.05f
#define CORRECTION_FACTOR 1e-5

void compute_normals(
    const Matrix3Xu& F,
    const Matrix2Xf& V2D,
    const vector<VectorXf>& H,
    const vector<VectorXf>& LH,
    vector<Matrix4Xf>& N,
    vector<Matrix4Xf>& LN
);

void compute_triangle_normal(
    const Matrix2Xf& V2D,
    const VectorXf& H,
    Matrix4Xf& N,
    unsigned int i0,
    unsigned int i1,
    unsigned int i2
);

void compute_normalized_heights(
    const MatrixXXf& raw_points,
    PointsStats& points_stats,
    vector<VectorXf>& H,
    vector<VectorXf>& LH
);

void triangulate_data(
    Matrix3Xu& F,
    Matrix2Xf& V2D
);

void compute_path_segments(
    VectorXu& path_segments,
    const Matrix2Xf& V2D
);

void recompute_data(
    const MatrixXXf& raw_points,
    PointsStats& points_stats,
    VectorXu& path_segments,
    Matrix3Xu& F,
    Matrix2Xf& V2D,
    vector<VectorXf>& H, vector<VectorXf>& LH,
    vector<Matrix4Xf>& N, vector<Matrix4Xf>& LN
)
{
    compute_min_max_intensities(points_stats, raw_points);
    compute_normalized_heights(raw_points, points_stats, H, LH);
    triangulate_data(F, V2D);
    compute_path_segments(path_segments, V2D);
    compute_normals(F, V2D, H, LH, N, LN);
    update_points_stats(points_stats, raw_points, V2D, H);
}

void compute_normals(
    const Matrix3Xu& F,
    const Matrix2Xf& V2D,
    const vector<VectorXf>& H,
    const vector<VectorXf>& LH,
    vector<Matrix4Xf>& N,
    vector<Matrix4Xf>& LN
)
{
    START_PROFILING("Computing normals");
    N.resize(H.size());
    LN.resize(LH.size());

    tbb::parallel_for(tbb::blocked_range<uint32_t>(0u, (uint32_t)N.size(), 1),
        [&](const tbb::blocked_range<uint32_t>& range) {
            for (uint32_t j = range.begin(); j != range.end(); ++j) {
                N[j].assign(V2D.size(), 0);
                LN[j].assign(V2D.size(), 0);

                for (Index i = 0; i < F.size(); ++i)
                {
                    compute_triangle_normal(V2D,  H[j],  N[j], F[i][0], F[i][1], F[i][2]);
                    compute_triangle_normal(V2D, LH[j], LN[j], F[i][0], F[i][1], F[i][2]);
                }

                tbb::parallel_for(tbb::blocked_range<uint32_t>(0u, (uint32_t)N[j].size(), GRAIN_SIZE),
                    [&](const tbb::blocked_range<uint32_t>& range) {
                        for (uint32_t i = range.begin(); i != range.end(); ++i) {
                            N[j][i] = enoki::normalize(N[j][i]);
                            LN[j][i] = enoki::normalize(LN[j][i]);
                        }
                    }
                );
            }
        }
    );
    END_PROFILING();
}

void compute_triangle_normal(
    const Matrix2Xf& V2D,
    const VectorXf& H,
    Matrix4Xf& N,
    unsigned int i0,
    unsigned int i1,
    unsigned int i2
)
{
    using namespace enoki;

    const Vector3f e01 = normalize(get3DPoint(V2D, H, i1) - get3DPoint(V2D, H, i0));
    const Vector3f e12 = normalize(get3DPoint(V2D, H, i2) - get3DPoint(V2D, H, i1));
    const Vector3f e20 = normalize(get3DPoint(V2D, H, i0) - get3DPoint(V2D, H, i2));

    Vector3f face_normal = normalize(cross(e12, -e01));

    Vector3f weights = Vector3f( dot(e01, -e20),
                                 dot(e12, -e01),
                                 dot(e20, -e12));
    weights = acos(max(-1.0f, (min(1.0f, weights)))); 

    N[i0] += weights[0] * concat(face_normal, 0.0f);
    N[i1] += weights[1] * concat(face_normal, 0.0f);
    N[i2] += weights[2] * concat(face_normal, 0.0f);
}

void compute_normalized_heights(
    const MatrixXXf& raw_points,
    PointsStats& points_stats,
    vector<VectorXf>& H, vector<VectorXf>& LH
)
{
    START_PROFILING("Computing normalized heights");

    Index wave_length_count = raw_points[0].size() - 2;     // don't take into account theta and phi angles
    Index points_count = raw_points.size();

    // compute overall min/max intensity
    float min_intensity = std::numeric_limits<float>::max();
    float max_intensity = std::numeric_limits<float>::min();
    for (size_t j = 0; j < wave_length_count; ++j)
    {
        min_intensity = std::min(min_intensity, points_stats.min_intensity(j));
        max_intensity = std::max(max_intensity, points_stats.max_intensity(j));
    }
    float correction_factor = (min_intensity <= 0.0f ? -min_intensity + CORRECTION_FACTOR : 0.0f);

    float min_log_intensity = log(min_intensity + correction_factor);
    float max_log_intensity = log(max_intensity + correction_factor);

    H.resize(wave_length_count);
    LH.resize(wave_length_count);
    for (size_t j = 0; j < wave_length_count; ++j)
    {
        H[j].resize(points_count);
        LH[j].resize(points_count);

        // normalize intensities
        tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)points_count, GRAIN_SIZE),
            [&](const tbb::blocked_range<uint32_t>& range)
        {
            for (uint32_t i = range.begin(); i < range.end(); ++i)
            {
                H[j][i] = (raw_points[i][2 + j] - min_intensity) / (max_intensity - min_intensity);
                LH[j][i] = (log(raw_points[i][2 + j] + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
            }
        }
        );
    }
    END_PROFILING();
}

void triangulate_data(Matrix3Xu& F, Matrix2Xf& V2D)
{
    START_PROFILING("Triangulating data");

    struct triangulateio in, out;
    memset(&in, 0, sizeof(struct triangulateio));
    memset(&out, 0, sizeof(struct triangulateio));

    in.pointlist = (float*)V2D.data();
    in.numberofpoints = V2D.size();

    char cmds[4] = {'z', 'Q', 'N', '\0'};
    triangulate(cmds, &in, &out, NULL);

    F.resize(out.numberoftriangles);
    memcpy(F.data(), out.trianglelist, out.numberoftriangles * 3 * sizeof(int));

    free(out.trianglelist);

    END_PROFILING();
}

void compute_path_segments(VectorXu& path_segments, const Matrix2Xf& V2D)
{
    START_PROFILING("Computing path segments");
    path_segments.clear();
    // path segments must always contain the first point
    path_segments.push_back(0);
    for (Index i = 1; i < V2D.size(); ++i)
    {
        // if two last points are too far appart, a new path segments begins
        const Vector2f& current = V2D[i];
        const Vector2f& prev = V2D[i - 1];
        if (enoki::squared_norm(prev - current) > MAX_SAMPLING_DISTANCE)
        {
            path_segments.push_back(i);
        }
    }
    // path segments must always contain the last point
    path_segments.push_back(V2D.size());
    END_PROFILING();
}


TEKARI_NAMESPACE_END