#include <tekari/data_io.h>

#include <unordered_set>
#include <fstream>
#include <tekari/stop_watch.h>
#include <tekari/selections.h>

TEKARI_NAMESPACE_BEGIN

struct Vector2f_hash : std::unary_function<Vector2f, size_t>
{
    size_t operator()(const Vector2f& v) const {
        size_t hash1 = std::hash<Vector2f::Value>()(v[0]);
        size_t hash2 = std::hash<Vector2f::Value>()(v[1]);
        return hash1 ^ (hash2 << 1);
    }
};

void load_standard_data_sample(
    std::ifstream& file,
    RawMeasurement& raw_measurement,
    Matrix2Xf& V2D,
    Metadata& metadata
);
void load_spectral_data_sample(
    std::ifstream& file,
    RawMeasurement& raw_measurement,
    Matrix2Xf& V2D,
    Metadata& metadata
);

void load_data_sample(
    const string& file_name,
    RawMeasurement& raw_measurement,
    Matrix2Xf& V2D,
    VectorXf& selected_points,
    Metadata& metadata
)
{
    START_PROFILING("Loading data sample");
    // try open file
    std::ifstream file(file_name);
    if (!file)
        throw std::runtime_error("Unable to open file " + file_name);

    size_t line_number = 0;
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
            {
                load_spectral_data_sample(file, raw_measurement, V2D, metadata);
            }
            else
            {
                load_standard_data_sample(file, raw_measurement, V2D, metadata);
            }

            selected_points.assign(metadata.points_in_file(), NOT_SELECTED_FLAG);
            break;
        }
    }
    END_PROFILING();
}

void load_standard_data_sample(
    std::ifstream& file,
    RawMeasurement& raw_measurement,
    Matrix2Xf& V2D,
    Metadata& metadata
)
{
    std::unordered_set<Vector2f, Vector2f_hash> read_vertices;

    V2D.resize(metadata.points_in_file());
    raw_measurement.resize(metadata.points_in_file(), 0);

    size_t line_number = 0;
    size_t n_points = 0;
    for (string line; getline(file, line); )
    {
        ++line_number;
        trim(line);

        if (line.empty() || line[0] == '#')
        {
            // skip empty/comment lines
        }
        else
        {
            float theta, phi, luminance;
            if (sscanf(line.c_str(), "%f %f %f", &theta, &phi, &luminance) != 3)
            {
                throw std::runtime_error("Error reading file");
            }

            Vector2f p2d = Vector2f{ theta, phi };
            if (read_vertices.count(p2d) != 0)
            {
                cerr << "Warning: found two points with exact same coordinates\n";
                continue;
            }
            read_vertices.insert(p2d);
            Vector2f transformed_point = hemisphere_to_disk(p2d);
            raw_measurement[n_points].set_theta(theta);
            raw_measurement[n_points].set_phi(phi);
            raw_measurement[n_points].set_luminance(luminance);
            V2D[n_points] = transformed_point;
            ++n_points;
        }
    }
}
void load_spectral_data_sample(
    std::ifstream& file,
    RawMeasurement& raw_measurement,
    Matrix2Xf& V2D,
    Metadata& metadata
)
{
    int n_wave_lengths = metadata.data_points_per_loop();

    std::unordered_set<Vector2f, Vector2f_hash> read_vertices;
    vector<vector<float>> raw_m;


    size_t line_number = 0;
    size_t n_points = 0;
    for (string line; getline(file, line); )
    {
        ++line_number;
        trim(line);

        if (line.empty() || line[0] == '#')
        {
            // skip empty/comment lines
        }
        else
        {
            std::istringstream line_stream{ line };
            Vector2f angles;
            line_stream >> angles[0] >> angles[1];
            if (read_vertices.count(angles) > 0)
            {
                cerr << "Warning: found two points with exact same coordinates\n";
                continue;                                   // skip similar points
            }
            read_vertices.insert(angles);

            V2D.push_back(hemisphere_to_disk(angles));
            raw_m.push_back(vector<float>{});
            raw_m[n_points].resize(n_wave_lengths + 3, 0);
            raw_m[n_points][0] = angles[0];
            raw_m[n_points][1] = angles[1];

            for (int i = 0; i < n_wave_lengths; ++i)
            {
                line_stream >> raw_m[n_points][i+3];
            }
            ++n_points;
        }
    }
    metadata.set_points_in_file(n_points);

    raw_measurement.resize(n_points, n_wave_lengths);
    for (size_t i = 0; i < n_points; ++i)
    {
        memcpy(raw_measurement[i].data(), raw_m[i].data(), n_wave_lengths + 3);
    }
}

void save_data_sample(
    const string& path,
    const RawMeasurement& raw_measurement,
    const Metadata& metadata
)
{
    START_PROFILING("Save data sample");
    // try open file
    FILE* dataset_file = fopen(path.c_str(), "w");
    if (!dataset_file)
        throw std::runtime_error("Unable to open file " + path);

    // save metadata
    for(const auto& line: metadata.raw_metadata())
        fprintf(dataset_file, "%s\n", line.c_str());

    //!feof(dataset_file) && !ferror(dataset_file))
    for (Index i = 0; i < raw_measurement.n_sample_points(); ++i)
    {
        RawMeasurement::SamplePoint sample_point = raw_measurement[i];
        for (Index j = 0; j < raw_measurement.n_wave_lengths() + 3; ++j)
        {
            fprintf(dataset_file, "%lf ", sample_point[j]);
        }
        fprintf(dataset_file, "\n");
    }
    fclose(dataset_file);
    END_PROFILING();
}

TEKARI_NAMESPACE_END