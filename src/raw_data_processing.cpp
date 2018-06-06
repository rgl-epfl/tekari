#include "tekari/raw_data_processing.h"
#include "tekari/stop_watch.h"

#define REAL float
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
    const MatrixXf &rawPoints,
    PointsStats &pointsStats,
    vector<VectorXf> &H,
    vector<VectorXf> &LH
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
    vector<VectorXf> &H, vector<VectorXf> &LH,
    vector<MatrixXf> &N, vector<MatrixXf> &LN
)
{
    compute_min_max_intensities(pointsStats, rawPoints);
    compute_normalized_heights(rawPoints, pointsStats, H, LH);
    triangulate_data(F, V2D);
    compute_path_segments(pathSegments, V2D);
    compute_normals(F, V2D, H, LH, N, LN);
    update_points_stats(pointsStats, rawPoints, V2D, H);

	for (size_t i = 0; i < rawPoints.rows(); i++)
	{
		cout << "Intensity : " << rawPoints(i, 0) << endl;
	}
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
                N[j].setZero();
                LN[j].resize(3, V2D.cols());
                LN[j].setZero();

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
    PointsStats &pointsStats,
    vector<VectorXf> &H, vector<VectorXf> &LH
)
{
    START_PROFILING("Computing normalized heights");

    H.resize(rawPoints.rows() - 2);
    LH.resize(rawPoints.rows() - 2);
    for (size_t j = 0; j < H.size(); ++j)
    {
        H[j].resize(rawPoints.cols());
        LH[j].resize(rawPoints.cols());

        // normalize intensities
        float min_intensity = pointsStats.minIntensity(j);
        float max_intensity = pointsStats.maxIntensity(j);
        float correction_factor = 0.0f;
        if (min_intensity <= 0.0f)
            correction_factor = -min_intensity + CORRECTION_FACTOR;
        float min_log_intensity = log(min_intensity + correction_factor);
        float max_log_intensity = log(max_intensity);

        tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)rawPoints.cols(), GRAIN_SIZE),
            [&](const tbb::blocked_range<uint32_t> &range)
        {
            for (uint32_t i = range.begin(); i < range.end(); ++i)
            {
                H[j](i) = (rawPoints(2, i) - min_intensity) / (max_intensity - min_intensity);
                LH[j](i) = (log(rawPoints(2, i) + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
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

	triangulate("zQN", &in, &out, NULL);

	F.resize(3, out.numberoftriangles);
	memcpy(F.data(), out.trianglelist, out.numberoftriangles * 3 * sizeof(int));

	free(out.trianglelist);

    END_PROFILING();
}

void compute_path_segments(VectorXu &pathSegments, const MatrixXf &V2D)
{
    START_PROFILING("Computing path segments");
    vector<unsigned int> pathSegs;
    // path segments must always contain the first point
    pathSegs.push_back(0);
    for (Eigen::Index i = 1; i < V2D.cols(); ++i)
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
    END_PROFILING();
}


TEKARI_NAMESPACE_END