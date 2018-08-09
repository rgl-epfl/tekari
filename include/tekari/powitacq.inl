#include <cmath>
#include <cstdint>        // uint32_t, etc.
#include <cstring>        // memcpy
#include <stdexcept>      // std::runtime_error
#include <limits>         // std::numeric_limits
#include <sstream>        // std::ostringstream
#include <unordered_map>

#define POWITACQ_SAMPLE_LUMINANCE 1

POWITACQ_NAMESPACE_BEGIN

// *****************************************************************************
// Vector operations, arithmetic convenience functions
// *****************************************************************************

using Vector2u = Vector<uint32_t, 2>;
using Vector2i = Vector<int32_t, 2>;

static constexpr float Pi = 3.1415926535897932384626433832795f;
static constexpr float OneMinusEpsilon = 0.999999940395355225f;

#define POWITACQ_ARITHMETIC_OPERATOR(op)                                       \
    template <typename T, size_t Dim>                                          \
    Vector<T, Dim> operator op(const Vector<T, Dim> &v1,                       \
                               const Vector<T, Dim> &v2) {                     \
        Vector<T, Dim> result;                                                 \
        for (size_t i = 0; i < Dim; ++i)                                       \
            result[i] = v1[i] op v2[i];                                        \
        return result;                                                         \
    }                                                                          \
    template <typename T, size_t Dim>                                          \
    Vector<T, Dim> operator op(const Vector<T, Dim> &v1, T s) {                \
        Vector<T, Dim> result;                                                 \
        for (size_t i = 0; i < Dim; ++i)                                       \
            result[i] = v1[i] op s;                                            \
        return result;                                                         \
    }

#define POWITACQ_ARITHMETIC_OPERATOR_COMPOUND(op)                              \
    template <typename T, size_t Dim>                                          \
    Vector<T, Dim> &operator op(Vector<T, Dim> &v1,                            \
                                const Vector<T, Dim> &v2) {                    \
        for (size_t i = 0; i < Dim; ++i)                                       \
            v1[i] op v2[i];                                                    \
        return v1;                                                             \
    }

POWITACQ_ARITHMETIC_OPERATOR(+)
POWITACQ_ARITHMETIC_OPERATOR_COMPOUND(+=)
POWITACQ_ARITHMETIC_OPERATOR(-)
POWITACQ_ARITHMETIC_OPERATOR_COMPOUND(-=)
POWITACQ_ARITHMETIC_OPERATOR(*)
POWITACQ_ARITHMETIC_OPERATOR_COMPOUND(*=)
POWITACQ_ARITHMETIC_OPERATOR(/)
POWITACQ_ARITHMETIC_OPERATOR_COMPOUND(/=)

#undef POWITACQ_ARITHMETIC_OPERATOR
#undef POWITACQ_ARITHMETIC_OPERATOR_COMPOUND

float fma(float a, float b, float c) {
    return a * b + c;
}

template <typename T> T clamp(T value, T min_value, T max_value) {
    return std::min(std::max(value, min_value), max_value);
}

template <typename T> T sqr(T value) { return value * value; }

template <typename T, size_t Dim>
Vector<T, Dim> clamp(Vector<T, Dim> value, T min_value, T max_value) {
    Vector<T, Dim> result;
    for (size_t i = 0; i < Dim; ++i)
        result[i] = clamp(value[i], min_value, max_value);
    return result;
}

template <typename T, size_t Dim>
Vector<T, Dim> min(Vector<T, Dim> a, Vector<T, Dim> b) {
    Vector<T, Dim> result;
    for (size_t i = 0; i < Dim; ++i)
        result[i] = std::min(a[i], b[i]);
    return result;
}

template <typename T, size_t Dim>
Vector<T, Dim> max(Vector<T, Dim> a, Vector<T, Dim> b) {
    Vector<T, Dim> result;
    for (size_t i = 0; i < Dim; ++i)
        result[i] = std::max(a[i], b[i]);
    return result;
}

template <typename T, size_t Dim> T hprod(const Vector<T, Dim> &v) {
    T result = v[0];
    for (size_t i = 1; i < Dim; ++i)
        result *= v[i];
    return result;
}

template <typename T, size_t Dim> T dot(const Vector<T, Dim> &v1, const Vector<T, Dim> &v2) {
    T result = 0;
    for (size_t i = 0; i < Dim; ++i)
        result += v1[i] * v2[i];
    return result;
}

template <typename T, size_t Dim> Vector<T, Dim> normalize(const Vector<T, Dim> &v) {
    return v / std::sqrt(dot(v, v));
}

// *****************************************************************************
// Bisection search for intervals
// *****************************************************************************

/**
 * \brief Find an interval in an ordered set
 *
 * This function is very similar to \c std::upper_bound, but it uses a functor
 * rather than an actual array to permit working with procedurally defined
 * data. It returns the index \c i such that pred(i) is \c true and pred(i+1)
 * is \c false. See below for special cases.
 *
 * This function is primarily used to locate an interval (i, i+1) for linear
 * interpolation, hence its name. To avoid issues out of bounds accesses, and
 * to deal with predicates that evaluate to \c true or \c false on the entire
 * domain, the returned left interval index is clamped to the range <tt>[left,
 * right-2]</tt>.
 * In particular:
 * If there is no index such that pred(i) is true, we return (left).
 * If there is no index such that pred(i+1) is false, we return (right-2).
 */
template <typename Predicate>
size_t find_interval(size_t size_, const Predicate &pred) {
    ssize_t size  = (ssize_t) size_ - 2,
            first = 1;

    while (size > 0) {
        size_t half   = (size_t) size >> 1,
               middle = first + half;

        // Evaluate the predicate */
        bool pred_result = pred(middle);

        // .. and recurse into the left or right side
        first = pred_result ? middle + 1 : first;
        size = pred_result ? size - (half + 1) : half;
    }

    return (size_t) clamp((ssize_t) first - 1, (ssize_t) 0,
                          (ssize_t) size_ - 2);
}

// *****************************************************************************
// Marginal-conditional warp
// *****************************************************************************

