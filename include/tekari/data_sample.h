#pragma once

#include <tekari/common.h>
#include <tekari/selections.h>
#include <nanogui/common.h>
#include <tekari/points_stats.h>
#include <tekari/metadata.h>
#include <tekari/color_map.h>
#include <tekari/data_io.h>
#include <tekari/axis.h>

TEKARI_NAMESPACE_BEGIN

#define USE_SHADOWS                         (1 << 0)
#define USE_SPECULAR                        (1 << 1)
#define DISPLAY_AXIS                        (1 << 2)
#define DISPLAY_PREDICTED_OUTGOING_ANGLE    (1 << 3)
#define USE_WIREFRAME                       (1 << 4)
#define USE_INTEGRATED_COLORS                (1 << 5)

class DataSample
{
public:
    // Usefull types
    enum Views
    {
        MESH = 0,
        PATH,
        POINTS,
        INCIDENT_ANGLE,
        VIEW_COUNT
    };

    // constructors/destructors, assignement operators
    DataSample();
    DataSample(const DataSample&) = delete;
    DataSample(DataSample&&) = default;
    virtual ~DataSample();
    DataSample& operator=(const DataSample&) = delete;
    DataSample& operator=(DataSample&&) = default;

    void draw_gl(const Matrix4f& model,
                const Matrix4f& mvp,
                int flags,
                float point_size,
                std::shared_ptr<ColorMap> color_map);

    void toggle_log_view();
    bool display_as_log() const { return m_display_as_log; }
    inline void toggle_view(Views view, bool toggle) { m_display_views[view] = toggle; }
    inline bool display_view(Views view) const { return m_display_views[view]; }

    virtual void init();
    void link_data_to_shaders();
    void update_shaders_data();

    // info accessors
    inline const string name()              const { return m_metadata.sample_name(); }
    inline const Metadata& metadata()       const { return m_metadata; }

    inline bool has_selection()                 const { return m_selection_stats.points_count > 0; }

    inline size_t intensity_count()  const { return m_points_stats.intensity_count; }
    inline size_t points_count()     const { return m_points_stats.points_count; }
    inline float average_intensity() const { return m_points_stats[m_intensity_index].average_intensity; }

    void recompute_data();

    virtual void delete_selected_points() {}
    virtual void save(const string& file_path) {}

    // Selection
    inline Vector3f selection_center() const
    {
        if (!has_selection())
            return Vector3f{0,0,0};
        return m_selection_stats[m_intensity_index].average_points[m_display_as_log];
    }
    void update_point_selection();

    // Dirty flag setter/getter
    inline bool dirty() const            { return m_dirty; }
    inline void set_dirty(bool dirty)    { m_dirty = dirty; }

    inline size_t intensity_index() const { return m_intensity_index; }
    inline Vector2f incident_angle() const { return m_metadata.incident_angle(); }

    virtual void set_intensity_index(size_t displayed_wavelength) {}
    virtual void set_incident_angle(const Vector2f& i) {}

    inline const VectorXf& wavelengths() const { return m_wavelengths; }
    inline string wavelength_str()
    {
        if (m_intensity_index == 0)
            return string("luminance");

        return to_string(m_wavelengths[m_intensity_index-1]) + string(" nm");
    }
    virtual void get_selection_spectrum(vector<float> &spectrum) = 0;

    VectorXf& selected_points() { return m_selected_points; }
    PointsStats& points_stats() { return m_points_stats; }
    PointsStats& selection_stats() { return m_selection_stats; }
    PointsStats::Slice& curr_selection_stats() { return m_selection_stats[m_intensity_index]; }
    Matrix2Xf& v2d() { return m_v2d; }

    inline const MatrixXXf::Row     curr_h() const  { return m_h[m_display_as_log][m_intensity_index]; }
    inline const Matrix4XXf::Row    curr_n() const  { return m_n[m_display_as_log][m_intensity_index]; }
    inline MatrixXXf::Row           curr_h()        { return m_h[m_display_as_log][m_intensity_index]; }
    inline Matrix4XXf::Row          curr_n()        { return m_n[m_display_as_log][m_intensity_index]; }

protected:
    Matrix3Xi   m_f;                // face indices
    Matrix2Xf   m_v2d;              // 2d coordinates (x,z)
    MatrixXXf   m_colors;
    MatrixXXf   m_h[2];             // heights (standard and log) per point (one for luminance and one for each wavelength)
    Matrix4XXf  m_n[2];             // normals (standard and log) per point (one for luminance and one for each wavelength)
    VectorXu    m_path_segments;
    VectorXf    m_wavelengths;
    Mask        m_cache_mask;       // bit map indicating whether some intensity data is valid
    size_t      m_intensity_index;  // 0 correspond to luminance, otherwise to a given wavelength
    // Untransformed data
    RawMeasurement    m_raw_measurement;    
                                            // theta0,      theta1,     theta2, ...
                                            // phi0,        phi1,       phi2, ...
                                            // luminance0,  luminance1, luminance2, ...
                                            // intensity0,  intensity1, intensity2, ...
                                            // ...
    PointsStats m_points_stats;

    // display Shaders
    nanogui::GLShader m_shaders[VIEW_COUNT];

    // display options
    bool m_display_as_log;
    bool m_display_views[VIEW_COUNT];

    // metadata
    Metadata m_metadata;
    Axis m_selection_axis;

    // Selected point
    VectorXf        m_selected_points;          // one flag per vertex (has to be float for webgl shader)
    PointsStats     m_selection_stats;

    // dirty flag to indicate changes in the data
    bool m_dirty;
};

TEKARI_NAMESPACE_END