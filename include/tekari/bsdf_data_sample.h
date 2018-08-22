#pragma once

#include <tekari/data_sample.h>
#include <tekari/powitacq.h>

TEKARI_NAMESPACE_BEGIN

class BSDFDataSample : public DataSample
{
public:

    BSDFDataSample(const string& file_path);
    virtual bool init() override;
    void set_incident_angle(const Vector2f& incident_angle);
    virtual void set_intensity_index(size_t displayed_wavelength) override;
    void set_sampling_resolution(size_t n_theta, size_t n_phi)
    {
        m_n_theta = n_theta;
        m_n_phi = n_phi;
        set_incident_angle(incident_angle());
    }

    pair<size_t, size_t> sampling_resolution() const { return make_pair(m_n_theta, m_n_phi); }

    virtual void get_selection_spectrum(vector<float> &spectrum) override;

private:
    void compute_samples();

    powitacq::BRDF m_brdf;
    size_t m_n_theta;
    size_t m_n_phi;
};

TEKARI_NAMESPACE_END
