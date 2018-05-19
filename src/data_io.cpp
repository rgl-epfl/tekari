#include "tekari/data_io.h"

#include "tekari/stop_watch.h"

TEKARI_NAMESPACE_BEGIN

using namespace std;
using namespace nanogui;

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
    FILE* datasetFile = fopen(fileName.c_str(), "r");
    if (!datasetFile)
        throw runtime_error("Unable to open file " + fileName);

    unsigned int lineNumber = 0;
    unsigned int pointN = 0;
    const size_t MAX_LENGTH = 512;
    char line[MAX_LENGTH];
    bool readingMetadata = true;
    while (!feof(datasetFile) && !ferror(datasetFile) && fgets(line, MAX_LENGTH, datasetFile))
    {
        ++lineNumber;

        // remove any trailing spaces (this will detect full of spaces lines)
        const char* head = line;
        while (isspace(*head)) ++head;

        if (*head == '\0')
        {
            // skip empty lines
        }
        else if (*head == '#' && readingMetadata)
        {
            metadata.addLine(head);
        }
        else
        {
            if (readingMetadata)
            {
                readingMetadata = false;
                metadata.initInfos();
                int nPoints = metadata.pointsInFile();
                if (nPoints >= 0)
                {
                    // as soon as we know the total size of the dataset, reserve enough space for it
                    V2D.resize(2, nPoints);
                    selectedPoints.resize(nPoints);
                    rawPoints.resize(3, nPoints);
                }
            }
            float phi, theta, intensity;
            if (sscanf(head, "%f %f %f", &theta, &phi, &intensity) != 3)
            {
                fclose(datasetFile);
                ostringstream errorMsg;
                errorMsg << "Invalid file format: " << head << " (line " << lineNumber << ")";
                throw runtime_error(errorMsg.str());
            }
            Vector3f rawPoint = Vector3f{ theta, phi, intensity };
            Vector2f transformedPoint = transformRawPoint(rawPoint);
            rawPoints.col(pointN) = rawPoint;
            V2D.col(pointN) = transformedPoint;
            selectedPoints(pointN) = false;
            ++pointN;
        }
    }
    fclose(datasetFile);
    END_PROFILING();
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