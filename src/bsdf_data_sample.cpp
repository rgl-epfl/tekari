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
    compute_samples({0.0f, 0.0f});
    triangulate_data(m_f, m_v2d);
    compute_path_segments(m_path_segments, m_v2d);

    // artificially assign metadata members
    m_metadata.add_line(m_brdf.description());
    m_metadata.set_sample_name(file_path.substr(file_path.find_last_of("/") + 1, file_path.find_last_of(".")));
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
    m_cache_mask.assign(m_raw_measurement.n_wave_lengths() + 1, false);

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

    vector<Vector3f> wos;
    vector<float> samples;

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

            Vector3f enoki_wo = powitacq_to_enoki_vec3(wo);
            if (enoki::norm(enoki_wo) == 0.0f)
                continue;

            sample *= pdf;

            wos.push_back(enoki_wo);
            samples.push_back(sample);
        }
    }
    // // add outter ring to complete mesh
    for (int j = 0; j < N_PHI; ++j)
    {
        float theta = 100.0f;
        float phi = 360.0f * j / N_PHI;
        wos.push_back(hemisphere_to_vec3({theta, phi}));
        samples.push_back(0.0f);
    }

    Index n_intensities = m_brdf.wavelengths().size() + 1;     // account for luminance
    Index n_sample_points = wos.size();

    m_raw_measurement.resize(n_sample_points, n_intensities);
    m_v2d.resize(n_sample_points);

    m_h.resize (n_intensities, n_sample_points);
    m_lh.resize(n_intensities, n_sample_points);
    m_n.resize (n_intensities, n_sample_points);
    m_ln.resize(n_intensities, n_sample_points);
    m_cache_mask.resize(n_intensities);
    m_points_stats.reset(n_intensities);
    m_selection_stats.reset(n_intensities);

    // artificially assign metadata members
    m_metadata.set_points_in_file(m_raw_measurement.n_sample_points());
    m_selected_points.assign(m_raw_measurement.n_sample_points(), NOT_SELECTED_FLAG);

    for (size_t i = 0; i < wos.size(); ++i)
    {
        RawMeasurement::SamplePoint sample_point = m_raw_measurement[i];
        Vector2f outgoing_angle = vec3_to_hemisphere(wos[i]);
        sample_point.set_theta(outgoing_angle.x());
        sample_point.set_phi(outgoing_angle.y());
        sample_point[m_intensity_index+2] = samples[i];
        m_v2d[i] = vec3_to_disk(wos[i]);
    }

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

TEKARI_NAMESPACE_END