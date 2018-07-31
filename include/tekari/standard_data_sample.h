#pragma once

#include <tekari/data_sample.h>
#include <tekari/raw_data_processing.h>

TEKARI_NAMESPACE_BEGIN

class StandardDataSample : public DataSample
{
public:
    StandardDataSample(const string &file_path)
    {
        load_data_sample(file_path, m_raw_measurement, m_v2d, m_selected_points, m_metadata);
        recompute_data();
    }

    virtual void set_intensity_index(size_t intensity_index) override
    {
        m_intensity_index = std::min(intensity_index, m_raw_measurement.n_wave_lengths());;

        if (!m_cache_mask[m_intensity_index])
        {
            m_cache_mask[m_intensity_index] = true;

            compute_min_max_intensities(m_points_stats, m_raw_measurement, m_intensity_index);

            compute_normalized_heights(
                m_raw_measurement,
                m_points_stats,
                m_h[m_intensity_index],
                m_lh[m_intensity_index],
                m_intensity_index);

            update_points_stats(m_points_stats, m_raw_measurement, m_v2d, m_h[m_intensity_index], m_lh[m_intensity_index], m_intensity_index);
            m_selection_stats.reset(m_raw_measurement.n_wave_lengths() + 1);
            update_selection_stats(m_selection_stats, m_selected_points, m_raw_measurement, m_v2d, m_h[m_intensity_index], m_lh[m_intensity_index], m_intensity_index);

            compute_normals(
                m_f, m_v2d,
                m_h[m_intensity_index],
                m_lh[m_intensity_index],
                m_n[m_intensity_index],
                m_ln[m_intensity_index]);
        }
        update_shaders_data();
        m_selection_axis.set_origin(selection_center());
    }
};

TEKARI_NAMESPACE_END