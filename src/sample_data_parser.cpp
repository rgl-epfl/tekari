#include "sample_data_parser.h"

#include <cstdint>
#include <limits>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <libqhullcpp/Qhull.h>

#include <iostream>
#include "stop_watch.h"

#define PI 3.14159

SampleDataParser::SampleDataParser(const std::string& sampleDataPath)
:   std::ostream(this)
,   m_VertexCount(0)
,   m_FaceCount(0)
,   m_State(SKIP_FIRST_LINE)
,   m_CurrentWord("")
,   m_CurrentVertexCount(0)
,   m_AlreadyLoaded(false)
{
    loadFromFile(sampleDataPath);
}

SampleDataParser::SampleDataParser()
:   std::ostream(this)
,   m_VertexCount(0)
,   m_FaceCount(0)
,   m_State(SKIP_FIRST_LINE)
,   m_CurrentWord("")
,   m_CurrentVertexCount(0)
,   m_AlreadyLoaded(false)
{}

SampleDataParser::SampleDataParser(const SampleDataParser&& other)
:   std::ostream(this)
,   m_Vertices(std::move(other.m_Vertices))
,   m_Heights(std::move(other.m_Heights))
,   m_LogHeights(std::move(other.m_LogHeights))
,   m_Normals(std::move(other.m_Normals))
,   m_LogNormals(std::move(other.m_LogNormals))
,   m_Indices(std::move(other.m_Indices))
,   m_VertexCount(other.m_VertexCount)
,   m_FaceCount(other.m_FaceCount)
,   m_State(other.m_State)
,   m_CurrentWord("")
,   m_CurrentVertexCount(other.m_CurrentVertexCount)
,   m_NormalPerVertexCount(std::move(other.m_NormalPerVertexCount))
,   m_AlreadyLoaded(other.m_AlreadyLoaded)
{
    std::cout <<"used move constructor" << std::endl;
}


bool SampleDataParser::loadFromFile(const std::string& sampleDataPath)
{
    if (m_AlreadyLoaded)
    {
        throw std::runtime_error("ERROR: cannot load sample data twice!");
    }
    double readDataElapsed;
    double runQhullElapsed;
    double outputQhullElapsed;

    // load vertex data
    PROFILE(readDataset(sampleDataPath), readDataElapsed);

    // triangulate it !
    const char* input_comment = "testing";
    const char* command = "-d -Qt";
    orgQhull::Qhull qhull;
    qhull.setOutputStream(&(*this));
    PROFILE(qhull.runQhull(input_comment, 2, m_Vertices.size(), (float*)m_Vertices.data(), command), runQhullElapsed);
    PROFILE(qhull.outputQhull("-o"), outputQhullElapsed);

    double totalElapsed = readDataElapsed + runQhullElapsed + outputQhullElapsed;
    std::cout << "Read data percentage " << (100 * readDataElapsed / totalElapsed) << "%" << std::endl;
    std::cout << "Run qhull percentage " << (100 * runQhullElapsed / totalElapsed) << "%" << std::endl;
    std::cout << "Output qhull percentage " << (100 * outputQhullElapsed / totalElapsed) << "%" << std::endl;

    m_AlreadyLoaded = true;
}

void SampleDataParser::linkDataToShaders(   nanogui::GLShader &normalShader,
                                            nanogui::GLShader &logShader,
                                            nanogui::GLShader &pathShader)
{
    if (!m_AlreadyLoaded)
    {
        throw std::runtime_error("ERROR: cannot link data to shader before loading.");
    }
    // TODO: not really efficient or pretty, but how to do better it using nanogui
    normalShader.bind();
    normalShader.uploadAttrib("in_normal", m_VertexCount, 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_Normals.data());
    normalShader.uploadAttrib("in_pos2d", m_VertexCount, 2, sizeof(nanogui::Vector2f), GL_FLOAT, GL_FALSE, (const void*)m_Vertices.data());
    normalShader.uploadAttrib("in_height", m_VertexCount, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)m_Heights.data());
    normalShader.uploadAttrib("indices", m_FaceCount, 3, 3 * sizeof(unsigned int), GL_UNSIGNED_INT, GL_FALSE, m_Indices.data());

    logShader.bind();
    logShader.uploadAttrib("in_normal", m_VertexCount, 3, sizeof(nanogui::Vector3f), GL_FLOAT, GL_FALSE, (const void*)m_LogNormals.data());
    logShader.shareAttrib(normalShader, "in_pos2d");
    logShader.uploadAttrib("in_height", m_VertexCount, 1, sizeof(float), GL_FLOAT, GL_FALSE, (const void*)m_LogHeights.data());
    logShader.shareAttrib(normalShader, "indices");

    pathShader.bind();
    pathShader.shareAttrib(normalShader, "in_pos2d");
    pathShader.shareAttrib(normalShader, "in_height");
    pathShader.shareAttrib(normalShader, "indices");
}

