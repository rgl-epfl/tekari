#define POWITACQ_IMPLEMENTATION
#include <tekari/bsdf_data_sample.h>
#include <tekari/stop_watch.h>

TEKARI_NAMESPACE_BEGIN

#define N_PHI 64
#define N_THETA 64
#define N_WAVE_LENGTHS 256

inline powitacq::Vector3f enoki_to_powitacq_vec3(const Vector3f& v) { return powitacq::Vector3f(v[0], v[1], v[2]); }
inline Vector3f powitacq_to_enoki_vec3(const powitacq::Vector3f& v) { return Vector3f(v[0], v[1], v[2]); }

BSDFDataSample::BSDFDataSample(const string& file_path)
: m_brdf(file_path)
{
    m_raw_measurement.resize(N_PHI * N_THETA + N_PHI, N_WAVE_LENGTHS);
    m_v2d.resize(N_PHI * N_THETA + N_PHI);
    compute_samples({0.0f, 0.0f});

    // artificially assign metadata members
    m_metadata.set_sample_name(file_path.substr(file_path.find_last_of("/") + 1, file_path.find_last_of(".")));
    m_metadata.set_points_in_file(m_raw_measurement.n_sample_points());
    m_selected_points.assign(m_raw_measurement.n_sample_points(), NOT_SELECTED_FLAG);

}

void BSDFDataSample::set_incident_angle(const Vector2f& incident_angle)
{
    // clear mask
    std::fill(m_cache_mask.begin(), m_cache_mask.end(), false);

    m_metadata.set_incident_angle(incident_angle);
    compute_samples(incident_angle);
    set_intensity_index(m_intensity_index);
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

            Vector2f outgoing_angle = vec3_to_hemisphere(powitacq_to_enoki_vec3(wo));

            samples *= pdf;
            RawMeasurement::SamplePoint sample_point = m_raw_measurement[j*N_THETA + i];
            sample_point.set_theta(outgoing_angle[0]);
            sample_point.set_phi(outgoing_angle[1]);
            sample_point.set_luminance(samples[0]);
            memcpy(sample_point.data() + 3, &samples[0], samples.size() * sizeof(float));
            m_v2d[j*N_THETA + i] = vec3_to_disk(powitacq_to_enoki_vec3(wo));
        }
    }
    // add outter ring to complete mesh
    for (int j = 0; j < N_PHI; ++j)
    {
        float theta = 90.0f;
        float phi = 360.0f * j / N_PHI;
        RawMeasurement::SamplePoint sample_point = m_raw_measurement[N_PHI*N_THETA + j];
        sample_point.set_theta(theta);
        sample_point.set_phi(phi);
        memset(sample_point.data() + 2, 0, (N_WAVE_LENGTHS + 1) * sizeof(float));
        m_v2d[N_PHI*N_THETA + j] = hemisphere_to_disk({theta, phi});
    }
    END_PROFILING();

    recompute_data();
}

TEKARI_NAMESPACE_END