#pragma once

#include "common.h"
#include <nanogui/common.h>
#include <vector>
#include <memory>
#include <functional>

#include "points_stats.h"
#include "Metadata.h"
#include "ColorMap.h"
#include "Axis.h"

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
        PATH = 0,
        POINTS,
        INCIDENT_ANGLE,
        VIEW_COUNT
    };

    // constructors/destructors, assignement operators
    DataSample();
    DataSample(const DataSample&) = delete;
    DataSample(DataSample&&) = default;
    ~DataSample();
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
    void init_shaders();

    // info accessors
    inline const std::string name()             const { return m_metadata.sample_name(); }
    inline const Metadata& metadata()           const { return m_metadata; }
    inline bool has_selection()                 const { return m_selection_stats.points_count() > 0; }
    inline const PointsStats& points_stats()    const { return m_points_stats; }
    inline const PointsStats& selection_stats() const { return m_selection_stats; }
    inline bool is_spectral()                   const { return m_metadata.is_spectral(); }
    inline unsigned int max_wave_length_index() const { return m_h.size() - 1; }

    inline PointsStats& points_stats()      { return m_points_stats; }
    inline Metadata& metadata()             { return m_metadata; }
    inline PointsStats& selection_stats()   { return m_selection_stats; }
    inline float selection_min_intensity()        const { return m_selection_stats.min_intensity(m_wave_length_index); }
    inline float selection_max_intensity()        const { return m_selection_stats.max_intensity(m_wave_length_index); }
    inline float selection_average_intensity()    const { return m_selection_stats.average_intensity(m_wave_length_index); }

    inline float average_intensity()    const { return m_points_stats.average_intensity(m_wave_length_index); }

    // accessors
    inline unsigned int wave_length_index()    const { return m_wave_length_index; }
    void set_wave_length_index(size_t displayed_wave_length);

    inline const VectorXf& curr_h()             const { return m_display_as_log ? m_lh[m_wave_length_index] : m_h[m_wave_length_index]; }
    inline const Matrix3Xf& curr_n()            const { return m_display_as_log ? m_ln[m_wave_length_index] : m_n[m_wave_length_index]; }
    inline const Matrix2Xf& V2D()               const { return m_v2D; }
    inline const MatrixXXf& raw_points()        const { return m_raw_points; }
    inline const VectorXi8& selected_points()   const { return m_selected_points; }

    inline VectorXf& curr_h()           { return m_display_as_log ? m_lh[m_wave_length_index] : m_h[m_wave_length_index]; }
    inline Matrix3Xf& curr_n()          { return m_display_as_log ? m_ln[m_wave_length_index] : m_n[m_wave_length_index]; }
    inline std::vector<VectorXf>& H()   { return m_h; }
    inline std::vector<VectorXf>& LH()  { return m_lh; }
    inline std::vector<Matrix3Xf>& N()  { return m_n; }
    inline std::vector<Matrix3Xf>& LN() { return m_ln; }
    inline Matrix2Xf& V2D()             { return m_v2D; }
    inline MatrixXXf& raw_points()      { return m_raw_points; }
    inline VectorXu& path_segments()    { return m_path_segments; }
    inline VectorXi8& selected_points() { return m_selected_points; }

    inline Matrix3Xu& F()                { return m_f; }

    // Selection
    inline Vector3f selection_center() const { return has_selection() ? m_selection_stats.average_point(m_wave_length_index) : Vector3f{ 0,0,0 }; }
    void update_point_selection();

    // Dirty flag setter/accessor
    inline bool dirty() const            { return m_dirty; }
    inline void set_dirty(bool dirty)    { m_dirty = dirty; }

private:
    // Raw sample data
    Matrix3Xu               m_f;                // face indices
    Matrix2Xf               m_v2D;              // 2d coordinates (x,z)
    std::vector<VectorXf>   m_h;                // heights per point (one for each wavelenght)
    std::vector<VectorXf>   m_lh;               // logarithmic heights per point (one for each wavelenght)
    std::vector<Matrix3Xf>  m_n;                // normals per point (one for each wavelenght)
    std::vector<Matrix3Xf>  m_ln;               // logarithmic normals per point (one for each wavelenght)
    VectorXu                m_path_segments;
    unsigned int            m_wave_length_index;
    // Untransformed data
    MatrixXXf    m_raw_points;      // point0 : theta, phi, intensity0, intensity1, ...
                                    // point1 : theta, phi, intensity0, intensity1, ...
                                    // point2 : theta, phi, intensity0, intensity1, ...
                                    // ...
    PointsStats m_points_stats;

    // display Shaders
    nanogui::GLShader m_mesh_shader;
    nanogui::GLShader m_shaders[VIEW_COUNT];
    std::function<void(const Matrix4f&, std::shared_ptr<ColorMap>)> m_draw_functions[VIEW_COUNT];
    nanogui::GLShader m_predicted_outgoing_angle_shader;

    // display options
    bool m_display_as_log;
    bool m_display_views[VIEW_COUNT];

    // metadata
    Metadata m_metadata;
    Axis m_selection_axis;

    // Selected point
    VectorXi8       m_selected_points;
    PointsStats     m_selection_stats;

    // dirty flag to indicate changes in the data
    bool m_dirty;
};

TEKARI_NAMESPACE_END