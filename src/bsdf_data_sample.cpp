#define POWITACQ_IMPLEMENTATION
#include <tekari/bsdf_data_sample.h>
#include <tekari/raw_data_processing.h>

TEKARI_NAMESPACE_BEGIN

#define N_PHI 64
#define N_THETA 32
#define N_WAVE_LENGTHS 256

inline powitacq::Vector3f enoki_to_powitacq_vec3(const Vector3f& v) { return powitacq::Vector3f(v[0], v[1], v[2]); }
inline Vector3f powitacq_to_enoki_vec3(const powitacq::Vector3f& v) { return Vector3f(v[0], v[1], v[2]); }

BSDFDataSample::BSDFDataSample(const string& file_path)
: m_brdf(file_path)
{
    m_raw_measurement.resize(N_PHI * N_THETA + N_PHI, N_WAVE_LENGTHS);
    m_v2d.resize(N_PHI * N_THETA + N_PHI);

    // add outter ring to complete mesh
    for (int j = 0; j < N_PHI; ++j)
    {
        float theta = 91.0f;
        float phi = 360.0f * j / N_PHI;
        RawMeasurement::SamplePoint sample_point = m_raw_measurement[N_PHI*N_THETA + j];
        sample_point.set_theta(theta);
        sample_point.set_phi(phi);
        memset(sample_point.data() + 2, 0, (N_WAVE_LENGTHS + 1) * sizeof(float));
        m_v2d[N_PHI*N_THETA + j] = hemisphere_to_disk({theta, phi});
    }
    compute_samples({0.0f, 0.0f});
    triangulate_data(m_f, m_v2d);
    compute_path_segments(m_path_segments, m_v2d);

    Index n_intensities = m_raw_measurement.n_wave_lengths() + 1;     // account for luminance
    Index n_sample_points = m_raw_measurement.n_sample_points();
    m_h.resize (n_intensities, n_sample_points);
    m_lh.resize(n_intensities, n_sample_points);
    m_n.resize (n_intensities, n_sample_points);
    m_ln.resize(n_intensities, n_sample_points);
    m_cache_mask.resize(n_intensities);
    m_points_stats.reset(n_intensities);
    m_selection_stats.reset(n_intensities);

    // artificially assign metadata members
    m_metadata.set_sample_name(file_path.substr(file_path.find_last_of("/") + 1, file_path.find_last_of(".")));
    m_metadata.set_points_in_file(m_raw_measurement.n_sample_points());
    m_selected_points.assign(m_raw_measurement.n_sample_points(), NOT_SELECTED_FLAG);

}

void BSDFDataSample::set_intensity_index(size_t intensity_index)
{
    m_intensity_index = std::min(intensity_index, m_raw_measurement.n_wave_lengths());;

    if (!m_cache_mask[m_intensity_index])
    {
        m_cache_mask[m_intensity_index] = true;

        compute_samples(m_metadata.incident_angle());
        compute_min_max_intensities(m_points_stats, m_raw_measurement, m_intensity_index);
        compute_normalized_heights(m_raw_measurement, m_points_stats, m_h, m_lh, m_intensity_index);

        update_points_stats(m_points_stats, m_raw_measurement, m_v2d, m_h, m_lh, m_intensity_index);
        m_selection_stats.reset(m_raw_measurement.n_wave_lengths() + 1);
        update_selection_stats( m_selection_stats, m_selected_points, m_raw_measurement, m_v2d, m_h, m_lh, m_intensity_index);

        compute_normals(m_f, m_v2d, m_h, m_lh, m_n, m_ln, m_intensity_index);
    }
    update_shaders_data();
    m_selection_axis.set_origin(selection_center());
}

void BSDFDataSample::set_incident_angle(const Vector2f& incident_angle)
{
    cout << std::setw(50) << std::left << "Setting incident angle ..\n";
    Timer<> timer;

    // clear mask
    std::fill(m_cache_mask.begin(), m_cache_mask.end(), false);

    m_metadata.set_incident_angle(incident_angle);
    compute_samples(m_metadata.incident_angle());
    triangulate_data(m_f, m_v2d);
    compute_path_segments(m_path_segments, m_v2d);
    m_points_stats.reset(m_raw_measurement.n_wave_lengths() + 1);
    set_intensity_index(m_intensity_index);
    link_data_to_shaders();

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void BSDFDataSample::compute_samples(const Vector2f& incident_angle)
{
    cout << std::setw(50) << std::left << "Sample brdf .. ";
    Timer<> timer;

    powitacq::Vector3f wi = enoki_to_powitacq_vec3(hemisphere_to_vec3(incident_angle));

    for (int theta = 0; theta < N_THETA; ++theta)
    {
        float v = float(theta + 0.5f) / N_THETA;
        for (int phi = 0; phi < N_PHI; ++phi)
        {
            float u = float(phi + 0.5f) / N_PHI;
            powitacq::Vector3f wo;
            float pdf;
            float sample = m_intensity_index == 0 ?
                                m_brdf.sample(powitacq::Vector2f(u, v), wi, m_intensity_index, &wo, &pdf):
                                m_brdf.sample(powitacq::Vector2f(u, v), wi, m_intensity_index-1, &wo, &pdf);

            Vector2f outgoing_angle = vec3_to_hemisphere(powitacq_to_enoki_vec3(wo));

            sample *= pdf;
            RawMeasurement::SamplePoint sample_point = m_raw_measurement[theta*N_PHI + phi];
            sample_point.set_theta(outgoing_angle[0]);
            sample_point.set_phi(outgoing_angle[1]);
            sample_point[m_intensity_index+2] = sample;
            m_v2d[theta*N_PHI + phi] = vec3_to_disk(powitacq_to_enoki_vec3(wo));
        }
    }

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

TEKARI_NAMESPACE_END