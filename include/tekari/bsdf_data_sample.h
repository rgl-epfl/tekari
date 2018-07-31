#pragma once

#include <tekari/data_sample.h>
#include <tekari/powitacq.h>

TEKARI_NAMESPACE_BEGIN

class BSDFDataSample : public DataSample
{
public:

    BSDFDataSample(const string& file_path);
    virtual void set_incident_angle(const Vector2f& incident_angle) override;
    virtual void set_intensity_index(size_t displayed_wave_length) override;

private:
    void compute_samples(const Vector2f& incident_angle);

    powitacq::BRDF m_brdf;

};

TEKARI_NAMESPACE_END