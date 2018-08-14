#pragma once

#include <tekari/common.h>
#include <tekari/matrix_xx.h>

TEKARI_NAMESPACE_BEGIN

class RawMeasurement
{
public:
    using Row = MatrixXX<float>::Row;

    RawMeasurement()
    : m_data(0, 0)
    {}
    RawMeasurement(size_t n_wavelengths, size_t n_sample_points)
    : m_data(n_wavelengths + 3, n_sample_points)
    {}
    RawMeasurement(size_t n_wavelengths, size_t n_sample_points, float v)
    : m_data(n_wavelengths + 3, n_sample_points, v)
    {}

    inline void resize(size_t n_wavelengths, size_t n_sample_points) { m_data.resize(n_wavelengths + 3, n_sample_points); }
    inline void assign(size_t n_wavelengths, size_t n_sample_points, float v) { m_data.assign(n_wavelengths + 3, n_sample_points, v); }
    inline void clear() { m_data.clear(); }

    // access a particular sample point

    inline Row theta()        { return m_data[0]; }
    inline Row phi()          { return m_data[1]; }
    inline Row luminance()    { return m_data[2]; }
    inline const Row theta() const        { return m_data[0]; }
    inline const Row phi() const          { return m_data[1]; }
    inline const Row luminance() const    { return m_data[2]; }

    inline void set_theta(size_t index, float value)        { m_data[0][index] = value; }
    inline void set_phi(size_t index, float value)          { m_data[1][index] = value; }
    inline void set_luminance(size_t index, float value)    { m_data[2][index] = value; }

    inline Row intensity(size_t i)                { return m_data[i+3]; }
    inline const Row intensity(size_t i) const    { return m_data[i+3]; }

    inline Row operator[](size_t i) { return m_data[i]; }
    inline const Row operator[](size_t i) const { return m_data[i]; }

    inline float& operator()(size_t i, size_t j) { return m_data(i, j); }
    inline float operator()(size_t i, size_t j) const { return m_data(i, j); }

    inline float* data()               { return m_data.data(); }
    inline const float* data() const   { return m_data.data(); }

    inline size_t n_wavelengths() const     { return m_data.n_rows() - 3; }
    inline size_t n_sample_points() const   { return m_data.n_cols(); }
    inline size_t size() const              { return m_data.n_rows() * m_data.n_cols(); }

private:
    MatrixXX<float> m_data;     // layout:  theta_0     theta_1     ...
                                //          phi_0       phi_1       ...
                                //          luminance_0 luminance_1 ...
                                //          intensity00 intensity01 ...
                                //          intensity10 intensity11 ...
                                //          ...         ...         ...
};

TEKARI_NAMESPACE_END