bool SampleDataParser::readDataset(const std::string &filePath)
{
    // read file
    float phi, theta, intensity;

    FILE* datasetFile = fopen(filePath.c_str(), "r");
    if (!datasetFile)
        return false;

    // min and max values for normalization
    float min_intensity = std::numeric_limits<float>::max();
    float max_intensity = std::numeric_limits<float>::min();

    // TODO change this to some smarter stuff
    const size_t MAX_LENGTH = 512;
    char line[MAX_LENGTH];
    while (fgets(line, MAX_LENGTH, datasetFile) && line[0] == '#')
        {}

    while (fscanf(datasetFile, "%f %f %f", &theta, &phi, &intensity) == 3)
    {
        float x = theta * cos(phi * PI / 180.0f) / 90;
        float z = theta * sin(phi * PI / 180.0f) / 90;

        m_Vertices.push_back({x, z});
        m_Heights.push_back(intensity);
        m_LogHeights.push_back(intensity);

        min_intensity = std::min(min_intensity, intensity);
        max_intensity = std::max(max_intensity, intensity);
    }
    fclose(datasetFile);

    // intensity normalization
    float min_log_intensity = log(min_intensity + 1);
    float max_log_intensity = log(max_intensity + 1);
    for(size_t i = 0; i < m_Heights.size(); ++i)
    {
        m_Heights[i]    = (m_Heights[i] - min_intensity) / (max_intensity - min_intensity);
        m_LogHeights[i] = (log(m_LogHeights[i] + 1) - min_log_intensity) / (max_log_intensity - min_log_intensity);
    }

    return true;
}

int SampleDataParser::overflow(int ch)
{
    char c = ch;
    m_CurrentWord << c;
    if (c == '\n')
    {
        switch (m_State)
        {
            case SKIP_FIRST_LINE:
            {
                m_CurrentWord.str(std::string());
                m_State = READ_INFO;
            }
                break;
            case READ_INFO:
            {
                unsigned int dummy;
                const std::string& tmpCurrWord = m_CurrentWord.str();
                if (sscanf(tmpCurrWord.c_str(), "%u %u %u", &m_VertexCount, &m_FaceCount, &dummy) == 3)
                {
                    m_Indices.reserve(3 * m_FaceCount);
                    m_NormalPerVertexCount.resize(m_VertexCount, 0);
                    m_Normals.resize(m_VertexCount, {0, 0, 0});
                    m_LogNormals.resize(m_VertexCount, {0, 0, 0});
                    m_CurrentWord.str(std::string());
                    m_State = READ_POINTS;
                }
                else
                {
                    m_State = UNDEFINED;
                }
            }
                break;
            case READ_POINTS:
            {
                ++m_CurrentVertexCount;
                m_CurrentWord.str(std::string());
                if (m_CurrentVertexCount == m_VertexCount)
                {
                    m_State = READ_FACES;
                }
            }
                break;
            case READ_FACES:
                extractIndicesFromCurrentWord();
                break;
            case UNDEFINED:
                throw new std::runtime_error("ERROR: unvalid OFF format");
        }
    }
    return 0;
}

void SampleDataParser::extractIndicesFromCurrentWord()
{
    unsigned int dummy, i0, i1, i2;
    const std::string& tmpCurrWord = m_CurrentWord.str();
    if (sscanf(tmpCurrWord.c_str(), "%u %u %u %u", &dummy, &i0, &i1, &i2) == 4)
    {
        m_Indices.push_back(i0);
        m_Indices.push_back(i1);
        m_Indices.push_back(i2);
        m_CurrentWord.str(std::string());

        // Compute normal
        nanogui::Vector3f faceNormal = computeNormal(i0, i1, i2);
        nanogui::Vector3f logFaceNormal = computeLogNormal(i0, i1, i2);

        ++m_NormalPerVertexCount[i0];
        ++m_NormalPerVertexCount[i1];
        ++m_NormalPerVertexCount[i2];
        m_Normals[i0] += faceNormal;
        m_Normals[i1] += faceNormal;
        m_Normals[i2] += faceNormal;

        m_LogNormals[i0] += logFaceNormal;
        m_LogNormals[i1] += logFaceNormal;
        m_LogNormals[i2] += logFaceNormal;

        // if we reached end of faces
        if (m_Indices.size() == m_FaceCount*3)
        {
            // normalize m_Normals
            for(size_t i = 0; i < m_Normals.size(); ++i)
            {
                m_Normals[i] /= m_NormalPerVertexCount[i];
                m_LogNormals[i] /= m_NormalPerVertexCount[i];
            }
            m_State = UNDEFINED;
        }
    }
    else
    {
        m_State = UNDEFINED;
    }
}

void SampleDataParser::printInfo()
{
    std::cout << "Number of m_Vertices : " << m_VertexCount << std::endl;
    for (int i = 0; i < m_VertexCount; ++i)
    {
        std::cout << m_Vertices[i][0] << " " << m_Vertices[i][1] << " " << m_Vertices[i][2] << std::endl;
    }
    std::cout << "Number of faces : " << m_FaceCount << std::endl;
    for (int i = 0; i < m_FaceCount; ++i)
    {
        std::cout << m_Indices[3*i + 0] << " " << m_Indices[3*i + 1] << " " << m_Indices[3*i + 2] << std::endl;
    }
}