#pragma once

#include <tekari/data_sample.h>

TEKARI_NAMESPACE_BEGIN

class BSDFDataSample : public DataSample
{
public:

    // constructors/destructors, assignement operators
    BSDFDataSample(const string& file_path)
    : m_brdf(file_path)
    {
        m_metadata.set_incident_angle({0.0f, 0.0f});
        compute_samples({0.0f, 0.0f});
    }

    virtual void set_incident_angle(const Vector2f& incident_angle) override
    {
        m_metadata.set_incident_angle(incident_angle);
        compute_samples(incident_angle);
        link_data_to_shaders();
    }

private:

    void compute_samples(const Vector2f& incident_angle)
    {
        m_raw_points.clear();
        m_v2d.clear();
        // vector<powitacq::vec3> wos;
        int n_theta = 32;
        int n_phi = 32;
        for (int i = 0; i < n_theta; ++i)
        {
            float u = float(i) / n_theta;
            for (int j = 0; j < n_phi; ++j)
            {
                float v = float(j) / n_phi;
                powitacq::vec3 wo;
                auto samples = m_brdf.sample(powitacq::vec2(u, v), enoki_to_powitacq_vec3(hemisphere_to_vec3(incident_angle)), &wo);

                // wos.push_back(wo);

                VectorXf raw_point(samples.size() + 2);
                Vector2f outgoing_angle = vec3_to_hemisphere(powitacq_to_enoki_vec3(wo));
                raw_point[0] = outgoing_angle[0];
                raw_point[1] = outgoing_angle[1];
                for(size_t i = 0; i < samples.size(); ++i)
                    raw_point[i+2] = samples[i];

                m_raw_points.push_back(raw_point);
                m_v2d.push_back(vec3_to_disk(powitacq_to_enoki_vec3(wo)));
            }
        }
        recompute_data();
    }

    powitacq::brdf m_brdf;

};

TEKARI_NAMESPACE_END