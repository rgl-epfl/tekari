#pragma once

#include <tekari/common.h>
#include <tekari/matrix_xx.h>

TEKARI_NAMESPACE_BEGIN

class RawMeasurement
{
public:
    
    class SamplePoint
    {
        friend class RawMeasurement;
    public:

        void operator=(const SamplePoint& other)
        {
            if(m_data.n_cols() != other.m_data.n_cols())
                throw new std::runtime_error("Invalide assignement operation between SamplePoints.");
            memcpy(m_data.data(), other.m_data.data(), m_data.n_cols());
        }

        float theta() const             { return m_data[0]; }
        float phi() const               { return m_data[1]; }
        float luminance() const         { return m_data[2]; }
        float intensity(size_t i) const { return m_data[i + 3]; }

        void set_theta(float value)                 { m_data[0] = value; }
        void set_phi(float value)                   { m_data[1] = value; }
        void set_luminance(float value)             { m_data[2] = value; }
        void set_intensity(size_t i, float value)   { m_data[i + 3] = value; }

        // access a particular value
        float operator[](size_t i) const    { return m_data[i]; }
        float &operator[](size_t i)         { return m_data[i]; }

        float *data()               { return m_data.data(); }
        const float *data() const   { return m_data.data(); }

        size_t n_wave_lengths() const      { return m_data.n_cols() - 3; }

    private:
        SamplePoint(MatrixXX<float>::Row data)
        : m_data(data)
        {}

        MatrixXX<float>::Row m_data;
    };

public:
    RawMeasurement()
    : m_data(0, 0)
    {}
    RawMeasurement(size_t n_sample_points, size_t n_wave_lengths)
    : m_data(n_sample_points, n_wave_lengths + 3)
    {}

    void resize(size_t n_sample_points, size_t n_wave_lengths) { m_data.resize(n_sample_points, n_wave_lengths + 3); }
    void clear() { m_data.clear(); }

    // access a particular sample point
    SamplePoint operator[](size_t i) { return SamplePoint(m_data[i]); }
    const SamplePoint operator[](size_t i) const { return SamplePoint(m_data[i]); }

    float* data()               { return m_data.data(); }
    const float* data() const   { return m_data.data(); }

    size_t n_wave_lengths() const      { return m_data.n_cols() - 3; }
    size_t n_sample_points() const    { return m_data.n_rows(); }

private:
    MatrixXX<float> m_data;
};

TEKARI_NAMESPACE_END