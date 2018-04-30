#include "tekari/Metadata.h"

#include <functional>

TEKARI_NAMESPACE_BEGIN

using namespace std;

void Metadata::parse(const std::string& line)
{
    size_t firstTab = line.find_first_of('\t');
    if (firstTab == string::npos)
        return;

    string tagInFile{ line.substr(1, firstTab - 1) };
    string restOfLine{ line.substr(firstTab + 1, line.size() - firstTab - 2) };

    vector<pair<string, function<void(void)>>> matches =
    {
        { "mountain-version",   [this, &restOfLine]() { addElement("Mountain Version", restOfLine); } },
        { "measured_at",        [this, &restOfLine]() { addElement("Measured At", restOfLine); } },
        { "data_read_from_db",  [this, &restOfLine]() { addElement("Data Read From Database At", restOfLine); } },
        { "sample_label",       [this, &restOfLine]() { addElement("Sample Label", restOfLine); } },
        { "sample_name",        [this, &restOfLine]() { addElement("Sample Name", restOfLine); } },
        { "lamp",               [this, &restOfLine]() { addElement("Lamp", restOfLine); } },
        { "database-host",      [this, &restOfLine]() { addElement("Mountain Version", restOfLine); } },
        { "database-name",      [this, &restOfLine]() { addElement("Mountain Version", restOfLine); } },
        { "dump-host",          [this, &restOfLine]() { addElement("Mountain Version", restOfLine); } },

        { "intheta",                [this, &restOfLine]() { addElement("Incident Theta", stof(restOfLine)); } },
        { "inphi",                  [this, &restOfLine]() { addElement("Incident Phi", stof(restOfLine)); } },
        { "front_integral",         [this, &restOfLine]() { addElement("Front Integral", stof(restOfLine)); } },
        { "mountain_minimum_angle", [this, &restOfLine]() { addElement("Mountain Minimum Angle", stof(restOfLine)); } },

        { "database_id",            [this, &restOfLine]() { addElement("Database ID", stoi(restOfLine)); } },
        { "datapoints_in_db",       [this, &restOfLine]() { addElement("Data Points In Database", stoi(restOfLine)); } },
        { "datapoints_in_file",     [this, &restOfLine]() { addElement("Data Points In File", stoi(restOfLine)); } },

        { "database_reference_id",  [this, &restOfLine]() { addElement("Database ID", stoi(restOfLine)); } },
        { "integrated_value",       [this, &restOfLine]() { addElement("Database ID", stoi(restOfLine)); } },
        { "beamshape",              [this, &restOfLine]() { addElement("Database ID", stoi(restOfLine)); } },
        { "filterlamp",             [this, &restOfLine]() { addElement("Database ID", stoi(restOfLine)); } },
        { "filterdet",              [this, &restOfLine]() { addElement("Database ID", stoi(restOfLine)); } },

    };

    for (const auto& match : matches)
    {

    }
}

template<typename T>
void Metadata::addElement(const std::string& name, const T& value)
{
    m_Elements.push_back(make_shared<MetadataElement<T>>(name, value));
}

template<>
void Metadata::addElement<string>(const string& name, const string& value)
{
    m_Elements.push_back(make_shared<MetadataElement<string>>(name, stripQuoteMarks(value)));
}

template<typename T>
T Metadata::getElementValue(const std::string& name) const
{
    for (const auto& element : m_Elements)
    {
        if (element->name() == name)
        {
            auto e = dynamic_cast<const MetadataElement<T>*>(element.get());
            return e->getValue();
        }
    }
    throw runtime_error("Element not found !");
}

TEKARI_NAMESPACE_END