#pragma once

#include <tekari/data_sample.h>

TEKARI_NAMESPACE_BEGIN

class StandardDataSample : public DataSample
{
public:
    StandardDataSample(const string &file_path)
    {
        load_data_sample(file_path, m_raw_measurement, m_v2d, m_selected_points, m_metadata);
        recompute_data();
    }
};

TEKARI_NAMESPACE_END