/**
 * \brief Implements a marginal sample warping scheme for 2D distributions
 * with linear interpolation and an optional dependence on additional parameters
 *
 * This class takes a rectangular floating point array as input and constructs
 * internal data structures to efficiently map uniform variates from the unit
 * square <tt>[0, 1]^2</tt> to a function on <tt>[0, 1]^2</tt> that linearly
 * interpolates the input array.
 *
 * The mapping is constructed via the inversion method, which is applied to
 * a marginal distribution over rows, followed by a conditional distribution
 * over columns.
 *
 * The implementation also supports <em>conditional distributions</em>, i.e. 2D
 * distributions that depend on an arbitrary number of parameters (indicated
 * via the \c Dimension template parameter).
 *
 * In this case, the input array should have dimensions <tt>N0 x N1 x ... x Nn
 * x res[1] x res[0]</tt> (where the last dimension is contiguous in memory),
 * and the <tt>param_res</tt> should be set to <tt>{ N0, N1, ..., Nn }</tt>,
 * and <tt>param_values</tt> should contain the parameter values where the
 * distribution is discretized. Linear interpolation is used when sampling or
 * evaluating the distribution for in-between parameter values.
 */
template <size_t Dimension = 0> class Marginal2D {
private:
    using FloatStorage = std::vector<float>;

#if !defined(_MSC_VER)
    static constexpr size_t ArraySize = Dimension;
#else
    static constexpr size_t ArraySize = (Dimension != 0) ? Dimension : 1;
#endif

public:
    Marginal2D() = default;

    /**
     * Construct a marginal sample warping scheme for floating point
     * data of resolution \c size.
     *
     * \c param_res and \c param_values are only needed for conditional
     * distributions (see the text describing the Marginal2D class).
     *
     * If \c normalize is set to \c false, the implementation will not
     * re-scale the distribution so that it integrates to \c 1. It can
     * still be sampled (proportionally), but returned density values
     * will reflect the unnormalized values.
     *
     * If \c build_cdf is set to \c false, the implementation will not
     * construct the cdf needed for sample warping, which saves memory in case
     * this functionality is not needed (e.g. if only the interpolation in \c
     * eval() is used).
     */
    Marginal2D(const Vector2u &size, const float *data,
               std::array<uint32_t, Dimension> param_res = { },
               std::array<const float *, Dimension> param_values = { },
               bool normalize = true, bool build_cdf = true)
        : m_size(size), m_patch_size(Vector2f(1.f) / Vector2f(m_size - 1u)),
          m_inv_patch_size(m_size - 1u) {

        if (build_cdf && !normalize)
            throw std::runtime_error("Marginal2D: build_cdf implies normalize=true");

        /* Keep track of the dependence on additional parameters (optional) */
        uint32_t slices = 1;
        for (int i = (int) Dimension - 1; i >= 0; --i) {
            if (param_res[i] < 1)
                throw std::runtime_error("Marginal2D(): parameter resolution must be >= 1!");

            m_param_size[i] = param_res[i];
            m_param_values[i] = FloatStorage(param_res[i]);
            memcpy(m_param_values[i].data(), param_values[i],
                   sizeof(float) * param_res[i]);
            m_param_strides[i] = param_res[i] > 1 ? slices : 0;
            slices *= m_param_size[i];
        }

        uint32_t n_values = hprod(size);

        m_data = FloatStorage(slices * n_values);

        if (build_cdf) {
            m_marginal_cdf = FloatStorage(slices * m_size.y());
            m_conditional_cdf = FloatStorage(slices * n_values);

            float *marginal_cdf = m_marginal_cdf.data(),
                  *conditional_cdf = m_conditional_cdf.data(),
                  *data_out = m_data.data();

            for (uint32_t slice = 0; slice < slices; ++slice) {
                /* Construct conditional CDF */
                for (uint32_t y = 0; y < m_size.y(); ++y) {
                    double sum = 0.0;
                    size_t i = y * size.x();
                    conditional_cdf[i] = 0.f;
                    for (uint32_t x = 0; x < m_size.x() - 1; ++x, ++i) {
                        sum += .5 * ((double) data[i] + (double) data[i + 1]);
                        conditional_cdf[i + 1] = (float) sum;
                    }
                }

                /* Construct marginal CDF */
                marginal_cdf[0] = 0.f;
                double sum = 0.0;
                for (uint32_t y = 0; y < m_size.y() - 1; ++y) {
                    sum += .5 * ((double) conditional_cdf[(y + 1) * size.x() - 1] +
                                 (double) conditional_cdf[(y + 2) * size.x() - 1]);
                    marginal_cdf[y + 1] = (float) sum;
                }

                /* Normalize CDFs and PDF (if requested) */
                float normalization = 1.f / marginal_cdf[m_size.y() - 1];
                for (size_t i = 0; i < n_values; ++i)
                    conditional_cdf[i] *= normalization;
                for (size_t i = 0; i < m_size.y(); ++i)
                    marginal_cdf[i] *= normalization;
                for (size_t i = 0; i < n_values; ++i)
                    data_out[i] = data[i] * normalization;

                marginal_cdf += m_size.y();
                conditional_cdf += n_values;
                data_out += n_values;
                data += n_values;
            }
        } else {
            float *data_out = m_data.data();

            for (uint32_t slice = 0; slice < slices; ++slice) {
                float normalization = 1.f / hprod(m_inv_patch_size);
                if (normalize) {
                    double sum = 0.0;
                    for (uint32_t y = 0; y < m_size.y() - 1; ++y) {
                        size_t i = y * size.x();
                        for (uint32_t x = 0; x < m_size.x() - 1; ++x, ++i) {
                            float v00 = data[i],
                                  v10 = data[i + 1],
                                  v01 = data[i + size.x()],
                                  v11 = data[i + 1 + size.x()],
                                  avg = .25f * (v00 + v10 + v01 + v11);
                            sum += (double) avg;
                        }
                    }
                    normalization = float(1.0 / sum);
                }

                for (uint32_t k = 0; k < n_values; ++k)
                    data_out[k] = data[k] * normalization;

                data += n_values;
                data_out += n_values;
            }
        }
    }


    /**
     * \brief Given a uniformly distributed 2D sample, draw a sample from the
     * distribution (parameterized by \c param if applicable)
     *
     * Returns the warped sample and associated probability density.
     */
    std::pair<Vector2f, float> sample(Vector2f sample,
                                      const float *param = nullptr) const {
        /* Avoid degeneracies at the extrema */
        sample = clamp(sample, 1.f - OneMinusEpsilon, OneMinusEpsilon);

        /* Look up parameter-related indices and weights (if Dimension != 0) */
        float param_weight[2 * ArraySize];
        uint32_t slice_offset = 0u;
        for (size_t dim = 0; dim < Dimension; ++dim) {
            if (m_param_size[dim] == 1) {
                param_weight[2 * dim] = 1.f;
                param_weight[2 * dim + 1] = 0.f;
                continue;
            }

            uint32_t param_index = find_interval(
                m_param_size[dim],
                [&](uint32_t idx) {
                    return m_param_values[dim].data()[idx] <= param[dim];
                }
            );

            float p0 = m_param_values[dim][param_index],
                  p1 = m_param_values[dim][param_index + 1];

            param_weight[2 * dim + 1] =
                clamp((param[dim] - p0) / (p1 - p0), 0.f, 1.f);
            param_weight[2 * dim] = 1.f - param_weight[2 * dim + 1];
            slice_offset += m_param_strides[dim] * param_index;
        }

        /* Sample the row first */
        uint32_t offset = 0;
        if (Dimension != 0)
            offset = slice_offset * m_size.y();

        auto fetch_marginal = [&](uint32_t idx)  -> float {
            return lookup<Dimension>(m_marginal_cdf.data(), offset + idx,
                                     m_size.y(), param_weight);
        };

        uint32_t row = find_interval(
            m_size.y(),
            [&](uint32_t idx) {
                return fetch_marginal(idx) < sample.y();
            }
        );

        sample.y() -= fetch_marginal(row);

        uint32_t slice_size = hprod(m_size);
        offset = row * m_size.x();
        if (Dimension != 0)
            offset += slice_offset * slice_size;

        float r0 = lookup<Dimension>(m_conditional_cdf.data(),
                                     offset + m_size.x() - 1, slice_size,
                                     param_weight),
              r1 = lookup<Dimension>(m_conditional_cdf.data(),
                                     offset + (m_size.x() * 2 - 1), slice_size,
                                     param_weight);

        bool is_const = std::abs(r0 - r1) < 1e-4f * (r0 + r1);
        sample.y() = is_const ? (2.f * sample.y()) :
            (r0 - std::sqrt(r0 * r0 - 2.f * sample.y() * (r0 - r1)));
        sample.y() /= is_const ? (r0 + r1) : (r0 - r1);

        /* Sample the column next */
        sample.x() *= (1.f - sample.y()) * r0 + sample.y() * r1;

        auto fetch_conditional = [&](uint32_t idx) -> float {
            float v0 = lookup<Dimension>(m_conditional_cdf.data(), offset + idx,
                                         slice_size, param_weight),
                  v1 = lookup<Dimension>(m_conditional_cdf.data() + m_size.x(),
                                         offset + idx, slice_size, param_weight);

            return (1.f - sample.y()) * v0 + sample.y() * v1;
        };

        uint32_t col = find_interval(
            m_size.x(),
            [&](uint32_t idx) {
                return fetch_conditional(idx) < sample.x();
            }
        );

        sample.x() -= fetch_conditional(col);

        offset += col;

        float v00 = lookup<Dimension>(m_data.data(), offset, slice_size,
                                      param_weight),
              v10 = lookup<Dimension>(m_data.data() + 1, offset, slice_size,
                                      param_weight),
              v01 = lookup<Dimension>(m_data.data() + m_size.x(), offset,
                                      slice_size, param_weight),
              v11 = lookup<Dimension>(m_data.data() + m_size.x() + 1, offset,
                                      slice_size, param_weight),
              c0  = fma((1.f - sample.y()), v00, sample.y() * v01),
              c1  = fma((1.f - sample.y()), v10, sample.y() * v11);

        is_const = std::abs(c0 - c1) < 1e-4f * (c0 + c1);
        sample.x() = is_const ? (2.f * sample.x()) :
            (c0 - std::sqrt(c0 * c0 - 2.f * sample.x() * (c0 - c1)));
        sample.x() /= is_const ? (c0 + c1) : (c0 - c1);

        return {
            (Vector2f(col, row) + sample) * m_patch_size,
            ((1.f - sample.x()) * c0 + sample.x() * c1) * hprod(m_inv_patch_size)
        };
    }

    /// Inverse of the mapping implemented in \c sample()
    std::pair<Vector2f, float> invert(Vector2f sample,
                                      const float *param = nullptr) const {
        /* Look up parameter-related indices and weights (if Dimension != 0) */
        float param_weight[2 * ArraySize];
        uint32_t slice_offset = 0u;
        for (size_t dim = 0; dim < Dimension; ++dim) {
            if (m_param_size[dim] == 1) {
                param_weight[2 * dim] = 1.f;
                param_weight[2 * dim + 1] = 0.f;
                continue;
            }

            uint32_t param_index = find_interval(
                m_param_size[dim],
                [&](uint32_t idx) {
                    return m_param_values[dim][idx] <= param[dim];
                }
            );

            float p0 = m_param_values[dim][param_index],
                  p1 = m_param_values[dim][param_index + 1];

            param_weight[2 * dim + 1] =
                clamp((param[dim] - p0) / (p1 - p0), 0.f, 1.f);
            param_weight[2 * dim] = 1.f - param_weight[2 * dim + 1];
            slice_offset += m_param_strides[dim] * param_index;
        }

        /* Fetch values at corners of bilinear patch */
        sample *= m_inv_patch_size;
        Vector2u pos = min(Vector2u(sample), m_size - 2u);
        sample -= Vector2f(Vector2i(pos));

        uint32_t offset = pos.x() + pos.y() * m_size.x();
        uint32_t slice_size = hprod(m_size);
        if (Dimension != 0)
            offset += slice_offset * slice_size;

        /* Invert the X component */
        float v00 = lookup<Dimension>(m_data.data(), offset, slice_size,
                                      param_weight),
              v10 = lookup<Dimension>(m_data.data() + 1, offset, slice_size,
                                      param_weight),
              v01 = lookup<Dimension>(m_data.data() + m_size.x(), offset, slice_size,
                                      param_weight),
              v11 = lookup<Dimension>(m_data.data() + m_size.x() + 1, offset, slice_size,
                                      param_weight);

        Vector2f w1 = sample, w0 = Vector2f(1.f) - w1;

        float c0  = fma(w0.y(), v00, w1.y() * v01),
              c1  = fma(w0.y(), v10, w1.y() * v11),
              pdf = fma(w0.x(), c0, w1.x() * c1);

        sample.x() *= c0 + .5f * sample.x() * (c1 - c0);

        float v0 = lookup<Dimension>(m_conditional_cdf.data(), offset,
                                     slice_size, param_weight),
              v1 = lookup<Dimension>(m_conditional_cdf.data() + m_size.x(),
                                     offset, slice_size, param_weight);

        sample.x() += (1.f - sample.y()) * v0 + sample.y() * v1;

        offset = pos.y() * m_size.x();
        if (Dimension != 0)
            offset += slice_offset * slice_size;

        float r0 = lookup<Dimension>(m_conditional_cdf.data(),
                                     offset + m_size.x() - 1, slice_size,
                                     param_weight),
              r1 = lookup<Dimension>(m_conditional_cdf.data(),
                                     offset + (m_size.x() * 2 - 1), slice_size,
                                     param_weight);

        sample.x() /= (1.f - sample.y()) * r0 + sample.y() * r1;

        /* Invert the Y component */
        sample.y() *= r0 + .5f * sample.y() * (r1 - r0);

        offset = pos.y();
        if (Dimension != 0)
            offset += slice_offset * m_size.y();

        sample.y() += lookup<Dimension>(m_marginal_cdf.data(), offset,
                                       m_size.y(), param_weight);

        return { sample, pdf * hprod(m_inv_patch_size) };
    }

    /**
     * \brief Evaluate the density at position \c pos. The distribution is
     * parameterized by \c param if applicable.
     */
    float eval(Vector2f pos, const float *param = nullptr) const {
        /* Look up parameter-related indices and weights (if Dimension != 0) */
        float param_weight[2 * ArraySize];
        uint32_t slice_offset = 0u;

        for (size_t dim = 0; dim < Dimension; ++dim) {
            if (m_param_size[dim] == 1) {
                param_weight[2 * dim] = 1.f;
                param_weight[2 * dim + 1] = 0.f;
                continue;
            }

            uint32_t param_index = find_interval(
                m_param_size[dim],
                [&](uint32_t idx) {
                    return m_param_values[dim][idx] <= param[dim];
                });

            float p0 = m_param_values[dim][param_index],
                  p1 = m_param_values[dim][param_index + 1];

            param_weight[2 * dim + 1] =
                clamp((param[dim] - p0) / (p1 - p0), 0.f, 1.f);
            param_weight[2 * dim] = 1.f - param_weight[2 * dim + 1];
            slice_offset += m_param_strides[dim] * param_index;
        }

        /* Compute linear interpolation weights */
        pos *= m_inv_patch_size;
        Vector2u offset = min(Vector2u(pos), m_size - 2u);

        Vector2f w1 = pos - Vector2f(Vector2i(offset)),
                 w0 = Vector2f(1.f) - w1;

        uint32_t index = offset.x() + offset.y() * m_size.x();

        uint32_t size = hprod(m_size);
        if (Dimension != 0)
            index += slice_offset * size;

        float v00 = lookup<Dimension>(m_data.data(), index, size,
                                      param_weight),
              v10 = lookup<Dimension>(m_data.data() + 1, index, size,
                                      param_weight),
              v01 = lookup<Dimension>(m_data.data() + m_size.x(), index, size,
                                      param_weight),
              v11 = lookup<Dimension>(m_data.data() + m_size.x() + 1, index, size,
                                      param_weight);

        return fma(w0.y(), fma(w0.x(), v00, w1.x() * v10),
                        w1.y() * fma(w0.x(), v01, w1.x() * v11)) *
               hprod(m_inv_patch_size);
    }

private:
        template <size_t Dim, std::enable_if_t<Dim != 0, int> = 0>
         float lookup(const float *data, uint32_t i0,
                      uint32_t size, const float *param_weight) const {
            uint32_t i1 = i0 + m_param_strides[Dim - 1] * size;

            float w0 = param_weight[2 * Dim - 2],
                  w1 = param_weight[2 * Dim - 1],
                  v0 = lookup<Dim - 1>(data, i0, size, param_weight),
                  v1 = lookup<Dim - 1>(data, i1, size, param_weight);

            return fma(v0, w0, v1 * w1);
        }

        template <size_t Dim, std::enable_if_t<Dim == 0, int> = 0>
        float lookup(const float *data, uint32_t index, uint32_t,
                     const float *) const {
            return data[index];
        }

    private:
        /// Resolution of the discretized density function
        Vector2u m_size;

        /// Size of a bilinear patch in the unit square
        Vector2f m_patch_size, m_inv_patch_size;

        /// Resolution of each parameter (optional)
        uint32_t m_param_size[ArraySize];

        /// Stride per parameter in units of sizeof(float)
        uint32_t m_param_strides[ArraySize];

        /// Discretization of each parameter domain
        FloatStorage m_param_values[ArraySize];

        /// Density values
        FloatStorage m_data;

        /// Marginal and conditional PDFs
        FloatStorage m_marginal_cdf;
        FloatStorage m_conditional_cdf;
};

