#include "tekari/data_io.h"

#include <unordered_set>
#include <fstream>
#include "tekari/stop_watch.h"

TEKARI_NAMESPACE_BEGIN

using namespace std;
using namespace nanogui;

struct Vector2f_hash : unary_function<Vector2f, size_t>
{
    size_t operator()(const Vector2f &v) const {
        size_t hash1 = std::hash<Vector2f::Scalar>()(v[0]);
        size_t hash2 = std::hash<Vector2f::Scalar>()(v[1]);
        return hash1 ^ (hash2 << 1);
    }
};

void load_standard_data_sample(
    ifstream& file,
    MatrixXf &raw_points,
    MatrixXf  &V2D,
    Metadata &metadata
);
void load_spectral_data_sample(
    ifstream& file,
    MatrixXf &raw_points,
    MatrixXf  &V2D,
    Metadata &metadata
);

void load_data_sample(
    const std::string& file_name,
    MatrixXf &raw_points,
    MatrixXf  &V2D,
    VectorXu8 &selected_points,
    Metadata &metadata
)
{
    START_PROFILING("Loading data sample");
    // try open file
    ifstream file(file_name);
    if (!file)
        throw runtime_error("Unable to open file " + file_name);

    unsigned int line_number = 0;
    for (string line; getline(file, line); )
    {
        ++line_number;

        if (line.empty())
        {
            continue;
        }
        if (line[0] == '#')
        {
            metadata.add_line(line);
        }
        else
        {
            file.seekg(0);

            metadata.init_infos();
            if (metadata.is_spectral())
                load_spectral_data_sample(file, raw_points, V2D, metadata);
            else
                load_standard_data_sample(file, raw_points, V2D, metadata);

            selected_points.resize(metadata.points_in_file());
            selected_points.set_zero();
            break;
        }
    }
    END_PROFILING();
}

void load_standard_data_sample(
    ifstream& file,
    MatrixXf &raw_points,
    MatrixXf  &V2D,
    Metadata &metadata
)
{
    unordered_set<Vector2f, Vector2f_hash> read_vertices;

    V2D.resize(2, metadata.points_in_file());
    raw_points.resize(3, metadata.points_in_file());

    unsigned int line_number = 0;
    unsigned int n_points = 0;
    for (string line; getline(file, line); )
    {
        ++line_number;

        if (line.empty() || line[0] == '#')
        {
            // skip empty/comment lines
        }
        else
        {
            float theta, phi, intensity;
            if (sscanf(line.c_str(), "%f %f %f", &theta, &phi, &intensity) != 3)
            {
                throw runtime_error("Error reading file");
            }

            Vector2f p2d = Vector2f{ theta, phi };
            if (read_vertices.count(p2d) != 0)
            {
                cerr << "Warning: found two points with exact same coordinates\n";
                continue;
            }
            read_vertices.insert(p2d);
            Vector2f transformed_point = transform_raw_point(p2d);
            raw_points.col(n_points) = Vector3f{ theta, phi, intensity };
            V2D.col(n_points) = transformed_point;
            ++n_points;
        }
    }
}
void load_spectral_data_sample(
    ifstream& file,
    MatrixXf &raw_points,
    MatrixXf  &V2D,
    Metadata &metadata
)
{
    int n_data_points_per_loop = metadata.data_points_per_loop();
    vector<vector<float>> raw_data;
    vector<Vector2f> v2d;

    unordered_set<Vector2f, Vector2f_hash> read_vertices;

    unsigned int line_number = 0;
    unsigned int n_points = 0;
    for (string line; getline(file, line); )
    {
        ++line_number;

        if (line.empty() || line[0] == '#')
        {
            // skip empty/comment lines
        }
        else
        {
            istringstream line_stream{ line };
            Vector2f angles;
            line_stream >> angles[0] >> angles[1];
            if (read_vertices.count(angles) > 0)
            {
                cerr << "Warning: found two points with exact same coordinates\n";
                continue;                                   // skip similar points
            }
            read_vertices.insert(angles);

            v2d.push_back(transform_raw_point(angles));
            raw_data.push_back(vector<float>{});
            raw_data[n_points].resize(n_data_points_per_loop + 2, 0);
            raw_data[n_points][0] = angles[0];
            raw_data[n_points][1] = angles[1];

            for (int i = 0; i < n_data_points_per_loop; ++i)
            {
                line_stream >> raw_data[n_points][i+2];
            }
            ++n_points;
        }
    }
    metadata.set_points_in_file(n_points);

    raw_points.resize(n_data_points_per_loop + 2, n_points);
    for (size_t i = 0; i < n_points; i++)
    {
        memcpy(raw_points.col(i).data(), raw_data[i].data(), sizeof(float) * (n_data_points_per_loop + 2));
    }

    V2D.resize(2, n_points);
    memcpy(V2D.data(), v2d.data(), sizeof(Vector2f) * n_points);
}

void save_data_sample(
    const std::string& path,
    const MatrixXf &raw_points,
    const Metadata &metadata
)
{
    START_PROFILING("Save data sample");
    // try open file
    FILE* dataset_file = fopen(path.c_str(), "w");
    if (!dataset_file)
        throw runtime_error("Unable to open file " + path);

    // save metadata
    for(const auto& line: metadata.raw_metadata())
        fprintf(dataset_file, "%s\n", line.c_str());

    //!feof(dataset_file) && !ferror(dataset_file))
    for (Eigen::Index i = 0; i < raw_points.cols(); ++i)
    {
        for (Eigen::Index j = 0; j < raw_points.rows(); ++j)
        {
            fprintf(dataset_file, "%lf ", raw_points(j, i));
        }
        fprintf(dataset_file, "\n");
    }
    fclose(dataset_file);
    END_PROFILING();
}

TEKARI_NAMESPACE_END