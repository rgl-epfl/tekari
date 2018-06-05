#pragma once

#include <string>
#include <vector>
#include "common.h"

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
    int dataPointsPerLoop() const { return mDataPointsPerLoop; }
	void setPointsInFile(int pointsInFile);

	inline const std::vector<std::string>& rawMetadata() const { return mRawMetadata; }

private:
    std::string* findLineContaining(const std::string &target);
    std::string* findLineStartingWith(const std::string &target);
    static std::string stripQuoteMarks(const std::string& word) { return word.substr(1, word.size() - 2); }

    std::vector<std::string> mRawMetadata;
    bool mIsSpectralData;
    float mInTheta;
    float mInPhi;
    std::string mSampleName;
    int mPointsInFile;
    int mDataPointsPerLoop;
};

TEKARI_NAMESPACE_END