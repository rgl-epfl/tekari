#include <tekari/raw_data_processing.h>

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
    const MatrixXXf& H,
    const MatrixXXf& LH,
    Matrix4XXf& N,
    Matrix4XXf& LN,
    size_t intensity_index
)
{
    cout << std::setw(50) << std::left << "Computing normals .. ";
    Timer<> timer;

    const MatrixXXf::Row h_row  = H[intensity_index];
    const MatrixXXf::Row lh_row = LH[intensity_index];
    Matrix4XXf::Row n_row    = N[intensity_index];
    Matrix4XXf::Row ln_row   = LN[intensity_index];

    n_row.fill (Vector4f(0));
    ln_row.fill(Vector4f(0));

    for (Index f = 0; f < F.size(); ++f)
    {
        Vector3f fn = Vector3f(0.0f);
        Vector3f lfn = Vector3f(0.0f);
        const Vector3u &face = F[f];
        for (int i = 0; i < 3; ++i) {
            Vector3f v0 = get_3d_point(V2D, h_row, face[i]),
                     v1 = get_3d_point(V2D, h_row, face[(i+1)%3]),
                     v2 = get_3d_point(V2D, h_row, face[(i+2)%3]),
                     d0 = v1-v0,
                     d1 = v2-v0;

            if (i == 0) {
                fn = enoki::normalize(enoki::cross(d0, d1));
            }
            float angle = fast_acos(enoki::dot(d0, d1) / std::sqrt(enoki::squared_norm(d0) * enoki::squared_norm(d1)));
            n_row[face[i]] += enoki::concat(fn*angle, 0.0f);

            // same for log mesh

            v0 = get_3d_point(V2D, lh_row, face[i]);
            v1 = get_3d_point(V2D, lh_row, face[(i+1)%3]);
            v2 = get_3d_point(V2D, lh_row, face[(i+2)%3]);
            d0 = v1-v0;
            d1 = v2-v0;

            if (i == 0) {
                lfn = enoki::normalize(enoki::cross(d0, d1));
            }
            angle = fast_acos(enoki::dot(d0, d1) / std::sqrt(enoki::squared_norm(d0) * enoki::squared_norm(d1)));
            ln_row[face[i]] += enoki::concat(lfn*angle, 0.0f);
        }

    }

    tbb::parallel_for(tbb::blocked_range<uint32_t>(0u, (uint32_t)n_row.n_cols(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range) {
            for (uint32_t i = range.begin(); i != range.end(); ++i) {
                n_row[i] = enoki::normalize(n_row[i]);
                ln_row[i] = enoki::normalize(ln_row[i]);
            }
        }
    );
    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void compute_normalized_heights(
    const RawMeasurement& raw_measurement,
    const PointsStats& points_stats,
    MatrixXXf& H,
    MatrixXXf& LH,
    size_t intensity_index
)
{
    cout << std::setw(50) << std::left << "Computing normalized heights .. ";
    Timer<> timer;

    MatrixXXf::Row h_row  = H[intensity_index];
    MatrixXXf::Row lh_row = LH[intensity_index];

    float min_intensity = points_stats[intensity_index].min_intensity;
    float max_intensity = points_stats[intensity_index].max_intensity;

    if (std::abs(min_intensity - max_intensity) <= 1e-5)
    {
        cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
        return;
    }

    float correction_factor = (min_intensity <= 0.0f ? -min_intensity + CORRECTION_FACTOR : 0.0f);

    float min_log_intensity = log(min_intensity + correction_factor);
    float max_log_intensity = log(max_intensity + correction_factor);
    // normalize intensities
    tbb::parallel_for(tbb::blocked_range<uint32_t>(0, (uint32_t)H.n_cols(), GRAIN_SIZE),
        [&](const tbb::blocked_range<uint32_t>& range)
        {
            for (uint32_t i = range.begin(); i < range.end(); ++i)
            {
                h_row[i]     = (raw_measurement[i][intensity_index+2] - min_intensity) / (max_intensity - min_intensity);
                lh_row[i]    = (log(raw_measurement[i][intensity_index+2] + correction_factor) - min_log_intensity) / (max_log_intensity - min_log_intensity);
            }
        }
    );

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void triangulate_data(Matrix3Xu& F, Matrix2Xf& V2D)
{
    cout << std::setw(50) << std::left << "Triangulating data .. ";
    Timer<> timer;

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

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void compute_path_segments(VectorXu& path_segments, const Matrix2Xf& V2D)
{
    cout << std::setw(50) << std::left << "Computing path segments .. ";
    Timer<> timer;

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

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}


TEKARI_NAMESPACE_END