using Warp2D0 = Marginal2D<0>;
using Warp2D2 = Marginal2D<2>;
using Warp2D3 = Marginal2D<3>;

// *****************************************************************************
// Tensor file I/O
// *****************************************************************************

class Tensor {
public:
    // Data type of the tensor's fields
    enum Type {
        /* Invalid/unspecified */
        Invalid = 0,

        /* Signed and size_teger values */
        UInt8,  Int8,
        UInt16, Int16,
        UInt32, Int32,
        UInt64, Int64,

        /* Floating point values */
        Float16, Float32, Float64,
    };

    struct Field {
        // Data type of the tensor's fields
        Type dtype;

        // Offset in the file
        size_t offset;

        /// Specifies both rank and size along each dimension
        std::vector<size_t> shape;

        /// Pointer to the start of the tensor
        std::unique_ptr<uint8_t[]> data;
    };

    /// Load a tensor file into memory
    Tensor(const std::string &filename);

    /// Does the file contain a field of the specified name?
    bool has_field(const std::string &name) const;

    /// Return a data structure with information about the specified field
    const Field &field(const std::string &name) const;

    /// Return a human-readable summary
    std::string to_string() const;

    /// Return the total size of the tensor's data
    size_t size() const { return m_size; }

    /// Return the name of the file from which the tensor was loaded (for compaptibility with Mitsuba's TensorFile class)
    std::string filename() const { return m_filename; }

private:
    std::unordered_map<std::string, Field> m_fields;
    std::string m_filename;
    size_t m_size;
};

