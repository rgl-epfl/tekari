#pragma once

#include "common.h"

#include <string>
#include <vector>

TEKARI_NAMESPACE_BEGIN

class Metadata
{
public:
    Metadata();

    void add_line(const std::string& line);
    void init_infos();

    bool is_spectral() const { return m_is_spectral; }
    float incident_theta() const { return m_in_theta; }
    float incident_phi() const { return m_in_phi; }
    const std::string& sample_name() const { return m_sample_name; }
    int points_in_file() const { return m_points_in_file; }
    int data_points_per_loop() const { return m_data_points_per_loop; }
    void set_points_in_file(int points_in_file);

    inline const std::vector<std::string>& raw_metadata() const { return m_raw_metadata; }

private:
    std::string* find_line_containing(const std::string &target);
    std::string* find_line_starting_with(const std::string &target);
    static std::string strip_quote_marks(const std::string& word) { return word.substr(1, word.size() - 2); }

    std::vector<std::string> m_raw_metadata;
    bool m_is_spectral;
    float m_in_theta;
    float m_in_phi;
    std::string m_sample_name;
    int m_points_in_file;
    int m_data_points_per_loop;
};

TEKARI_NAMESPACE_END