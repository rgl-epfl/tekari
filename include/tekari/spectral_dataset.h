#pragma once

#include <tekari/dataset.h>
#include <tekari/raw_data_processing.h>

TEKARI_NAMESPACE_BEGIN

class StandardDataset : public Dataset
{
public:
    StandardDataset(const string &file_path)
    {
        load_dataset(file_path, m_raw_measurement, m_v2d, m_selected_points, m_metadata);
        recompute_data();
    }

    virtual void set_intensity_index(size_t intensity_index) override
    {
        m_intensity_index = std::min(intensity_index, m_raw_measurement.n_wavelengths());;

        if (!m_cache_mask[m_intensity_index])
        {
            m_cache_mask[m_intensity_index] = true;

            compute_min_max_intensities(m_points_stats, m_raw_measurement, m_intensity_index);
            compute_normalized_heights(m_raw_measurement, m_points_stats, m_h, m_lh, m_intensity_index);
            update_points_stats(m_points_stats, m_raw_measurement, m_v2d, m_h, m_lh, m_intensity_index);
            m_selection_stats.reset(m_raw_measurement.n_wavelengths() + 1);
            update_selection_stats(m_selection_stats, m_selected_points, m_raw_measurement, m_v2d, m_h, m_lh, m_intensity_index);
            compute_normals(m_f, m_v2d, m_h, m_lh, m_n, m_ln, m_intensity_index);
        }
        update_shaders_data();
        m_selection_axis.set_origin(selection_center());
    }

    virtual void init() override
    {
        Dataset::init();
        link_data_to_shaders();
        set_intensity_index(n_wavelengths() / 2);
    }

private:
};

TEKARI_NAMESPACE_END