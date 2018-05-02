#pragma once

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include "common.h"
#include "MetadataElement.h"

TEKARI_NAMESPACE_BEGIN

class Metadata
{
public:
    Metadata();

    void addLine(const std::string& line);
    void initInfos();

    bool isSpectralData() const { return m_IsPectralData; }
    float incidentTheta() const { return m_InTheta; }
    float incidentPhi() const { return m_InPhi; }
    const std::string& sampleName() const { return m_SampleName; }
    int pointsInFile() const { return m_PointsInFile; }

    std::string toString() const;

private:
    const std::string* findLineContaining(const std::string &target) const;
    const std::string* findLineStartingWith(const std::string &target) const;
    static std::string stripQuoteMarks(const std::string& word) { return word.substr(1, word.size() - 2);; }

    std::vector<std::string> m_RawMetaData;
    bool m_IsPectralData;
    float m_InTheta;
    float m_InPhi;
    std::string m_SampleName;
    int m_PointsInFile;
};

TEKARI_NAMESPACE_END