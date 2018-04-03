#pragma once

#include <string>
#include <iostream>

struct Metadata
{
	std::string mountainVersion;
	std::string databaseHost;
	std::string databaseName;
	std::string dataReadFromDatabaseAt;
	std::string measuredAt;
	std::string dumpHost;
	std::string sampleLabel;
	std::string sampleName;
	std::string lamp;

	float mountainMinimumAngle	= 0.0f;
	float incidentTheta			= 0.0f;
	float incidentPhi			= 0.0f;
	float frontIntegral			= 0.0f;

	int databaseId				= -1;
	int datapointsInDatabase	= -1;
	int datapointsInFile		= -1;

	void parse(const std::string& line)
	{
		size_t firstTab = line.find_first_of('\t');
		if (firstTab == std::string::npos)
			return;

		std::string tagInFile{ line.substr(1, firstTab - 1) };
		std::string restOfLine{ line.substr(firstTab + 1, line.size() - firstTab - 2) };
		
		if		(tagInFile == "mountain-version")		{ mountainVersion			= stripQuoteMarks(restOfLine); }
		else if (tagInFile == "measured_at")			{ measuredAt				= stripQuoteMarks(restOfLine); }
		else if (tagInFile == "data_read_from_db")		{ dataReadFromDatabaseAt	= stripQuoteMarks(restOfLine); }
		else if (tagInFile == "sample_label")			{ sampleLabel				= stripQuoteMarks(restOfLine); }
		else if (tagInFile == "sample_name")			{ sampleName				= stripQuoteMarks(restOfLine); }
		else if (tagInFile == "lamp")					{ lamp						= stripQuoteMarks(restOfLine); }
		
		else if (tagInFile == "database-host")			{ databaseHost	= restOfLine; }
		else if (tagInFile == "database-name")			{ databaseName	= restOfLine; }
		else if (tagInFile == "dump-host")				{ dumpHost		= restOfLine; }
		
		else if (tagInFile == "intheta")				{ incidentTheta			= stof(restOfLine); }
		else if (tagInFile == "inphi")					{ incidentPhi			= stof(restOfLine); }
		else if (tagInFile == "front_integral")			{ frontIntegral			= stof(restOfLine); }
		else if (tagInFile == "mountain_minimum_angle") { mountainMinimumAngle	= stof(restOfLine); }

		else if (tagInFile == "database_id")			{ databaseId			= stoi(restOfLine); }
		else if (tagInFile == "datapoints_in_db")		{ datapointsInDatabase	= stoi(restOfLine); }
		else if (tagInFile == "datapoints_in_file")		{ datapointsInFile		= stoi(restOfLine); }
		
		else if (tagInFile == "database_reference_id")	{}
		else if (tagInFile == "integrated_value")		{}
		else if (tagInFile == "beamshape")				{}
		else if (tagInFile == "filterlamp")				{}
		else if (tagInFile == "filterdet")				{}
	}

private:
	static std::string stripQuoteMarks(const std::string& word) { return word.substr(1, word.size() - 2);; }
};