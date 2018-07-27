#define POWITACQ_IMPLEMENTATION
#include <tekari/bsdf_data_sample.h>
#include <tekari/stop_watch.h>

TEKARI_NAMESPACE_BEGIN

#define N_PHI 32
#define N_THETA 32
#define N_WAVE_LENGTHS 256

inline powitacq::Vector3f enoki_to_powitacq_vec3(const Vector3f& v) { return powitacq::Vector3f(v[0], v[1], v[2]); }
inline Vector3f powitacq_to_enoki_vec3(const powitacq::Vector3f& v) { return Vector3f(v[0], v[1], v[2]); }

static float eval_gaussian(const Vector2f& c, const Vector2f& p)
{
    Vector2f diff_spread = (p - c) * 5.0f;
    return exp(-enoki::squared_norm(diff_spread));
}

BSDFDataSample::BSDFDataSample(const string& file_path)
: m_brdf(file_path)
{
    m_raw_measurement.resize(N_PHI * N_THETA, N_WAVE_LENGTHS);
    compute_samples({0.0f, 0.0f});

    // artificially assign metadata members
    m_metadata.set_sample_name(file_path.substr(file_path.find_last_of("/") + 1, file_path.find_last_of(".")));
    m_metadata.set_points_in_file(m_raw_measurement.n_sample_points());
    m_selected_points.assign(m_raw_measurement.n_sample_points(), NOT_SELECTED_FLAG);

}

void BSDFDataSample::set_incident_angle(const Vector2f& incident_angle)
{
    m_metadata.set_incident_angle(incident_angle);
    compute_samples(incident_angle);
    update_point_selection();
    link_data_to_shaders();
}

void BSDFDataSample::compute_samples(const Vector2f& incident_angle)
{
    START_PROFILING("Sample brdf");
    for (int j = 0; j < N_PHI; ++j)
    {
        float v = float(j + 0.5f) / N_PHI;
        for (int i = 0; i < N_THETA; ++i)
        {
            float u = float(i + 0.5f) / N_THETA;
            powitacq::Vector3f wo;
            float pdf;
            auto samples = m_brdf.sample(powitacq::Vector2f(u, v), enoki_to_powitacq_vec3(hemisphere_to_vec3(incident_angle)), &wo, &pdf);

            // printf("[%f, %f] -> [%f, %f, %f]\n", u, v, wo[0], wo[1], wo[2]);

            Vector2f outgoing_angle = vec3_to_hemisphere(powitacq_to_enoki_vec3(wo));

            memcpy(m_raw_measurement[j*N_THETA + i].data(), &samples[0], samples.size());
            m_v2d.push_back(vec3_to_disk(powitacq_to_enoki_vec3(wo)));
        }
    }

    // m_raw_measurement.clear();
    // m_v2d.clear();
    // int n_theta = 20;
    // int n_phi = 20;
    // for (int i = 0; i < n_theta; ++i)
    // {
    //     float theta = (i + 1) * 90.f / n_theta;
    //     for (int j = 0; j < n_phi; ++j)
    //     {
    //         float phi = j * 360.0f / n_phi;
    //         Vector2f v2d = hemisphere_to_disk({theta, phi});
    //         float h = eval_gaussian(hemisphere_to_disk(incident_angle), v2d);
    //         m_raw_measurement.push_back({theta, phi, h});
    //         m_v2d.push_back(v2d);
    //     }
    // }

    END_PROFILING();

    recompute_data();

    Log(Info, "Number of triangles normals to compute : %zu\n", m_brdf.wavelengths().size() * 2 * m_f.size());
}

TEKARI_NAMESPACE_END