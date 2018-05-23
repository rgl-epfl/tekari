#include "tekari/data_io.h"

#include "tekari/stop_watch.h"

TEKARI_NAMESPACE_BEGIN

using namespace std;
using namespace nanogui;

void load_standard_data_sample(
    FILE* file,
    MatrixXf &rawPoints,
    MatrixXf  &V2D,
    VectorXu8 &selectedPoints,
    Metadata &metadata
);
void load_spectral_data_sample(
    FILE* file,
    MatrixXf &rawPoints,
    MatrixXf  &V2D,
    VectorXu8 &selectedPoints,
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
    unsigned int pointN = 0;
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
            {
                load_spectral_data_sample(file, rawPoints, V2D, selectedPoints, metadata);
            }
            else
            {
                load_standard_data_sample(file, rawPoints, V2D, selectedPoints, metadata);
            }
        }
    }
    END_PROFILING();
}

void load_standard_data_sample(
    FILE* file,
    MatrixXf &rawPoints,
    MatrixXf  &V2D,
    VectorXu8 &selectedPoints,
    Metadata &metadata
)
{
    V2D.resize(2, metadata.pointsInFile());
    rawPoints.resize(3, metadata.pointsInFile());
    selectedPoints.resize(metadata.pointsInFile());
    selectedPoints.setZero();

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
            float phi, theta, intensity;
            if (sscanf(head, "%f %f %f", &theta, &phi, &intensity) != 3)
            {
                fclose(file);
                ostringstream errorMsg;
                errorMsg << "Invalid file format: " << head << " (line " << lineNumber << ")";
                throw runtime_error(errorMsg.str());
            }
            Vector2f transformedPoint = transformRawPoint(Vector2f{ theta, phi });
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
    VectorXu8 &selectedPoints,
    Metadata &metadata
)
{
    int nDataPointsPerLoop = metadata.dataPointsPerLoop();
    vector<vector<float>> rawData;
    vector<Vector2f> v2d;

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
            v2d.push_back(Vector2f{0, 0});
            rawData.push_back(vector<float>{});
            rawData.back().resize(nDataPointsPerLoop + 2, 0);
            for (int i = 0; i < nDataPointsPerLoop + 2; ++i)
            {
                float value;
                int valid_reads = sscanf(head, "%f", &value);
                const char* new_head = head;
                if (i != nDataPointsPerLoop + 1)
                {
                    if ((new_head = strchr(head, ' ')) == nullptr)
                        new_head = strchr(head, '\t');
                }

                if (valid_reads != 1 || new_head == nullptr)
                {
                    fclose(file);
                    ostringstream errorMsg;
                    errorMsg << "Invalid file format: " << head << " (line " << lineNumber << ")";
                    throw runtime_error(errorMsg.str());
                }
                rawData[nPoints][i] = value;
            }
            Vector2f transformedPoint = transformRawPoint(Vector2f{ rawData[nPoints][0], rawData[nPoints][1] });
            v2d[nPoints] = transformedPoint;
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

    selectedPoints.resize(nPoints);
    selectedPoints.setZero();

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
    fprintf(datasetFile, "%s", metadata.toString().c_str());

    //!feof(datasetFile) && !ferror(datasetFile))
    for (Eigen::Index i = 0; i < rawPoints.cols(); ++i)
    {
        fprintf(datasetFile, "%lf %lf %lf\n", rawPoints(0, i), rawPoints(1, i), rawPoints(2, i));
    }
    fclose(datasetFile);
    END_PROFILING();
}

TEKARI_NAMESPACE_END