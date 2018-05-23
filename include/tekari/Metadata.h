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

    bool isSpectralData() const { return mIsSpectralData; }
    float incidentTheta() const { return mInTheta; }
    float incidentPhi() const { return mInPhi; }
    const std::string& sampleName() const { return mSampleName; }
    int pointsInFile() const { return mPointsInFile; }
    void setPointsInFile(int pointsInFile) { mPointsInFile = pointsInFile; }
    int dataPointsPerLoop() const { return mDataPointsPerLoop; }

    std::string toString() const;

private:
    const std::string* findLineContaining(const std::string &target) const;
    const std::string* findLineStartingWith(const std::string &target) const;
    static std::string stripQuoteMarks(const std::string& word) { return word.substr(1, word.size() - 2);; }

    std::vector<std::string> mRawMetaData;
    bool mIsSpectralData;
    float mInTheta;
    float mInPhi;
    std::string mSampleName;
    int mPointsInFile;
    int mDataPointsPerLoop;
};

TEKARI_NAMESPACE_END