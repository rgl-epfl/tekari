#pragma once

#include <tekari/common.h>

#include <vector>

TEKARI_NAMESPACE_BEGIN

class Metadata
{
public:
    Metadata();

    void add_line(const string& line);
    void init_infos();

    bool is_spectral() const { return m_is_spectral; }
    float incident_theta() const { return m_in_theta; }
    float incident_phi() const { return m_in_phi; }
    const string& sample_name() const { return m_sample_name; }
    int points_in_file() const { return m_points_in_file; }
    int data_points_per_loop() const { return m_data_points_per_loop; }
    void set_points_in_file(int points_in_file);

    inline const vector<string>& raw_metadata() const { return m_raw_metadata; }

private:
    string* find_line_containing(const string& target);
    string* find_line_starting_with(const string& target);
    static string strip_quote_marks(const string& word) { return trim_copy(word, [](int c){ return c == '\"' || std::isspace(c); }); }

    vector<string> m_raw_metadata;
    bool m_is_spectral;
    float m_in_theta;
    float m_in_phi;
    string m_sample_name;
    int m_points_in_file;
    int m_data_points_per_loop;
};

TEKARI_NAMESPACE_END