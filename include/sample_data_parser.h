#pragma once

#include <vector>
#include <nanogui/nanogui.h>
#include <iomanip>
#include <fstream>
#include <string>

class SampleDataParser : public std::ostream, std::streambuf
{
private:
    typedef enum {
        SKIP_FIRST_LINE,
        READ_INFO,
        READ_POINTS,
        READ_FACES,
        UNDEFINED
    } SampleDataParserState;
    
public:
    SampleDataParser();
    SampleDataParser(const std::string& sampleDataPath);

    SampleDataParser(const SampleDataParser&) = delete;
    SampleDataParser(const SampleDataParser&& other);

    const SampleDataParser& operator=(const SampleDataParser&) = delete;
    const SampleDataParser& operator=(const SampleDataParser&& other) = delete;

    bool loadFromFile(const std::string& sampleDataPath);
    void linkDataToShaders( nanogui::GLShader &normalShader,
                            nanogui::GLShader &logShader,
                            nanogui::GLShader &pathShader);
    unsigned int getNFaces() const { return m_FaceCount; }

private:
    bool readDataset(const std::string &filePath);
    int overflow(int ch);
    void extractIndicesFromCurrentWord();
    void printInfo();

    inline nanogui::Vector3f getVertex(unsigned int i) const { return {m_Vertices[i][0], m_Heights[i], m_Vertices[i][1]}; }
    inline nanogui::Vector3f computeNormal(unsigned int i0, unsigned int i1, unsigned int i2) const
    {
        return (getVertex(i2)-getVertex(i0)).cross(getVertex(i1)-getVertex(i0)).normalized();
    }

    inline nanogui::Vector3f getLogVertex(unsigned int i) const { return {m_Vertices[i][0], m_LogHeights[i], m_Vertices[i][1]}; }
    inline nanogui::Vector3f computeLogNormal(unsigned int i0, unsigned int i1, unsigned int i2) const
    {
        return (getLogVertex(i2)-getLogVertex(i0)).cross(getLogVertex(i1)-getLogVertex(i0)).normalized();
    }

private:
    // sampe data to be constructed from the input provided
    std::vector<nanogui::Vector2f>  m_Vertices;
    std::vector<float>              m_Heights;
    std::vector<float>              m_LogHeights;
    std::vector<nanogui::Vector3f>  m_Normals;
    std::vector<nanogui::Vector3f>  m_LogNormals;
    std::vector<unsigned int>       m_Indices;

    unsigned int m_VertexCount;
    unsigned int m_FaceCount;

    SampleDataParserState m_State;
    std::stringstream m_CurrentWord;
    unsigned int m_CurrentVertexCount;
    std::vector<unsigned int> m_NormalPerVertexCount;

    bool m_AlreadyLoaded;
};