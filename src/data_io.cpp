#include "tekari/data_io.h"

#include <unordered_set>
#include <functional>
#include "tekari/stop_watch.h"

TEKARI_NAMESPACE_BEGIN

using namespace std;
using namespace nanogui;

struct Vector2fHash : unary_function<Vector2f, size_t>
{
    size_t operator()(const Vector2f &v) const {
        size_t hash1 = std::hash<Vector2f::Scalar>()(v[0]);
        size_t hash2 = std::hash<Vector2f::Scalar>()(v[1]);
        return hash1 ^ (hash2 << 1);
    }
};

void load_standard_data_sample(
    FILE* file,
    MatrixXf &rawPoints,
    MatrixXf  &V2D,
    Metadata &metadata
);
void load_spectral_data_sample(
    FILE* file,
    MatrixXf &rawPoints,
    MatrixXf  &V2D,
    Metadata &metadata
);

void load_data_sample(
    const std::string& fileName,
    MatrixXf &rawPoints,
    MatrixXf  &V2D,
    VectorXu8 &selectedPoints,
    Metadata &metadata
)
{
    START_PROFILING("Loading data sample");
    // try open file
    FILE* file = fopen(fileName.c_str(), "r");
    if (!file)
        throw runtime_error("Unable to open file " + fileName);

    unsigned int lineNumber = 0;
    const size_t MAX_LENGTH = 512;
    char line[MAX_LENGTH];
    while (!feof(file) && !ferror(file) && fgets(line, MAX_LENGTH, file))
    {
        ++lineNumber;

        // remove any trailing spaces (this will detect full of spaces lines)
        const char* head = line;
        while (isspace(*head)) ++head;

        if (*head == '\0')
        {
            // skip empty lines
        }
        else if (*head == '#')
        {
            metadata.addLine(head);
        }
        else
        {
            fseek(file, 0, SEEK_SET);

            metadata.initInfos();
            if (metadata.isSpectralData())
                load_spectral_data_sample(file, rawPoints, V2D, metadata);
            else
                load_standard_data_sample(file, rawPoints, V2D, metadata);

            selectedPoints.resize(metadata.pointsInFile());
            selectedPoints.setZero();
            break;
        }
    }
    END_PROFILING();
}

void load_standard_data_sample(
    FILE* file,
    MatrixXf &rawPoints,
    MatrixXf  &V2D,
    Metadata &metadata
)
{
    unordered_set<Vector2f, Vector2fHash> readVertices;

    V2D.resize(2, metadata.pointsInFile());
    rawPoints.resize(3, metadata.pointsInFile());

    unsigned int lineNumber = 0;
    unsigned int nPoints = 0;
    const size_t MAX_LENGTH = 512;
    char line[MAX_LENGTH];
    while (!feof(file) && !ferror(file) && fgets(line, MAX_LENGTH, file))
    {
        ++lineNumber;

        // remove any trailing spaces (this will detect full of spaces lines)
        const char* head = line;
        while (isspace(*head)) ++head;

        if (*head == '\0' || *head == '#')
        {
            // skip empty/comment lines
        }
        else
        {
            float theta, phi, intensity;
            if (sscanf(head, "%f %f %f", &theta, &phi, &intensity) != 3)
            {
                fclose(file);
                ostringstream errorMsg;
                errorMsg << "Invalid file format: " << head << " (line " << lineNumber << ")";
                throw runtime_error(errorMsg.str());
            }
            Vector2f p2d = Vector2f{ theta, phi };
            if (readVertices.count(p2d) != 0)
            {
                cerr << "Warning: found two points with exact same coordinates\n";
                continue;
            }
            readVertices.insert(p2d);
            Vector2f transformedPoint = transformRawPoint(p2d);
            rawPoints.col(nPoints) = Vector3f{ theta, phi, intensity };
            V2D.col(nPoints) = transformedPoint;
            ++nPoints;
        }
    }
    fclose(file);
}
void load_spectral_data_sample(
    FILE* file,
    MatrixXf &rawPoints,
    MatrixXf  &V2D,
    Metadata &metadata
)
{
    int nDataPointsPerLoop = metadata.dataPointsPerLoop();
    vector<vector<float>> rawData;
    vector<Vector2f> v2d;

    unordered_set<Vector2f, Vector2fHash> readVertices;

    unsigned int lineNumber = 0;
    unsigned int nPoints = 0;
    const size_t MAX_LENGTH = 8192;
    char line[MAX_LENGTH];
    while (!feof(file) && !ferror(file) && fgets(line, MAX_LENGTH, file))
    {
        ++lineNumber;

        // remove any trailing spaces (this will detect full of spaces lines)
        const char* head = line;
        while (isspace(*head)) ++head;

        if (*head == '\0' || *head == '#')
        {
            // skip empty/comment lines
        }
        else
        {
            Vector2f angles;
            if (sscanf(head, "%f %f", &angles[0], &angles[1]) != 2)
            {
                fclose(file);
                ostringstream errorMsg;
                errorMsg << "Invalid file format: " << head << " (line " << lineNumber << ")";
                throw runtime_error(errorMsg.str());
            }
            if (readVertices.count(angles) > 0)
            {
                cerr << "Warning: found two points with exact same coordinates\n";
                continue;                                   // skip similar points
            }
            readVertices.insert(angles);

            v2d.push_back(transformRawPoint(angles));
            rawData.push_back(vector<float>{});
            rawData.back().resize(nDataPointsPerLoop + 2, 0);
            rawData.back()[0] = angles[0];
            rawData.back()[1] = angles[1];

            for (int i = 0; i < nDataPointsPerLoop; ++i)
            {
                float value;
                
                if (nullptr == (head = strchr(head, ' ')) ||
                    1 != sscanf(head, "%f", &value))
                {
                    fclose(file);
                    ostringstream errorMsg;
                    errorMsg << "Invalid file format: " << head << " (line " << lineNumber << ")";
                    throw runtime_error(errorMsg.str());
                }
                rawData[nPoints][i+2] = value;
            }
            ++nPoints;
        }
    }
    metadata.setPointsInFile(nPoints);

    rawPoints.resize(nDataPointsPerLoop + 2, nPoints);
    for (size_t i = 0; i < nPoints; i++)
    {
        memcpy(rawPoints.col(i).data(), rawData[i].data(), sizeof(float) * (nDataPointsPerLoop + 2));
    }

    V2D.resize(2, nPoints);
    memcpy(V2D.data(), v2d.data(), sizeof(Vector2f) * nPoints);

    fclose(file);
}

void save_data_sample(
    const std::string& path,
    const MatrixXf &rawPoints,
    const Metadata &metadata
)
{
    START_PROFILING("Save data sample");
    // try open file
    FILE* datasetFile = fopen(path.c_str(), "w");
    if (!datasetFile)
        throw runtime_error("Unable to open file " + path);

    // save metadata
	for(const auto& line: metadata.rawMetadata())
		fprintf(datasetFile, "%s\n", line.c_str());

    //!feof(datasetFile) && !ferror(datasetFile))
    for (Eigen::Index i = 0; i < rawPoints.cols(); ++i)
    {
        fprintf(datasetFile, "%lf %lf %lf\n", rawPoints(0, i), rawPoints(1, i), rawPoints(2, i));
    }
    fclose(datasetFile);
    END_PROFILING();
}

TEKARI_NAMESPACE_END