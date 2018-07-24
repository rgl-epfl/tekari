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
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    Metadata& metadata
);
void load_spectral_data_sample(
    std::ifstream& file,
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    Metadata& metadata
);
void load_bsdf_data_sample(
    std::ifstream& file,
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    Metadata& metadata
);

void load_data_sample(
    const string& file_name,
    MatrixXXf& raw_points,
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
            {
                load_spectral_data_sample(file, raw_points, V2D, metadata);
            }
            else
            {
                load_standard_data_sample(file, raw_points, V2D, metadata);
            }

            selected_points.assign(metadata.points_in_file(), NOT_SELECTED_FLAG);
            break;
        }
    }
    END_PROFILING();
}

void load_standard_data_sample(
    std::ifstream& file,
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    Metadata& metadata
)
{
    std::unordered_set<Vector2f, Vector2f_hash> read_vertices;

    V2D.resize(metadata.points_in_file());
    raw_points.resize(metadata.points_in_file());
    for (Index i = 0; i < raw_points.size(); ++i)
    {
        raw_points[i].resize(3);
    }

    unsigned int line_number = 0;
    unsigned int n_points = 0;
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
            float theta, phi, intensity;
            if (sscanf(line.c_str(), "%f %f %f", &theta, &phi, &intensity) != 3)
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
            Vector2f transformed_point = transform_raw_point(p2d);
            raw_points[n_points][0] = theta;
            raw_points[n_points][1] = phi;
            raw_points[n_points][2] = intensity;
            V2D[n_points] = transformed_point;
            ++n_points;
        }
    }
}
void load_spectral_data_sample(
    std::ifstream& file,
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    Metadata& metadata
)
{
    int n_data_points_per_loop = metadata.data_points_per_loop();

    std::unordered_set<Vector2f, Vector2f_hash> read_vertices;

    unsigned int line_number = 0;
    unsigned int n_points = 0;
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

            V2D.push_back(transform_raw_point(angles));
            raw_points.push_back(vector<float>{});
            raw_points[n_points].resize(n_data_points_per_loop + 2, 0);
            raw_points[n_points][0] = angles[0];
            raw_points[n_points][1] = angles[1];

            for (int i = 0; i < n_data_points_per_loop; ++i)
            {
                line_stream >> raw_points[n_points][i+2];
            }
            ++n_points;
        }
    }
    metadata.set_points_in_file(n_points);
}

void load_bsdf_data_sample(
    const string& file_name,
    MatrixXXf& raw_points,
    Matrix2Xf& V2D,
    Metadata& metadata
)
{
//     FILE *file = fopen(file_name, "rb");

//     if (file == nullptr)
//         throw std::runtime_error("Unable to open file " + file_name);

// //
//     if (size() < 12 + 2 + 4)
//         Throw("Invalid tensor file: too small, truncated?");
// //

//     uint8_t header[12], version[2];
//     uint32_t n_fields;
//     fread(header, sizeof(uint8_t), 12, file);
//     fread(version, sizeof(uint8_t), 2, file);
//     fread(&n_fields, sizeof(n_fields), 1, file);

//     if (memcmp(header, "tensor_file", 12) != 0)
//         Throw("Invalid tensor file: invalid header.");
//     else if (version[0] != 0 && version[1] != 0)
//         Throw("Invalid tensor file: unknown file version.");

//     for (uint32_t i = 0; i < n_fields; ++i) {
//         uint8_t dtype;
//         uint16_t name_length, ndim;
//         uint64_t offset;

//         fread(&name_length, 2, 1, file);
//         std::string name(name_length, '\0');
//         fread(name.data(), 1, name_length, file);
//         fread(&ndim, 2, 1, file);
//         fread(&dtype, 1, 1, file);
//         fread(&offset, 4, 1, file);
//         if (dtype == Struct::EInvalid || dtype > Struct::EFloat64)
//             Throw("Invalid tensor file: unknown type.");

//         std::vector<size_t> shape(ndim);
//         for (size_t j = 0; j < (size_t) ndim; ++j) {
//             uint64_t size_value;
//             fread(&size_value, sizeof(size_value), 1, file);
//             shape[j] = (size_t) size_value;
//         }

//         m_fields[name] =
//             Field{ (Struct::EType) dtype, static_cast<size_t>(offset), shape,
//                    (const uint8_t *) data() + offset };
//     }

    // load actual tensors
}

void save_data_sample(
    const string& path,
    const MatrixXXf& raw_points,
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
    for (Index i = 0; i < raw_points.size(); ++i)
    {
        for (Index j = 0; j < raw_points[i].size(); ++j)
        {
            fprintf(dataset_file, "%lf ", raw_points[i][j]);
        }
        fprintf(dataset_file, "\n");
    }
    fclose(dataset_file);
    END_PROFILING();
}

TEKARI_NAMESPACE_END