#include "tekari/raw_data_processing.h"
#include "tekari/stop_watch.h"

#define REAL float
#define VOID void
#include "triangle.h"

#include <tbb/parallel_for.h>

TEKARI_NAMESPACE_BEGIN

#define MAX_SAMPLING_DISTANCE 0.05f
#define CORRECTION_FACTOR 1e-5

using namespace std;
using namespace nanogui;

void compute_normals(
    const MatrixXu &F,
    const MatrixXf &V2D,
    const vector<VectorXf> &H,
    const vector<VectorXf> &LH,
    vector<MatrixXf> &N,
    vector<MatrixXf> &LN
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
    const MatrixXf &raw_points,
    PointsStats &points_stats,
    vector<VectorXf> &H,
    vector<VectorXf> &LH
);

void triangulate_data(
    MatrixXu &F,
    MatrixXf &V2D
);

void compute_path_segments(
    VectorXu &path_segments,
    const MatrixXf &V2D
);

void recompute_data(
    const MatrixXf &raw_points,
    PointsStats &points_stats,
    VectorXu &path_segments,
    MatrixXu &F,
    MatrixXf &V2D,
    vector<VectorXf> &H, vector<VectorXf> &LH,
    vector<MatrixXf> &N, vector<MatrixXf> &LN
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
    const MatrixXu &F,
    const MatrixXf &V2D,
    const vector<VectorXf> &H,
    const vector<VectorXf> &LH,
    vector<MatrixXf> &N,
    vector<MatrixXf> &LN
)
{
    START_PROFILING("Computing normals");
    N.resize(H.size());
    LN.resize(LH.size());

    tbb::parallel_for(tbb::blocked_range<uint32_t>(0u, (uint32_t)N.size(), 1),
        [&](const tbb::blocked_range<uint32_t> &range) {
            for (uint32_t j = range.begin(); j != range.end(); ++j) {
                N[j].resize(3, V2D.cols());
                N[j].set_zero();
                LN[j].resize(3, V2D.cols());
                LN[j].set_zero();

                for (Eigen::Index i = 0; i < F.cols(); ++i)
                {
                    const unsigned int &i0 = F(0, i);
                    const unsigned int &i1 = F(1, i);
                    const unsigned int &i2 = F(2, i);

                    compute_triangle_normal(V2D, H[j], N[j], i0, i1, i2);
                    compute_triangle_normal(V2D, LH[j], LN[j], i0, i1, i2);
                }

                tbb::parallel_for(tbb::blocked_range<uint32_t>(0u, (uint32_t)N[j].cols(), GRAIN_SIZE),
                    [&](const tbb::blocked_range<uint32_t> &range) {
                        for (uint32_t i = range.begin(); i != range.end(); ++i) {
                            N[j].col(i).normalize();
                            LN[j].col(i).normalize();
                        }
                    }
                );
            }
        }
    );
    END_PROFILING();
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

    Vector3f face_normal = e12.cross(-e01).normalized();

    float w0 = (float)acos(max(-1.0f, min(1.0f, e01.dot(-e20))));
    float w1 = (float)acos(max(-1.0f, min(1.0f, e12.dot(-e01))));
    float w2 = (float)acos(max(-1.0f, min(1.0f, e20.dot(-e12))));

    N.col(i0) += w0 * face_normal;
    N.col(i1) += w1 * face_normal;
    N.col(i2) += w2 * face_normal;
}

void compute_normalized_heights(
    const MatrixXf &raw_points,
    PointsStats &points_stats,
    vector<VectorXf> &H, vector<VectorXf> &LH
)
{
    START_PROFILING("Computing normalized heights");

    H.resize(raw_points.rows() - 2);
    LH.resize(raw_points.rows() - 2);
    for (size_t j = 0; j < H.size(); ++j)
    {
        H[j].resize(raw_points.cols());
        LH[j].resize(raw_points.cols());

        // normalize intensities
        float min_intensity = points_stats.min_intensity(j);
        float max_intensity = points_stats.max_intensity(j);
        float correction_factor = (min_intensity <= 0.0f ? -min_intensity + CORRECTION_FACTOR : 0.0f);

        float min_log_intensity = log(min_intensity + correction_factor);
        float max_log_intensity = log(max_intensity + correction_factor);

        tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)raw_points.cols(), GRAIN_SIZE),
            [&](const tbb::blocked_range<uint32_t> &range)
        {
            for (uint32_t i = range.begin(); i < range.end(); ++i)
            {
                H[j](i) = (raw_points(2 + j, i) - min_intensity) / (max_intensity - min_intensity);
                LH[j](i) = (log(raw_points(2 + j, i) + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
            }
        }
        );
    }
    END_PROFILING();
}

void triangulate_data(MatrixXu &F, MatrixXf &V2D)
{
    START_PROFILING("Triangulating data");

    struct triangulateio in, out;
    memset(&in, 0, sizeof(struct triangulateio));
    memset(&out, 0, sizeof(struct triangulateio));

    in.pointlist = V2D.data();
    in.numberofpoints = V2D.cols();

    triangulate("z_q_n", &in, &out, NULL);

    F.resize(3, out.numberoftriangles);
    memcpy(F.data(), out.trianglelist, out.numberoftriangles * 3 * sizeof(int));

    free(out.trianglelist);

    END_PROFILING();
}

void compute_path_segments(VectorXu &path_segments, const MatrixXf &V2D)
{
    START_PROFILING("Computing path segments");
    vector<unsigned int> path_segs;
    // path segments must always contain the first point
    path_segs.push_back(0);
    for (Eigen::Index i = 1; i < V2D.cols(); ++i)
    {
        // if two last points are too far appart, a new path segments begins
        const Vector2f& current = V2D.col(i);
        const Vector2f& prev = V2D.col(i - 1);
        if ((prev - current).squared_norm() > MAX_SAMPLING_DISTANCE)
        {
            path_segs.push_back(i);
        }
    }
    // path segments must always contain the last point
    path_segs.push_back(V2D.cols());

    path_segments.resize(path_segs.size());
    memcpy(path_segments.data(), path_segs.data(), path_segs.size() * sizeof(unsigned int));
    END_PROFILING();
}


TEKARI_NAMESPACE_END