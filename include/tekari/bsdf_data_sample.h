#pragma once

#include <tekari/data_sample.h>
#include <tekari/powitacq.h>

TEKARI_NAMESPACE_BEGIN

class BSDFDataSample : public DataSample
{
public:

    BSDFDataSample(const string& file_path);
    virtual void init() override;
    virtual void set_incident_angle(const Vector2f& incident_angle) override;
    virtual void set_intensity_index(size_t displayed_wavelength) override;
    virtual inline string wavelength_str() override
    {
        if (m_intensity_index == 0)
            return string("luminance");

        return to_string(m_brdf.wavelengths()[m_intensity_index-1]) + string(" nm");
    }

    virtual vector<float> get_selection_spectrum() override
    {
        size_t point_index = m_selection_stats[m_intensity_index].highest_point_index;
        powitacq::Spectrum s = m_brdf.sample_state(point_index);

        vector<float> result;
        result.reserve(s.size());

        float max = -std::numeric_limits<float>::max();
        for(size_t i = 0; i < s.size(); ++i)
        {
            if (m_brdf.wavelengths()[i] > 360.0f && m_brdf.wavelengths()[i] < 1000.0f)
            {
                result.push_back(s[i]);
                max = std::max(max, s[i]);
            }
        }
        // normalize result
        for(size_t i = 0; i < result.size(); ++i)
        {
            result[i] /= max;
        }
        return result;
    }


private:
    void compute_samples();

    powitacq::BRDF m_brdf;
    size_t m_n_theta;
    size_t m_n_phi;
};

TEKARI_NAMESPACE_END