static std::ostream &operator<<(std::ostream &os, Tensor::Type value) {
    switch(value) {
        case Tensor::Invalid:  os << "invalid"; break;
        case Tensor::UInt8 :   os << "uint8_t"; break;
        case Tensor::Int8:     os << "int8_t"; break;
        case Tensor::UInt16:   os << "uint16_t"; break;
        case Tensor::Int16:    os << "int16_t"; break;
        case Tensor::UInt32:   os << "uint32_t"; break;
        case Tensor::Int32:    os << "int8_t"; break;
        case Tensor::UInt64:   os << "uint64_t"; break;
        case Tensor::Int64:    os << "int64_t"; break;
        case Tensor::Float16:  os << "float16_t"; break;
        case Tensor::Float32:  os << "float32_t"; break;
        case Tensor::Float64:  os << "float64_t"; break;
        default:               os << "unkown"; break;
    }
    return os;
}

static size_t type_size(Tensor::Type value) {
    switch(value) {
        case Tensor::Invalid:  return 0; break;
        case Tensor::UInt8 :   return 1; break;
        case Tensor::Int8:     return 1; break;
        case Tensor::UInt16:   return 2; break;
        case Tensor::Int16:    return 2; break;
        case Tensor::UInt32:   return 4; break;
        case Tensor::Int32:    return 4; break;
        case Tensor::UInt64:   return 8; break;
        case Tensor::Int64:    return 8; break;
        case Tensor::Float16:  return 2; break;
        case Tensor::Float32:  return 4; break;
        case Tensor::Float64:  return 8; break;
        default:               return 0; break;
    }
}

