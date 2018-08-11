#define POWITACQ_IMPLEMENTATION
#include <tekari/bsdf_data_sample.h>
#include <tekari/raw_data_processing.h>

TEKARI_NAMESPACE_BEGIN

inline powitacq::Vector3f enoki_to_powitacq_vec3(const Vector3f& v) { return powitacq::Vector3f(v[0], v[1], v[2]); }
inline Vector3f powitacq_to_enoki_vec3(const powitacq::Vector3f& v) { return Vector3f(v[0], v[1], v[2]); }

BSDFDataSample::BSDFDataSample(const string& file_path)
: m_brdf(file_path)
, m_n_theta(32)
, m_n_phi(32)
{
    // artificially assign metadata members
    m_metadata.add_line(m_brdf.description());
    m_metadata.set_sample_name(file_path.substr(file_path.find_last_of("/") + 1, file_path.find_last_of(".")));
}

void BSDFDataSample::init()
{
    DataSample::init();
    set_incident_angle({0.0f, 0.0f});
}

void BSDFDataSample::set_intensity_index(size_t intensity_index)
{
    m_intensity_index = std::min(intensity_index, m_raw_measurement.n_wavelengths() + 1);;
    if (!m_cache_mask[m_intensity_index])
    {
        m_cache_mask[m_intensity_index] = true;

        compute_samples();
        compute_min_max_intensities(m_points_stats, m_raw_measurement, m_intensity_index);
        compute_normalized_heights(m_raw_measurement, m_points_stats, m_h, m_lh, m_intensity_index);

        update_points_stats(m_points_stats, m_raw_measurement, m_v2d, m_h, m_lh, m_intensity_index);
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


    vector<float> luminance;
    vector<powitacq::Vector3f> wos;
    if (!m_brdf.set_state(enoki_to_powitacq_vec3(hemisphere_to_vec3(incident_angle)), m_n_theta, m_n_phi, luminance, wos))
    {
        cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
        return;
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

    // clear mask
    m_cache_mask.assign(n_intensities, false);
    m_cache_mask[0] = true;                     // luminance is always computed

    // artificially assign metadata members
    m_metadata.set_incident_angle(incident_angle);
    m_metadata.set_points_in_file(m_raw_measurement.n_sample_points());
    m_selected_points.assign(m_raw_measurement.n_sample_points(), NOT_SELECTED_FLAG);

    for (size_t i = 0; i < wos.size(); ++i)
    {
        RawMeasurement::SamplePoint sample_point = m_raw_measurement[i];
        Vector2f outgoing_angle = vec3_to_hemisphere(powitacq_to_enoki_vec3(wos[i]));
        sample_point.set_theta(outgoing_angle.x());
        sample_point.set_phi(outgoing_angle.y());
        sample_point.set_luminance(luminance[i]);
        m_v2d[i] = vec3_to_disk(powitacq_to_enoki_vec3(wos[i]));
    }

    triangulate_data(m_f, m_v2d);
    compute_path_segments(m_path_segments, m_v2d);

    // compute data for luminance
    compute_min_max_intensities(m_points_stats, m_raw_measurement, 0);
    compute_normalized_heights(m_raw_measurement, m_points_stats, m_h, m_lh, 0);
    update_points_stats(m_points_stats, m_raw_measurement, m_v2d, m_h, m_lh, 0);
    update_selection_stats(m_selection_stats, m_selected_points, m_raw_measurement, m_v2d, m_h, m_lh, 0);
    compute_normals(m_f, m_v2d, m_h, m_lh, m_n, m_ln, 0);

    set_intensity_index(m_intensity_index);
    link_data_to_shaders();

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

void BSDFDataSample::compute_samples()
{
    cout << std::setw(50) << std::left << "Sample brdf .. ";
    Timer<> timer;

    vector<float> frs;
    m_brdf.sample_state(m_intensity_index-1, frs);

    for (size_t i = 0; i < frs.size(); ++i)
    {
        m_raw_measurement[i][m_intensity_index+2] = frs[i];
    }

    cout << "done. (took " <<  time_string(timer.value()) << ")" << endl;
}

TEKARI_NAMESPACE_END