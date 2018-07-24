#pragma once

#include <tekari/data_sample.h>

TEKARI_NAMESPACE_BEGIN

class TestDataSample : public DataSample
{
public:

    // constructors/destructors, assignement operators
    TestDataSample(const string& file_path)
    : DataSample(file_path)
    {}

    virtual void set_incident_angle(const Vector2f& incident_angle) override
    {
        m_metadata.set_incident_angle(incident_angle);
     
        m_raw_points.clear();
        m_v2d.clear();
        int n_theta = 20;
        int n_phi = 20;
        for (int i = 0; i < n_theta; ++i)
        {
            float theta = (i + 1) * 90.f / n_theta;
            for (int j = 0; j < n_phi; ++j)
            {
                float phi = j * 360.0f / n_phi;
                Vector2f v2d = transform_raw_point({theta, phi});
                float h = eva_gaussian(transform_raw_point(incident_angle), v2d);
                m_raw_points.push_back({theta, phi, h});
                m_v2d.push_back(v2d);
            }
        }

        recompute_data();
        link_data_to_shaders();
    }

private:

    static float eva_gaussian(const Vector2f& c, const Vector2f& p)
    {
        Vector2f diff_spread = (p - c) * 5.0f;
        return exp(-enoki::squared_norm(diff_spread));
    }
};

TEKARI_NAMESPACE_END