Tensor::Tensor(const std::string &filename) : m_filename(filename) {
    // Helpful macros to limit error-handling code duplication
    #define ASSERT(cond, msg)                              \
        do {                                               \
            if (!(cond)) {                                 \
                fclose(file);                              \
                throw std::runtime_error("Tensor: " msg);  \
            }                                              \
        } while(0)

    #define SAFE_READ(vars, size, count) \
        ASSERT(fread(vars, size, count, file) == (count), "Unable to read " #vars ".")

    FILE *file = fopen(filename.c_str(), "rb");
    if (file == NULL)
        throw std::runtime_error("Unable to open file " + filename);

    ASSERT(!fseek(file, 0, SEEK_END),"Unable to seek to end of file.");

    long size = ftell(file);
    ASSERT(size != -1, "Unable to tell file cursor position.");
    m_size = static_cast<size_t>(size);
    rewind(file);

    ASSERT(m_size >= 12 + 2 + 4, "Invalid tensor file: too small, truncated?");

    uint8_t header[12], version[2];
    uint32_t n_fields;
    SAFE_READ(header, sizeof(*header), 12);
    SAFE_READ(version, sizeof(*version), 2);
    SAFE_READ(&n_fields, sizeof(n_fields), 1);

    ASSERT(memcmp(header, "tensor_file", 12) == 0, "Invalid tensor file: invalid header.");
    ASSERT(version[0] == 1 && version[1] == 0, "Invalid tensor file: unknown file version.");

    for (uint32_t i = 0; i < n_fields; ++i) {
        uint8_t dtype;
        uint16_t name_length, ndim;
        uint64_t offset;

        SAFE_READ(&name_length, sizeof(name_length), 1);
        std::string name(name_length, '\0');
        SAFE_READ((char*)name.data(), 1, name_length);
        SAFE_READ(&ndim, sizeof(ndim), 1);
        SAFE_READ(&dtype, sizeof(dtype), 1);
        SAFE_READ(&offset, sizeof(offset), 1);
        ASSERT(dtype != Invalid && dtype <= Float64, "Invalid tensor file: unknown type.");

        std::vector<size_t> shape(ndim);
        size_t total_size = type_size((Type)dtype);       // no need to check here, line 43 already removes invalid types
        for (size_t j = 0; j < (size_t) ndim; ++j) {
            uint64_t size_value;
            SAFE_READ(&size_value, sizeof(size_value), 1);
            shape[j] = (size_t) size_value;
            total_size *= shape[j];
        }

        auto data = std::unique_ptr<uint8_t[]>(new uint8_t[total_size]);

        long cur_pos = ftell(file);
        ASSERT(cur_pos != -1, "Unable to tell current cursor position.");
        ASSERT(fseek(file, offset, SEEK_SET) != -1, "Unable to seek to tensor offset.");
        SAFE_READ(data.get(), 1, total_size);
        ASSERT(fseek(file, cur_pos, SEEK_SET) != -1, "Unable to seek back to current position");

        m_fields[name] =
            Field{ (Type) dtype, static_cast<size_t>(offset), shape, std::move(data) };
    }

    fclose(file);

    #undef SAFE_READ
    #undef ASSERT
}

/// Does the file contain a field of the specified name?
bool Tensor::has_field(const std::string &name) const {
    return m_fields.find(name) != m_fields.end();
}

/// Return a data structure with information about the specified field
const Tensor::Field &Tensor::field(const std::string &name) const {
    auto it = m_fields.find(name);
    if (it == m_fields.end())
        throw std::runtime_error("Tensor: Unable to find field " + name);
    return it->second;
}

/// Return a human-readable summary
std::string Tensor::to_string() const {
    std::ostringstream oss;
    oss << "Tensor[" << std::endl
        << "  filename = \"" << m_filename << "\"," << std::endl
        << "  size = " << size() << "," << std::endl
        << "  fields = {" << std::endl;

    size_t ctr = 0;
    for (const auto &it : m_fields) {
        oss << "    \"" << it.first << "\"" << " => [" << std::endl
            << "      dtype = " << it.second.dtype << "," << std::endl
            << "      offset = " << it.second.offset << "," << std::endl
            << "      shape = [";
        const auto& shape = it.second.shape;
        for (size_t j = 0; j < shape.size(); ++j) {
            oss << shape[j];
            if (j + 1 < shape.size())
                oss << ", ";
        }

        oss << "]" << std::endl;

        oss << "    ]";
        if (++ctr < m_fields.size())
            oss << ",";
        oss << std::endl;

    }

    oss << "  }" << std::endl
        << "]";

    return oss.str();
}

// *****************************************************************************
// BRDF implementation
// *****************************************************************************

struct BRDF::Data {
    Warp2D0 ndf;
    Warp2D0 sigma;
    Warp2D2 vndf;
    Warp2D2 luminance;
    Warp2D3 spectra;
    Spectrum wavelengths;
    bool isotropic;
    bool jacobian;
};

// *****************************************************************************
// BRDF convenience functions
// *****************************************************************************

template <typename Value> Value u2theta(Value u) {
    return sqr(u) * (Pi / 2.f);
}

template <typename Value> Value u2phi(Value u) {
    return (2.f * u - 1.f) * Pi;
}

template <typename Value> Value theta2u(Value theta) {
    return std::sqrt(theta * (2.f / Pi));
}

template <typename Value> Value phi2u(Value phi) {
    return (phi + Pi) / (2.f * Pi);
}

Spectrum BRDF::zero() const {
    return Spectrum(0.f, m_data->wavelengths.size());
}

const Spectrum &BRDF::wavelengths() const {
    return m_data->wavelengths;
}

// *****************************************************************************
// Ctor/dtor
// *****************************************************************************

BRDF::BRDF(const std::string &path_to_file) {

    Tensor tf = Tensor(path_to_file);
    auto& theta_i = tf.field("theta_i");
    auto& phi_i = tf.field("phi_i");
    auto& ndf = tf.field("ndf");
    auto& sigma = tf.field("sigma");
    auto& vndf = tf.field("vndf");
    auto& spectra = tf.field("spectra");
    auto& luminance = tf.field("luminance");
    auto& wavelengths = tf.field("wavelengths");
    auto& description = tf.field("description");
    auto& jacobian = tf.field("jacobian");

    if (!(description.shape.size() == 1 &&
          description.dtype == Tensor::UInt8 &&

          theta_i.shape.size() == 1 &&
          theta_i.dtype == Tensor::Float32 &&

          phi_i.shape.size() == 1 &&
          phi_i.dtype == Tensor::Float32 &&

          wavelengths.shape.size() == 1 &&
          wavelengths.dtype == Tensor::Float32 &&

          ndf.shape.size() == 2 &&
          ndf.dtype == Tensor::Float32 &&

          sigma.shape.size() == 2 &&
          sigma.dtype == Tensor::Float32 &&

          vndf.shape.size() == 4 &&
          vndf.dtype == Tensor::Float32 &&
          vndf.shape[0] == phi_i.shape[0] &&
          vndf.shape[1] == theta_i.shape[0] &&

          luminance.shape.size() == 4 &&
          luminance.dtype == Tensor::Float32 &&
          luminance.shape[0] == phi_i.shape[0] &&
          luminance.shape[1] == theta_i.shape[0] &&
          luminance.shape[2] == luminance.shape[3] &&

          spectra.dtype == Tensor::Float32 &&
          spectra.shape.size() == 5 &&
          spectra.shape[0] == phi_i.shape[0] &&
          spectra.shape[1] == theta_i.shape[0] &&
          spectra.shape[2] == wavelengths.shape[0] &&
          spectra.shape[3] == spectra.shape[4] &&

          luminance.shape[2] == spectra.shape[3] &&
          luminance.shape[3] == spectra.shape[4] &&

          jacobian.shape.size() == 1 &&
          jacobian.shape[0] == 1 &&
          jacobian.dtype == Tensor::UInt8))
            throw std::runtime_error("Invalid file structure: " + tf.to_string());

    m_data = std::unique_ptr<BRDF::Data>(new BRDF::Data());

    m_data->isotropic = phi_i.shape[0] <= 2;
    m_data->jacobian  = ((uint8_t *) jacobian.data.get())[0];

    if (!m_data->isotropic) {
        float *phi_i_data = (float *) phi_i.data.get();
        int reduction = (int) std::rint((2 * Pi) /
            (phi_i_data[phi_i.shape[0] - 1] - phi_i_data[0]));
        if (reduction != 1)
            throw std::runtime_error("reduction != 1, not supported by this implementation");
    }

    /* Construct NDF interpolant data structure */
    m_data->ndf = Warp2D0(
        Vector2u(ndf.shape[1], ndf.shape[0]),
        (float *) ndf.data.get(),
        { }, { }, false, false
    );

    /* Construct projected surface area interpolant data structure */
    m_data->sigma = Warp2D0(
        Vector2u(sigma.shape[1], sigma.shape[0]),
        (float *) sigma.data.get(),
        { }, { }, false, false
    );

    /* Construct VNDF warp data structure */
    m_data->vndf = Warp2D2(
        Vector2u(vndf.shape[3], vndf.shape[2]),
        (float *) vndf.data.get(),
        {{ (uint32_t) phi_i.shape[0],
           (uint32_t) theta_i.shape[0] }},
        {{ (const float *) phi_i.data.get(),
           (const float *) theta_i.data.get() }}
    );

    /* Construct Luminance warp data structure */
    m_data->luminance = Warp2D2(
        Vector2u(luminance.shape[3], luminance.shape[2]),
        (float *) luminance.data.get(),
        {{ (uint32_t) phi_i.shape[0],
           (uint32_t) theta_i.shape[0] }},
        {{ (const float *) phi_i.data.get(),
           (const float *) theta_i.data.get() }}
    );

    /* Copy wavelength information */
    size_t size = wavelengths.shape[0];
    m_data->wavelengths.resize(size);
    for (size_t i = 0; i < size; ++i)
        m_data->wavelengths[i] = ((const float *) wavelengths.data.get())[i];

    /* Construct spectral interpolant */
    m_data->spectra = Warp2D3(
        Vector2u(spectra.shape[4], spectra.shape[3]),
        (float *) spectra.data.get(),
        {{ (uint32_t) phi_i.shape[0],
           (uint32_t) theta_i.shape[0],
           (uint32_t) wavelengths.shape[0] }},
        {{ (const float *) phi_i.data.get(),
           (const float *) theta_i.data.get(),
           (const float *) wavelengths.data.get() }},
        false, false
    );

    m_description = std::string((char*)description.data.get(), description.shape[0]);
}

BRDF::~BRDF() { }

// *****************************************************************************
// PDF interface
// *****************************************************************************

float BRDF::pdf(const Vector3f &wi, const Vector3f &wo) const {
    if (wi.z() <= 0 || wo.z() <= 0)
        return 0;

    Vector3f wm = normalize(wi + wo);

    /* Cartesian -> spherical coordinates */
    float theta_i = std::acos(wi.z()),
          phi_i   = std::atan2(wi.y(), wi.x()),
          theta_m = std::acos(wm.z()),
          phi_m   = std::atan2(wm.y(), wm.x());

    /* Spherical coordinates -> unit coordinate system */
    Vector2f u_wm = Vector2f(
        theta2u(theta_m),
        phi2u(m_data->isotropic ? (phi_m - phi_i) : phi_m)
    );
    u_wm.y() = u_wm.y() - std::floor(u_wm.y());

    Vector2f sample;
    float vndf_pdf, params[2] = { phi_i, theta_i };
    std::tie(sample, vndf_pdf) = m_data->vndf.invert(u_wm, params);

    float pdf = 1.f;
    #if POWITACQ_SAMPLE_LUMINANCE
        pdf = m_data->luminance.eval(sample, params);
    #endif

    float sin_theta_m = std::sqrt(1 - sqr(wm.z()));
    float jacobian = std::max(2.f * sqr(Pi) * u_wm.x() *
                              sin_theta_m, 1e-6f) * 4.f * dot(wi, wm);

    return vndf_pdf * pdf / jacobian;
}

// *****************************************************************************
// Eval interface
// *****************************************************************************

Spectrum BRDF::eval(const Vector3f &wi, const Vector3f &wo) const {
    if (wi.z() <= 0 || wo.z() <= 0)
        return zero();

    Vector3f wm = normalize(wi + wo);

    /* Cartesian -> spherical coordinates */
    float theta_i = std::acos(wi.z()),
          phi_i   = std::atan2(wi.y(), wi.x()),
          theta_m = std::acos(wm.z()),
          phi_m   = std::atan2(wm.y(), wm.x());

    /* Spherical coordinates -> unit coordinate system */
    Vector2f u_wi = Vector2f(theta2u(theta_i), phi2u(phi_i));
    Vector2f u_wm = Vector2f(
        theta2u(theta_m),
        phi2u(m_data->isotropic ? (phi_m - phi_i) : phi_m)
    );
    u_wm.y() = u_wm.y() - std::floor(u_wm.y());

    Vector2f sample;
    float vndf_pdf, params[2] = { phi_i, theta_i };
    std::tie(sample, vndf_pdf) = m_data->vndf.invert(u_wm, params);

    Spectrum fr = zero();
    for (int i = 0; i < (int) m_data->wavelengths.size(); ++i) {
        float params_fr[3] = { phi_i, theta_i, m_data->wavelengths[i] };

        fr[i] = m_data->spectra.eval(sample, params_fr);
    }

    fr *= m_data->ndf.eval(u_wm, params) /
            (4 * m_data->sigma.eval(u_wi, params));

    return fr;
}

// *****************************************************************************
// Sample interface
// *****************************************************************************


float BRDF::sample(const Vector2f &u, const Vector3f &wi,
                        size_t wave_length_index, Vector3f *wo_out, float *pdf_out) const {
    if (wi.z() <= 0) {
        if (wo_out)
            *wo_out = Vector3f(0.f);
        if (pdf_out)
            *pdf_out = 0;
        return 0.0f;
    }

    float theta_i = std::acos(wi.z()),
          phi_i   = std::atan2(wi.y(), wi.x());

    float params[2] = { phi_i, theta_i };
    Vector2f u_wi = Vector2f(theta2u(theta_i), phi2u(phi_i));
    Vector2f sample = Vector2f(u.y(), u.x());
    float lum_pdf = 1.f;

    #if POWITACQ_SAMPLE_LUMINANCE
        std::tie(sample, lum_pdf) =
            m_data->luminance.sample(sample, params);
    #endif

    Vector2f u_wm;
    float ndf_pdf;
    std::tie(u_wm, ndf_pdf) =
        m_data->vndf.sample(sample, params);

    float phi_m   = u2phi(u_wm.y()),
          theta_m = u2theta(u_wm.x());

    if (m_data->isotropic)
        phi_m += phi_i;

    /* Spherical -> Cartesian coordinates */
    float sin_phi_m = std::sin(phi_m),
          cos_phi_m = std::cos(phi_m),
          sin_theta_m = std::sin(theta_m),
          cos_theta_m = std::cos(theta_m);

    Vector3f wm = Vector3f(
        cos_phi_m * sin_theta_m,
        sin_phi_m * sin_theta_m,
        cos_theta_m
    );

    Vector3f wo = wm * 2.f * dot(wm, wi) - wi;
    if (wo.z() <= 0) {
        if (wo_out)
            *wo_out = Vector3f(0.f);
        if (pdf_out)
            *pdf_out = 0;
        return 0.0f;
    }

    float fr = 0.0f;
    float params_fr[3] = { phi_i, theta_i, m_data->wavelengths[wave_length_index] };

    fr = m_data->spectra.eval(sample, params_fr);

    fr *= m_data->ndf.eval(u_wm, params) /
            (4 * m_data->sigma.eval(u_wi, params));

    float jacobian = std::max(2.f * sqr(Pi) * u_wm.x() *
                              sin_theta_m, 1e-6f) * 4.f * dot(wi, wm);

    float pdf = ndf_pdf * lum_pdf / jacobian;

    if (wo_out)  (*wo_out)  = wo;
    if (pdf_out) (*pdf_out) = pdf;

    return fr / pdf;
}

Spectrum BRDF::sample(const Vector2f &u, const Vector3f &wi,
                      Vector3f *wo_out, float *pdf_out) const {
    if (wi.z() <= 0) {
        if (wo_out)
            *wo_out = Vector3f(0.f);
        if (pdf_out)
            *pdf_out = 0;
        return zero();
    }

    float theta_i = std::acos(wi.z()),
          phi_i   = std::atan2(wi.y(), wi.x());

    float params[2] = { phi_i, theta_i };
    Vector2f u_wi = Vector2f(theta2u(theta_i), phi2u(phi_i));
    Vector2f sample = Vector2f(u.y(), u.x());
    float lum_pdf = 1.f;

    #if POWITACQ_SAMPLE_LUMINANCE
        std::tie(sample, lum_pdf) =
            m_data->luminance.sample(sample, params);
    #endif

    Vector2f u_wm;
    float ndf_pdf;
    std::tie(u_wm, ndf_pdf) =
        m_data->vndf.sample(sample, params);

    float phi_m   = u2phi(u_wm.y()),
          theta_m = u2theta(u_wm.x());

    if (m_data->isotropic)
        phi_m += phi_i;

    /* Spherical -> Cartesian coordinates */
    float sin_phi_m = std::sin(phi_m),
          cos_phi_m = std::cos(phi_m),
          sin_theta_m = std::sin(theta_m),
          cos_theta_m = std::cos(theta_m);

    Vector3f wm = Vector3f(
        cos_phi_m * sin_theta_m,
        sin_phi_m * sin_theta_m,
        cos_theta_m
    );

    Vector3f wo = wm * 2.f * dot(wm, wi) - wi;
    if (wo.z() <= 0) {
        if (wo_out)
            *wo_out = Vector3f(0.f);
        if (pdf_out)
            *pdf_out = 0;
        return zero();
    }

    Spectrum fr = zero();
    for (int i = 0; i < (int) m_data->wavelengths.size(); ++i) {
        float params_fr[3] = { phi_i, theta_i, m_data->wavelengths[i] };

        fr[i] = m_data->spectra.eval(sample, params_fr);
    }

    fr *= m_data->ndf.eval(u_wm, params) /
            (4 * m_data->sigma.eval(u_wi, params));

    float jacobian = std::max(2.f * sqr(Pi) * u_wm.x() *
                              sin_theta_m, 1e-6f) * 4.f * dot(wi, wm);

    float pdf = ndf_pdf * lum_pdf / jacobian;

    if (wo_out)  (*wo_out)  = wo;
    if (pdf_out) (*pdf_out) = pdf;

    return fr / pdf;
}

void set_incident_angle(const Vector3f &wi, size_t theta_n, size_t phi_n)
{
    m_theta_n = theta_n;
    m_theta_n = phi_n;
    size_t n_points = theta_n * phi_n;
    m_samples.resize(n_points);
    m_pdfs.resize(n_points);
    m_scales.resize(n_points);

    m_wi = wi;
    if (wi.z() <= 0) {
        m_wo = Vector3f(0.f);
        m_pdfs.assign(n_points, 0);
        return;
    }

    float theta_i = std::acos(wi.z()),
          phi_i   = std::atan2(wi.y(), wi.x());

    m_params[0] = phi_i;
    m_params[1] = theta_i;
    Vector2f u_wi = Vector2f(theta2u(theta_i), phi2u(phi_i));

    for (float theta = 0; theta < theta_n; ++theta)
    {
        float v = float(theta + 0.5f) / theta_n;
        for (float phi = 0; phi < phi_n; ++phi)
        {
            float u = float(phi + 0.5f) / phi_n;
            size_t index = theta * n_phi + phi;

            m_samples[index] = Vector2f(v, u);
            float lum_pdf = 1.f;

            #if POWITACQ_SAMPLE_LUMINANCE
                std::tie(m_samples[index], lum_pdf) =
                    m_data->luminance.sample(m_samples[index], m_params);
            #endif

            Vector2f u_wm;
            float ndf_pdf;
            std::tie(u_wm, ndf_pdf) =
                m_data->vndf.sample(m_samples[index], m_params);

            float phi_m   = u2phi(u_wm.y()),
                  theta_m = u2theta(u_wm.x());
            if (m_data->isotropic)
                phi_m += phi_i;

            /* Spherical -> Cartesian coordinates */
            float sin_phi_m = std::sin(phi_m),
                  cos_phi_m = std::cos(phi_m),
                  sin_theta_m = std::sin(theta_m),
                  cos_theta_m = std::cos(theta_m);

            Vector3f wm = Vector3f(
                cos_phi_m * sin_theta_m,
                sin_phi_m * sin_theta_m,
                cos_theta_m
            );

            m_wo = wm * 2.f * dot(wm, wi) - wi;
            if (m_wo.z() <= 0) {
                m_wo = Vector3f(0.f);
                m_pdfs[index] = 0;
                continue;
            }

            float jacobian = std::max(2.f * sqr(Pi) * u_wm.x() *
                                      sin_theta_m, 1e-6f) * 4.f * dot(wi, wm);
            
            m_pdfs[index] = ndf_pdf * lum_pdf / jacobian;

            m_scales[index] = m_data->ndf.eval(u_wm, m_params) /
                        (4 * m_data->sigma.eval(u_wi, m_params));
        }
    }
}

POWITACQ_NAMESPACE_END
