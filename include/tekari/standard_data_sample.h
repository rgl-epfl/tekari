#pragma once

#include <tekari/data_sample.h>
#include <tekari/raw_data_processing.h>

TEKARI_NAMESPACE_BEGIN

class StandardDataSample : public DataSample
{
public:
    StandardDataSample(const string &file_path)
    {
        load_data_sample(file_path, m_raw_measurement, m_v2d, m_wavelengths, m_metadata);
        recompute_data();
    }

    virtual void get_selection_spectrum(vector<float> &spectrum) override
    {
        if (m_wavelengths.empty())
            return;

        spectrum.clear();
        spectrum.reserve(m_wavelengths.size());

        for (size_t i = 0; i < m_wavelengths.size(); ++i)
        {
            spectrum.push_back(m_raw_measurement(i + 3, m_intensity_index));
        }
    }


    virtual void set_intensity_index(size_t intensity_index) override
    {
        m_intensity_index = std::min(intensity_index, m_raw_measurement.n_wavelengths());;

        if (!m_cache_mask[m_intensity_index])
        {
            m_cache_mask[m_intensity_index] = true;

            compute_min_max_intensities(m_points_stats, m_raw_measurement, m_intensity_index);
            compute_normalized_heights(m_raw_measurement, m_points_stats, m_h, m_intensity_index);
            update_points_stats(m_points_stats, m_raw_measurement, m_v2d, m_h, m_intensity_index);
            update_selection_stats(m_selection_stats, m_selected_points, m_raw_measurement, m_v2d, m_h, m_intensity_index);
            compute_normals(m_f, m_v2d, m_h, m_n, m_intensity_index);
        }
        update_shaders_data();
    }

    virtual void delete_selected_points() override
    {
        tekari::delete_selected_points(m_selected_points, m_raw_measurement, m_v2d, m_selection_stats, m_metadata);
        recompute_data();
        link_data_to_shaders();

        // clear mask
        std::fill(m_cache_mask.begin(), m_cache_mask.end(), false);
        set_intensity_index(m_intensity_index);
    }

    virtual void save(const string& path) override
    {
        save_data_sample(path, m_raw_measurement, m_metadata);
    }

    virtual void init() override
    {
        DataSample::init();
        link_data_to_shaders();
        set_intensity_index(0);
    }
};

TEKARI_NAMESPACE_END