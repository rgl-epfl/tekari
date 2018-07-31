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

#define USES_SHADOWS                     (1 << 0)
#define USES_SPECULAR                    (1 << 1)
#define DISPLAY_AXIS                     (1 << 2)
#define DISPLAY_PREDICTED_OUTGOING_ANGLE (1 << 3)

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

    void draw_gl(const Vector3f& view_origin,
                const Matrix4f& model,
                const Matrix4f& view,
                const Matrix4f& proj,
                int flags,
                std::shared_ptr<ColorMap> color_map);

    void toggle_log_view();
    inline void toggle_view(Views view, bool toggle) { m_display_views[view] = toggle; }
    inline bool display_view(Views view) const { return m_display_views[view]; }

    void link_data_to_shaders();
    void update_shaders_data();
    void init_shaders();

    // info accessors
    inline const string name()              const { return m_metadata.sample_name(); }
    inline bool is_spectral()               const { return m_metadata.is_spectral(); }
    inline const Metadata& metadata()       const { return m_metadata; }

    inline bool has_selection()             const { return m_selection_stats.points_count > 0; }
    inline size_t selection_points_count()  const { return m_selection_stats.points_count; }

    inline size_t intensity_count()            const { return m_points_stats.intensity_count; }
    inline size_t points_count()            const { return m_points_stats.points_count; }
    inline size_t n_wave_lengths()          const { return m_raw_measurement.n_wave_lengths(); }
    inline float average_intensity()        const { return m_points_stats.average_intensity[m_intensity_index]; }

    inline float selection_min_intensity()      const { return m_selection_stats.min_intensity[m_intensity_index]; }
    inline float selection_max_intensity()      const { return m_selection_stats.max_intensity[m_intensity_index]; }
    inline float selection_average_intensity()  const { return m_selection_stats.average_intensity[m_intensity_index]; }

    // Data selection/computation methods (wrapers for corresponding routines, easier to call)
    void select_points(const Matrix4f& mvp, const SelectionBox& selection_box, const Vector2i& canvas_size, SelectionMode mode);
    void select_closest_point(const Matrix4f& mvp, const Vector2i& mouse_pos, const Vector2i& canvas_size);
    void select_extreme_point(bool highest);
    void select_all_points();
    void deselect_all_points();
    void move_selection_along_path(bool up);
    void delete_selected_points();
    size_t count_selected_points() const;
    void recompute_data();

    void save(const string& file_path);

    // Selection
    inline Vector3f selection_center() const
    {
        return  has_selection() ?
                    m_display_as_log?
                        m_selection_stats.average_log_point[m_intensity_index] :
                        m_selection_stats.average_point[m_intensity_index] :
                    Vector3f{ 0,0,0 };
    }
    void update_point_selection();

    // Dirty flag setter/getter
    inline bool dirty() const            { return m_dirty; }
    inline void set_dirty(bool dirty)    { m_dirty = dirty; }

    inline size_t intensity_index() const { return m_intensity_index; }
    virtual void set_intensity_index(size_t displayed_wave_length) {}

    inline Vector2f incident_angle() const { return m_metadata.incident_angle(); }
    virtual void set_incident_angle(const Vector2f& i) {}

private:
    inline const MatrixXXf::Row     curr_h() const  { return m_display_as_log ? m_lh[m_intensity_index] : m_h[m_intensity_index]; }
    inline const Matrix4XXf::Row    curr_n() const  { return m_display_as_log ? m_ln[m_intensity_index] : m_n[m_intensity_index]; }
    inline MatrixXXf::Row           curr_h()        { return m_display_as_log ? m_lh[m_intensity_index] : m_h[m_intensity_index]; }
    inline Matrix4XXf::Row          curr_n()        { return m_display_as_log ? m_ln[m_intensity_index] : m_n[m_intensity_index]; }

protected:
    // Raw sample data
    Matrix3Xu   m_f;                // face indices
    Matrix2Xf   m_v2d;              // 2d coordinates (x,z)
    MatrixXXf   m_h;                // heights per point (one for each wavelenght)
    MatrixXXf   m_lh;               // logarithmic heights per point (one for each wavelenght)
    Matrix4XXf  m_n;                // normals per point (one for each wavelenght)
    Matrix4XXf  m_ln;               // logarithmic normals per point (one for each wavelenght)
    Mask        m_cache_mask;       // bit map indicating whether some intensity data is valid
    VectorXu    m_path_segments;
    size_t      m_intensity_index;
    // Untransformed data
    RawMeasurement    m_raw_measurement;      // point0 : theta, phi, intensity0, intensity1, ...
                                    // point1 : theta, phi, intensity0, intensity1, ...
                                    // point2 : theta, phi, intensity0, intensity1, ...
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