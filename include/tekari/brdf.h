/* brdf.h - BRDF toolkit
by Jonathan Dupuy and Wenzel Jakob

   Do this:
      #define POWITACQ_BRDF_IMPLEMENTATION 1
   before you include this file in *one* C++ file to create the implementation.

   INTERFACING

   define POWITACQ_ASSERT(x) to avoid using assert.h.
   define POWITACQ_LOG(format, ...) to use your own logger (default prints in stdout)
*/

#ifndef POWITACQ_INCLUDE_BRDF_H
#define POWITACQ_INCLUDE_BRDF_H

#include <vector>
#include <string>
#include <memory>
#include <valarray>

namespace powitacq {

// *****************************************************************************
/* Exception API */
struct exc : public std::exception {
	exc(const char *fmt, ...);
	virtual ~exc() throw() {}
	const char *what() const throw() {return m_str.c_str();}
	std::string m_str;
};

// *****************************************************************************
/* Standalone vec2 and vec3 utilities */
struct vec2 {
	explicit vec2(float x = 0): x(x), y(x) {}
	vec2(float x, float y) : x(x), y(y) {}
	float& operator[](int i) {return (&x)[i];}
	const float& operator[](int i) const {return (&x)[i];}
	float x, y;
};
struct vec3 {
	explicit vec3(float x = 0): x(x), y(x), z(x) {}
	vec3(float x, float y, float z) : x(x), y(y), z(z) {}
	float& operator[](int i) {return (&x)[i];}
	const float& operator[](int i) const {return (&x)[i];}
	float x, y, z;
};

// *****************************************************************************
/* BRDF API */
class brdf {
	struct data;
	std::unique_ptr<data> m_data;
public:
	// spectrum type
	typedef std::valarray<float> value_type;
	const value_type zero_value() const;
	const std::vector<float> & wavelengths() const;

	// evaluate f_r * cos
	value_type eval(const vec3 &wi, const vec3 &wo) const;

    // importance sample f_r * cos from two uniform numbers
	value_type sample(const vec2 &u,
	                  const vec3 &wi,
	                  vec3 *wo = nullptr,
	                  float *pdf = nullptr) const;

	// evaluate the PDF of a sample
	float pdf(const vec3 &wi, const vec3 &wo) const;

	// ctor / dtor
	brdf(const std::string &path_to_file);
	~brdf();
};

} // namespace powitacq

//
//
//// end header file /////////////////////////////////////////////////////
#endif // POWITACQ_INCLUDE_BRDF_H


#ifdef POWITACQ_BRDF_IMPLEMENTATION
#include <cmath>
#include <cstdarg>
#include <iostream>     // std::ios, std::istream, std::cout
#include <fstream>      // std::filebuf
#include <cstring>      // memcpy
#include <cstdint>      // uint32_t
#include <limits>       // inf

#include <sstream>
#include <unordered_map>
#include <cstdio>

#ifndef POWITACQ_ASSERT
#	include <cassert>
#	define POWITACQ_ASSERT(x) assert(x)
#endif

#ifndef POWITACQ_LOG
#	include <cstdio>
#	define POWITACQ_LOG(format, ...) fprintf(stdout, format, ##__VA_ARGS__)
#endif

#ifdef _MSC_VER
#	pragma warning(disable: 4244) // possible loss of data
#endif

namespace powitacq {

// *****************************************************************************
// std math
using std::sqrt;
using std::cos;
using std::sin;
using std::tan;
using std::atan2;
using std::acos;
using std::modf;
using std::exp;
using std::log;
using std::pow;
using std::fabs;
using std::erf;
using std::floor;
using std::ceil;

namespace math {
	const float pi = 3.1415926535897932384626433832795f;
}

// *****************************************************************************
// utility API
template<typename T> static T min(const T &a, const T &b) {return a < b ? a : b;}
template<typename T> static T max(const T &a, const T &b) {return a > b ? a : b;}
template<typename T> static T clamp(const T &x, const T &a, const T &b) {return min(b, max(a, x));}
template<typename T> static T sat(const T &x) {return clamp(x, T(0), T(1));}
template<typename T> static T sqr(const T &x) {return x * x;}
template<typename T> static int sgn(T x) {return (T(0) < x) - (x < T(0));}
template<typename T>
static T max3(const T &x, const T &y, const T &z)
{
	T m = x;

	(m < y) && (m = y);
	(m < z) && (m = z);

	return m;
}

template<typename T>
static T inversesqrt(const T &x)
{
	POWITACQ_ASSERT(x > T(0));
	return (T(1) / sqrt(x));
}

exc::exc(const char *fmt, ...)
{
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, 256, fmt, args);
	va_end(args);

	m_str = std::string(buf);
}

// *****************************************************************************
// Vector API

#define OP operator
#define V3 vec3
V3 OP*(float a, const V3 &b) {return V3(a * b.x, a * b.y, a * b.z);}
V3 OP*(const V3 &a, float b) {return V3(b * a.x, b * a.y, b * a.z);}
V3 OP/(const V3 &a, float b) {return (1 / b) * a;}
V3 OP*(const V3 &a, const V3 &b) {return V3(a.x * b.x, a.y * b.y, a.z * b.z);}
V3 OP/(const V3 &a, const V3 &b) {return V3(a.x / b.x, a.y / b.y, a.z / b.z);}
V3 OP+(const V3 &a, const V3 &b) {return V3(a.x + b.x, a.y + b.y, a.z + b.z);}
V3 OP-(const V3 &a, const V3 &b) {return V3(a.x - b.x, a.y - b.y, a.z - b.z);}
V3& OP+=(V3 &a, const V3 &b) {a.x+= b.x; a.y+= b.y; a.z+= b.z; return a;}
V3& OP*=(V3 &a, const V3 &b) {a.x*= b.x; a.y*= b.y; a.z*= b.z; return a;}
V3& OP*=(V3 &a, float b) {a.x*= b; a.y*= b; a.z*= b; return a;}
#undef V3

#define V2 vec2
V2 OP*(float a, const V2 &b) {return V2(a * b.x, a * b.y);}
V2 OP*(const V2 &a, float b) {return V2(b * a.x, b * a.y);}
V2 OP/(const V2 &a, float b) {return (1 / b) * a;}
V2 OP*(const V2 &a, const V2 &b) {return V2(a.x * b.x, a.y * b.y);}
V2 OP+(const V2 &a, const V2 &b) {return V2(a.x + b.x, a.y + b.y);}
V2 OP-(const V2 &a, const V2 &b) {return V2(a.x - b.x, a.y - b.y);}
V2& OP+=(V2 &a, const V2 &b) {a.x+= b.x; a.y+= b.y; return a;}
V2& OP*=(V2 &a, const V2 &b) {a.x*= b.x; a.y*= b.y; return a;}
V2& OP*=(V2 &a, float b) {a.x*= b; a.y*= b; return a;}
#undef V2
#undef OP

static float dot(const vec2 &a, const vec2 &b)
{
	return (a.x * b.x + a.y * b.y);
}

static float dot(const vec3 &a, const vec3 &b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

static vec3 cross(const vec3 &a, const vec3 &b)
{
	return vec3(a.y * b.z - a.z * b.y,
	            a.z * b.x - a.x * b.z,
	            a.x * b.y - a.y * b.x);
}

static vec3 normalize(const vec3 &v)
{
	float mag_sqr = dot(v, v);
	return (inversesqrt(mag_sqr) * v);
}

// *****************************************************************************
// Internal Classes
// 
// *****************************************************************************

// *****************************************************************************
/* Standalone vec2 API */
template <typename T>
struct tvec2 {
	explicit tvec2(T x = 0): x(x), y(x) {}
	tvec2(T x, T y) : x(x), y(y) {}
    template <typename V>
        tvec2(const tvec2<V> &x) : x(x[0]), y(x[1]) {}
	T& operator[](int i) {return (&x)[i];}
	const T& operator[](int i) const {return (&x)[i];}
	T x, y;
};
typedef tvec2<int32_t>  ivec2;
typedef tvec2<uint32_t> uvec2;
typedef tvec2<float>    fvec2;

#define OP operator
#define V2 tvec2<T>
#define TT template <typename T>
TT V2 OP*(float a, const V2 &b) {return V2(a * b[0], a * b[1]);}
TT V2 OP*(const V2 &a, T b) {return V2(b * a[0], b * a[1]);}
TT V2 OP/(const V2 &a, T b) {return (1 / b) * a;}
TT V2 OP*(const V2 &a, const V2 &b) {return V2(a[0] * b[0], a[1] * b[1]);}
TT V2 OP+(const V2 &a, const V2 &b) {return V2(a[0] + b[0], a[1] + b[1]);}
TT V2 OP-(const V2 &a, const V2 &b) {return V2(a[0] - b[0], a[1] - b[1]);}
TT V2 OP-(const V2 &a, const T &b) {return V2(a[0] - b, a[1] - b);}
TT V2& OP-=(V2 &a, const V2 &b) {a[0]-= b[0]; a[1]-= b[1]; return a;}
TT V2& OP+=(V2 &a, const V2 &b) {a[0]+= b[0]; a[1]+= b[1]; return a;}
TT V2& OP*=(V2 &a, const V2 &b) {a[0]*= b[0]; a[1]*= b[1]; return a;}
TT V2& OP*=(V2 &a, T b) {a[0]*= b; a[1]*= b; return a;}
TT T hprod(const V2 &v) { return v[0] * v[1];}
TT V2 min(const V2 &a, const V2 &b) { return V2(min(a[0], b[0]), min(a[1], b[1])); }
TT V2 max(const V2 &a, const V2 &b) { return V2(max(a[0], b[0]), max(a[1], b[1])); }
TT V2 clamp(const V2 &x, const V2 &a, const V2 &b) { return min(max(x, a), b); }
TT V2 clamp(const V2 &x, const T &a, const T &b) { return clamp(x, tvec2<T>(a), tvec2<T>(b)); }
#undef TT
#undef V2
#undef OP
// *****************************************************************************

using Float = float;
using Vector2f = fvec2;
using Vector2u = uvec2;
using Vector2i = ivec2;

#define Throw(reason) throw std::runtime_error(reason)
static const float  OneMinusEpsilon = 0.999999940395355225f;

/// Type trait to inspect the return and argument types of functions
template <typename T, typename SFINAE = void> struct function_traits { };

/// Vanilla function
template <typename R, typename... A> struct function_traits<R(*)(A...)> {
    using Args = std::tuple<A...>;
    using Return = R;
};

/// Method
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...)> {
    using Class = C;
    using Args = std::tuple<A...>;
    using Return = R;
};

/// Method (const)
template <typename C, typename R, typename... A> struct function_traits<R(C::*)(A...) const> {
    using Class = C;
    using Args = std::tuple<A...>;
    using Return = R;
};

/// Lambda function -- strip lambda closure and delegate back to ``function_traits``
template <typename F>
struct function_traits<
    F, std::enable_if_t<std::is_member_function_pointer<decltype(
           &std::remove_reference_t<F>::operator())>::value>>
    : function_traits<decltype(&std::remove_reference_t<F>::operator())> { };

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
 *
 * \remark This function is intended for vectorized predicates and additionally
 * accepts a mask as an input. This mask can be used to disable some of the
 * array entries. The mask is passed to the predicate as a second parameter.
 */
template <typename Size, typename Predicate,
          typename Args  = typename function_traits<Predicate>::Args,
          typename Index = std::decay_t<std::tuple_element_t<0, Args>>,
          typename Mask  = std::decay_t<std::tuple_element_t<1, Args>>>
 Index find_interval(const Size &left, const Size &right,
                               const Predicate &pred, const Mask &active_in) {
    using IndexMask         = bool;
    using SignedIndex       = int32_t;
    using IndexScalar       = int32_t;

    Size initial_size = right - left;
    Index first(left + 1),
          size(initial_size - 2);

    while (size > 0) {
        Index half   = size >> 1,
              middle = first + half;

        /* Evaluate the predicate */
        bool pred_result = pred(middle, Mask(true));
        
        /* .. and recurse into the left or right */
        if (pred_result)
            first = middle + 1;

        /* Update the remaining interval size */
        size = pred_result ? size - (half + 1) : half;
    }

    return Index(clamp(first - 1, left, right - 2));
}

template <typename Size, typename Predicate, typename Mask>
 auto find_interval(const Size &size, const Predicate &pred, const Mask &active) {
    return find_interval(Size(0), size, pred, active);
}

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
 *
 * \remark The Python API exposes explicitly instantiated versions of this
 * class named Hierarchical2D0, Hierarchical2D1, and Hierarchical2D2 for data
 * that depends on 0, 1, and 2 parameters, respectively.
 */
template <size_t Dimension = 0> class Marginal2D {
private:
    //using FloatStorage = std::unique_ptr<Float[], enoki::aligned_deleter>;
    using FloatStorage = std::vector<Float>;

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
    Marginal2D(const Vector2u &size, const Float *data,
               std::array<uint32_t, Dimension> param_res = {},
               std::array<const Float *, Dimension> param_values = {},
               bool normalize = true, bool build_cdf = true): 
        m_size(size),
		m_patch_size({1.f / (m_size[0] - 1u), 1.f / (m_size[1] - 1u)}),
        m_inv_patch_size({1.f * (m_size[0] - 1u), 1.f * (m_size[1] - 1u)}) {

        if (build_cdf && !normalize)
            Throw("Marginal2D: build_cdf implies normalize=true");

        /* Keep track of the dependence on additional parameters (optional) */
        uint32_t slices = 1;
        for (int i = (int) Dimension - 1; i >= 0; --i) {
            if (param_res[i] < 1)
                Throw("warp::Marginal2D(): parameter resolution must be >= 1!");

            m_param_size[i] = param_res[i];
            //m_param_values[i] = FloatStorage(enoki::alloc<Float>(param_res[i]));
            m_param_values[i] = FloatStorage(param_res[i]);
            memcpy(m_param_values[i].data(), param_values[i],
                   sizeof(Float) * param_res[i]);
            m_param_strides[i] = param_res[i] > 1 ? slices : 0;
            slices *= m_param_size[i];
        }

        uint32_t n_values = hprod(size);

        m_data = FloatStorage(slices * n_values);

        if (build_cdf) {
            m_marginal_cdf = FloatStorage(slices * m_size[1]);
            m_conditional_cdf = FloatStorage(slices * n_values);

            Float *marginal_cdf = m_marginal_cdf.data(),
                  *conditional_cdf = m_conditional_cdf.data(),
                  *data_out = m_data.data();

            for (uint32_t slice = 0; slice < slices; ++slice) {
                /* Construct conditional CDF */
                for (uint32_t y = 0; y < m_size[1]; ++y) {
                    double sum = 0.0;
                    size_t i = y * size[0];
                    conditional_cdf[i] = 0.f;
                    for (uint32_t x = 0; x < m_size[0] - 1; ++x, ++i) {
                        sum += .5 * ((double) data[i] + (double) data[i + 1]);
                        conditional_cdf[i + 1] = (Float) sum;
                    }
                }

                /* Construct marginal CDF */
                marginal_cdf[0] = 0.f;
                double sum = 0.0;
                for (uint32_t y = 0; y < m_size[1] - 1; ++y) {
                    sum += .5 * ((double) conditional_cdf[(y + 1) * size[0] - 1] +
                                 (double) conditional_cdf[(y + 2) * size[0] - 1]);
                    marginal_cdf[y + 1] = (Float) sum;
                }

                /* Normalize CDFs and PDF (if requested) */
                Float normalization = 1.f / marginal_cdf[m_size[1] - 1];
                for (size_t i = 0; i < n_values; ++i)
                    conditional_cdf[i] *= normalization;
                for (size_t i = 0; i < m_size[1]; ++i)
                    marginal_cdf[i] *= normalization;
                for (size_t i = 0; i < n_values; ++i)
                    data_out[i] = data[i] * normalization;

                marginal_cdf += m_size[1];
                conditional_cdf += n_values;
                data_out += n_values;
                data += n_values;
            }
        } else {
            Float *data_out = m_data.data();

            for (uint32_t slice = 0; slice < slices; ++slice) {
                Float normalization = 1.f / hprod(m_inv_patch_size);
                if (normalize) {
                    double sum = 0.0;
                    for (uint32_t y = 0; y < m_size[1] - 1; ++y) {
                        size_t i = y * size[0];
                        for (uint32_t x = 0; x < m_size[0] - 1; ++x, ++i) {
                            Float v00 = data[i],
                                  v10 = data[i + 1],
                                  v01 = data[i + size[0]],
                                  v11 = data[i + 1 + size[0]],
                                  avg = .25f * (v00 + v10 + v01 + v11);
                            sum += (double) avg;
                        }
                    }
                    normalization = Float(1.0 / sum);
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
    template <typename Vector2f, typename Value = Float>
    std::pair<Vector2f, Value> sample(Vector2f sample, const Value *param = nullptr,
                                      bool active = true) const {
        using UInt32 = uint32_t;
        using Mask = bool;

        /* Avoid degeneracies at the extrema */
        sample = clamp(sample, 1.f - OneMinusEpsilon, OneMinusEpsilon);

        /* Look up parameter-related indices and weights (if Dimension != 0) */
        Value param_weight[2 * ArraySize];
        UInt32 slice_offset = 0u;
        for (size_t dim = 0; dim < Dimension; ++dim) {
            if (m_param_size[dim] == 1) {
                param_weight[2 * dim] = 1.f;
                param_weight[2 * dim + 1] = 0.f;
                continue;
            }

            UInt32 param_index = find_interval(
                m_param_size[dim],
                [&](UInt32 idx, Mask active_) {
                    return m_param_values[dim].data()[idx] <= param[dim];
                },
                active);

            Value p0 = m_param_values[dim][param_index],
                  p1 = m_param_values[dim][param_index + 1];

            param_weight[2 * dim + 1] =
                clamp((param[dim] - p0) / (p1 - p0), 0.f, 1.f);
            param_weight[2 * dim] = 1.f - param_weight[2 * dim + 1];
            slice_offset += m_param_strides[dim] * param_index;
        }

        /* Sample the row first */
        UInt32 offset = 0;
        if (Dimension != 0)
            offset = slice_offset * m_size[1];

        auto fetch_marginal = [&](UInt32 idx, Mask active_)  -> Value {
            return lookup<Dimension>(m_marginal_cdf.data(), offset + idx,
                                     m_size[1], param_weight, active_);
        };

        UInt32 row = find_interval(
            m_size[1],
            [&](UInt32 idx, Mask active_)  -> Mask {
                return fetch_marginal(idx, active_) < sample[1];
            },
            active);

        sample[1] -= fetch_marginal(row, active);

        uint32_t slice_size = hprod(m_size);
        offset = row * m_size[0];
        if (Dimension != 0)
            offset += slice_offset * slice_size;

        Value r0 = lookup<Dimension>(m_conditional_cdf.data(),
                                     offset + m_size[0] - 1, slice_size,
                                     param_weight, active),
              r1 = lookup<Dimension>(m_conditional_cdf.data(),
                                     offset + (m_size[0] * 2 - 1), slice_size,
                                     param_weight, active);

        Mask is_const = fabs(r0 - r1) < 1e-4f * (r0 + r1);
        sample[1] = is_const ? 2.f * sample[1] :
            r0 - sqrtf(r0 * r0 - 2.f * sample[1] * (r0 - r1));
        sample[1] /= is_const ? r0 + r1 : r0 - r1;

        /* Sample the column next */
        sample[0] *= (1.f - sample[1]) * r0 + sample[1] * r1;

        auto fetch_conditional = [&](UInt32 idx, Mask active_)  -> Value {
            Value v0 = lookup<Dimension>(m_conditional_cdf.data(), offset + idx,
                                         slice_size, param_weight, active_),
                  v1 = lookup<Dimension>(m_conditional_cdf.data() + m_size[0],
                                         offset + idx, slice_size, param_weight, active_);

            return (1.f - sample[1]) * v0 + sample[1] * v1;
        };

        UInt32 col = find_interval(
            m_size[0],
            [&](UInt32 idx, Mask active_)  -> Mask {
                return fetch_conditional(idx, active_) < sample[0];
            },
            active);

        sample[0] -= fetch_conditional(col, active);

        offset += col;

        Value v00 = lookup<Dimension>(m_data.data(), offset, slice_size,
                                      param_weight, active),
              v10 = lookup<Dimension>(m_data.data() + 1, offset, slice_size,
                                      param_weight, active),
              v01 = lookup<Dimension>(m_data.data() + m_size[0], offset,
                                      slice_size, param_weight, active),
              v11 = lookup<Dimension>(m_data.data() + m_size[0] + 1, offset,
                                      slice_size, param_weight, active),
              c0  = std::fma((1.f - sample[1]), v00, sample[1] * v01),
              c1  = std::fma((1.f - sample[1]), v10, sample[1] * v11);

        is_const = fabs(c0 - c1) < 1e-4f * (c0 + c1);
        sample[0] = is_const ? 2.f * sample[0] :
            c0 - sqrtf(c0 * c0 - 2.f * sample[0] * (c0 - c1));
        sample[0] /= is_const ? c0 + c1 : c0 - c1;

        return {
            (Vector2f(col, row) + sample) * m_patch_size,
            ((1.f - sample[0]) * c0 + sample[0] * c1) * hprod(m_inv_patch_size)
        };
    }

    /// Inverse of the mapping implemented in \c sample()
    template <typename Vector2f, typename Value = Float>
    std::pair<Vector2f, Value> invert(Vector2f sample, const Value *param = nullptr,
                                      bool active = true) const {
        using UInt32 = uint32_t;
        using Mask = bool;

        /* Look up parameter-related indices and weights (if Dimension != 0) */
        Value param_weight[2 * ArraySize];
        UInt32 slice_offset = 0u;
        for (size_t dim = 0; dim < Dimension; ++dim) {
            if (m_param_size[dim] == 1) {
                param_weight[2 * dim] = 1.f;
                param_weight[2 * dim + 1] = 0.f;
                continue;
            }

            UInt32 param_index = find_interval(
                m_param_size[dim],
                [&](UInt32 idx, Mask active_) {
                    return m_param_values[dim][idx] <= param[dim];
                },
                active);

            Value p0 = m_param_values[dim][param_index],
                  p1 = m_param_values[dim][param_index + 1];

            param_weight[2 * dim + 1] =
                clamp((param[dim] - p0) / (p1 - p0), 0.f, 1.f);
            param_weight[2 * dim] = 1.f - param_weight[2 * dim + 1];
            slice_offset += m_param_strides[dim] * param_index;
        }

        /* Fetch values at corners of bilinear patch */
        sample *= m_inv_patch_size;
        Vector2u pos = min(Vector2u(sample), m_size - 2u);
        sample -= Vector2f(pos[0], pos[1]);

        UInt32 offset = pos[0] + pos[1] * m_size[0];
        uint32_t slice_size = hprod(m_size);
        if (Dimension != 0)
            offset += slice_offset * slice_size;

        /* Invert the X component */
        Value v00 = lookup<Dimension>(m_data.data(), offset, slice_size,
                                      param_weight, active),
              v10 = lookup<Dimension>(m_data.data() + 1, offset, slice_size,
                                      param_weight, active),
              v01 = lookup<Dimension>(m_data.data() + m_size[0], offset, slice_size,
                                      param_weight, active),
              v11 = lookup<Dimension>(m_data.data() + m_size[0] + 1, offset, slice_size,
                                      param_weight, active);

        Vector2f w1 = sample, w0 = Vector2f(1.f) - w1;

        Value c0  = std::fma(w0[1], v00, w1[1] * v01),
              c1  = std::fma(w0[1], v10, w1[1] * v11),
              pdf = std::fma(w0[0], c0, w1[0] * c1);

        sample[0] *= c0 + .5f * sample[0] * (c1 - c0);

        Value v0 = lookup<Dimension>(m_conditional_cdf.data(), offset,
                                     slice_size, param_weight, active),
              v1 = lookup<Dimension>(m_conditional_cdf.data() + m_size[0],
                                     offset, slice_size, param_weight, active);

        sample[0] += (1.f - sample[1]) * v0 + sample[1] * v1;

        offset = pos[1] * m_size[0];
        if (Dimension != 0)
            offset += slice_offset * slice_size;

        Value r0 = lookup<Dimension>(m_conditional_cdf.data(),
                                     offset + m_size[0] - 1, slice_size,
                                     param_weight, active),
              r1 = lookup<Dimension>(m_conditional_cdf.data(),
                                     offset + (m_size[0] * 2 - 1), slice_size,
                                     param_weight, active);

        sample[0] /= (1.f - sample[1]) * r0 + sample[1] * r1;

        /* Invert the Y component */
        sample[1] *= r0 + .5f * sample[1] * (r1 - r0);

        offset = pos[1];
        if (Dimension != 0)
            offset += slice_offset * m_size[1];

        sample[1] += lookup<Dimension>(m_marginal_cdf.data(), offset,
                                        m_size[1], param_weight, active);

        return { sample, pdf * hprod(m_inv_patch_size) };
    }

    /**
     * \brief Evaluate the density at position \c pos. The distribution is
     * parameterized by \c param if applicable.
     */
    template <typename Vector2f, typename Value = Float>
    Value eval(Vector2f pos, const Value *param = nullptr,
               bool active = true) const {
        using UInt32 = uint32_t;
        using Mask = bool;

        /* Look up parameter-related indices and weights (if Dimension != 0) */
        Value param_weight[2 * ArraySize];
        UInt32 slice_offset = 0u;
        for (size_t dim = 0; dim < Dimension; ++dim) {
            if (m_param_size[dim] == 1) {
                param_weight[2 * dim] = 1.f;
                param_weight[2 * dim + 1] = 0.f;
                continue;
            }

            UInt32 param_index = find_interval(
                m_param_size[dim],
                [&](UInt32 idx, Mask active_) {
                    return m_param_values[dim][idx] <= param[dim];
                },
                active);

            Value p0 = m_param_values[dim][param_index],
                  p1 = m_param_values[dim][param_index + 1];

            param_weight[2 * dim + 1] =
                clamp((param[dim] - p0) / (p1 - p0), 0.f, 1.f);
            param_weight[2 * dim] = 1.f - param_weight[2 * dim + 1];
            slice_offset += m_param_strides[dim] * param_index;
        }

        /* Compute linear interpolation weights */
        pos *= m_inv_patch_size;
        Vector2u offset = min(Vector2u(pos[0], pos[1]), Vector2u(m_size) - Vector2u(2u));
        Vector2f w1 = pos - Vector2f(Vector2i(offset)),
                 w0 = Vector2f(1.f) - w1;

        UInt32 index = offset[0] + offset[1] * m_size[0];

        uint32_t size = hprod(m_size);
        if (Dimension != 0)
            index += slice_offset * size;

        Value v00 = lookup<Dimension>(m_data.data(), index, size,
                                      param_weight, active),
              v10 = lookup<Dimension>(m_data.data() + 1, index, size,
                                      param_weight, active),
              v01 = lookup<Dimension>(m_data.data() + m_size[0], index, size,
                                      param_weight, active),
              v11 = lookup<Dimension>(m_data.data() + m_size[0] + 1, index, size,
                                      param_weight, active);

        return std::fma(w0[1],  std::fma(w0[0], v00, w1[0] * v10),
                     w1[1] * std::fma(w0[0], v01, w1[0] * v11)) * hprod(m_inv_patch_size);
    }

private:
        template <size_t Dim, typename Index, typename Value,
                  std::enable_if_t<Dim != 0, int> = 0>
         Value lookup(const Float *data, Index i0,
                                uint32_t size,
                                const Value *param_weight,
                                bool active) const {
            Index i1 = i0 + m_param_strides[Dim - 1] * size;

            Value w0 = param_weight[2 * Dim - 2],
                  w1 = param_weight[2 * Dim - 1],
                  v0 = lookup<Dim - 1>(data, i0, size, param_weight, active),
                  v1 = lookup<Dim - 1>(data, i1, size, param_weight, active);

            return std::fma(v0, w0, v1 * w1);
        }

        template <size_t Dim, typename Index, typename Value,
                  std::enable_if_t<Dim == 0, int> = 0>
         Value lookup(const Float *data, Index index, uint32_t,
                                const Value *, bool active) const {
            return data[index];
        }

private:
    /// Resolution of the discretized density function
    Vector2u m_size;

    /// Size of a bilinear patch in the unit square
    Vector2f m_patch_size, m_inv_patch_size;

    /// Resolution of each parameter (optional)
    uint32_t m_param_size[ArraySize];

    /// Stride per parameter in units of sizeof(Float)
    uint32_t m_param_strides[ArraySize];

    /// Discretization of each parameter domain
    FloatStorage m_param_values[ArraySize];

    /// Density values
    FloatStorage m_data;

    /// Marginal and conditional PDFs
    FloatStorage m_marginal_cdf;
    FloatStorage m_conditional_cdf;
};

typedef Marginal2D<0> Warp2D0;
typedef Marginal2D<2> Warp2D2;
typedef Marginal2D<3> Warp2D3;

#undef Throw

// *****************************************************************************
// Private Tensor File API
class Tensor {
public:
	// Data type of the tensor's fields
    enum EType {
        /* Invalid/unspecified */
        EInvalid = 0,

        /* Signed and unsigned integer values */
        EUInt8,  EInt8,
        EUInt16, EInt16,
        EUInt32, EInt32,
        EUInt64, EInt64,

        /* Floating point values */
        EFloat16, EFloat32, EFloat64,
    };

    struct Field {
		// Data type of the tensor's fields
    	EType dtype;
    	// offset in the file (unused)
    	size_t offset;
        /// Specifies both rank and size along each dimension
        std::vector<size_t> shape;
        /// Const pointer to the start of the tensor
        const void* data;
    };

    /// Map the specified file into memory
    Tensor(const std::string &filename);
    
    /// Destructor
    ~Tensor();

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

static std::ostream &operator<<(std::ostream &os, Tensor::EType value)
{
    switch(value)
    {
        case Tensor::EInvalid:  os << "invalid"; break;
        case Tensor::EUInt8 :   os << "uint8_t"; break;
        case Tensor::EInt8:     os << "int8_t"; break;
        case Tensor::EUInt16:   os << "uint16_t"; break;
        case Tensor::EInt16:    os << "int16_t"; break;
        case Tensor::EUInt32:   os << "uint32_t"; break;
        case Tensor::EInt32:    os << "int8_t"; break;
        case Tensor::EUInt64:   os << "uint64_t"; break;
        case Tensor::EInt64:    os << "int64_t"; break;
        case Tensor::EFloat16:  os << "float16_t"; break;
        case Tensor::EFloat32:  os << "float32_t"; break;
        case Tensor::EFloat64:  os << "float64_t"; break;
        default:                os << "unkown"; break;
    }
    return os;
}

static size_t type_size(Tensor::EType value)
{
    switch(value)
    {
        case Tensor::EInvalid:  return 0; break;
        case Tensor::EUInt8 :   return 1; break;
        case Tensor::EInt8:     return 1; break;
        case Tensor::EUInt16:   return 2; break;
        case Tensor::EInt16:    return 2; break;
        case Tensor::EUInt32:   return 4; break;
        case Tensor::EInt32:    return 4; break;
        case Tensor::EUInt64:   return 8; break;
        case Tensor::EInt64:    return 8; break;
        case Tensor::EFloat16:  return 2; break;
        case Tensor::EFloat32:  return 4; break;
        case Tensor::EFloat64:  return 8; break;
        default:                return 0; break;
    }
}

Tensor::Tensor(const std::string &filename)
: m_filename(filename)
{
    // Helpfull macros to limit error-handling code duplication
#define ASSERT(cond, msg)                              \
    do {                                               \
        if (!(cond)) {                                 \
            for(auto field: m_fields)                  \
                free((void*)field.second.data);                      \
            m_fields.clear();                          \
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
        ASSERT(dtype != EInvalid && dtype <= EFloat64, "Invalid tensor file: unknown type.");

        std::vector<size_t> shape(ndim);
        size_t total_size = type_size((EType)dtype);       // no need to check here, line 43 already removes invalid types
        for (size_t j = 0; j < (size_t) ndim; ++j) {
            uint64_t size_value;
            SAFE_READ(&size_value, sizeof(size_value), 1);
            shape[j] = (size_t) size_value;
            total_size *= shape[j];
        }

        void* data = malloc(total_size);
        ASSERT(data != nullptr, "Unable to allocate enough memory.");

        long cur_pos = ftell(file);
        ASSERT(cur_pos != -1, "Unable to tell current cursor position.");
        ASSERT(fseek(file, offset, SEEK_SET) != -1, "Unable to seek to tensor offset.");
        SAFE_READ(data, 1, total_size);
        ASSERT(fseek(file, cur_pos, SEEK_SET) != -1, "Unable to seek back to current position");

        m_fields[name] =
            Field{ (EType) dtype, static_cast<size_t>(offset), shape, data };
    }
    
    fclose(file);

#undef SAFE_READ
#undef ASSERT
}

/// Does the file contain a field of the specified name?
bool Tensor::has_field(const std::string &name) const
{
    return m_fields.find(name) != m_fields.end();
}

/// Return a data structure with information about the specified field
const Tensor::Field &Tensor::field(const std::string &name) const
{
    auto it = m_fields.find(name);
    if (it == m_fields.end())
        throw std::runtime_error("Tensor: Unable to find field " + name);
    return it->second;
}

/// Return a human-readable summary
std::string Tensor::to_string() const
{
    std::ostringstream oss;
    oss << "Tensor[" << std::endl
        << "  filename = \"" << m_filename << "\"," << std::endl
        << "  size = " << size() << "," << std::endl
        << "  fields = {" << std::endl;

    size_t ctr = 0;
    for (auto it : m_fields) {
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

/// Destructor
Tensor::~Tensor()
{
    for(auto& field: m_fields)
        free((void*)field.second.data);
}

// *****************************************************************************
// Internal data
struct brdf::data
{
	Warp2D0 ndf;
    Warp2D0 sigma;
    Warp2D2 vndf;
    Warp2D2 luminance;
    Warp2D3 spectra;
	std::vector<float> wavelengths;
	bool isotropic;
	bool jacobian;
	int reduction;
};

// *****************************************************************************
// BRDF API

template <typename Value> Value u2theta(Value u) {
	return sqr(u) * (math::pi / 2.f);
}

template <typename Value> Value u2phi(Value u) {
	return (2.f * u - 1.f) * math::pi;
}

template <typename Value> Value theta2u(Value theta) {
	return sqrt(theta * (2.f / math::pi));
}

template <typename Value> Value phi2u(Value phi) {
	return (phi + math::pi) / (2.f * math::pi);
}

// -----------------------------------------------------------------------------
// zero value
const brdf::value_type brdf::zero_value() const 
{
	return brdf::value_type(0.f, m_data->wavelengths.size());
}

// -----------------------------------------------------------------------------
// get the wavelengths sample points
const std::vector<float> & brdf::wavelengths() const
{
	return m_data->wavelengths;
}

// -----------------------------------------------------------------------------
// Dtor
brdf::~brdf()
{

}

// -----------------------------------------------------------------------------
// Ctor
brdf::brdf(const std::string &path_to_file)
{
	Tensor tf = Tensor(path_to_file);
	auto theta_i = tf.field("theta_i");
	auto phi_i = tf.field("phi_i");
	auto ndf = tf.field("ndf");
	auto sigma = tf.field("sigma");
	auto vndf = tf.field("vndf");
	auto spectra = tf.field("spectra");
	auto luminance = tf.field("luminance");
	auto wavelengths = tf.field("wavelengths");
	auto description = tf.field("description");
	auto jacobian = tf.field("jacobian");

	if (!(description.shape.size() == 1 &&
              description.dtype == Tensor::EUInt8 &&

              theta_i.shape.size() == 1 &&
              theta_i.dtype == Tensor::EFloat32 &&

              phi_i.shape.size() == 1 &&
              phi_i.dtype == Tensor::EFloat32 &&

              wavelengths.shape.size() == 1 &&
              wavelengths.dtype == Tensor::EFloat32 &&

              ndf.shape.size() == 2 &&
              ndf.dtype == Tensor::EFloat32 &&

              sigma.shape.size() == 2 &&
              sigma.dtype == Tensor::EFloat32 &&

              vndf.shape.size() == 4 &&
              vndf.dtype == Tensor::EFloat32 &&
              vndf.shape[0] == phi_i.shape[0] &&
              vndf.shape[1] == theta_i.shape[0] &&

              luminance.shape.size() == 4 &&
              luminance.dtype == Tensor::EFloat32 &&
              luminance.shape[0] == phi_i.shape[0] &&
              luminance.shape[1] == theta_i.shape[0] &&
              luminance.shape[2] == luminance.shape[3] &&

              spectra.dtype == Tensor::EFloat32 &&
              spectra.shape.size() == 5 &&
              spectra.shape[0] == phi_i.shape[0] &&
              spectra.shape[1] == theta_i.shape[0] &&
              spectra.shape[2] == wavelengths.shape[0] &&
              spectra.shape[3] == spectra.shape[4] &&

              luminance.shape[2] == spectra.shape[3] &&
              luminance.shape[3] == spectra.shape[4] &&

              jacobian.shape.size() == 1 &&
              jacobian.shape[0] == 1 &&
              jacobian.dtype == Tensor::EUInt8))
			throw exc("Invalid file structure: %s", tf.to_string().c_str());

	m_data = std::unique_ptr<brdf::data>(new brdf::data);

	m_data->isotropic = phi_i.shape[0] <= 2;
	m_data->jacobian  = ((uint8_t *) jacobian.data)[0];

	if (!m_data->isotropic) {
		float *phi_i_data = (float *) phi_i.data;
		m_data->reduction = (int) std::rint((2 * math::pi) /
			(phi_i_data[phi_i.shape[0] - 1] - phi_i_data[0]));
	}

	/* Construct NDF interpolant data structure */
	m_data->ndf = Warp2D0(
		Vector2u(ndf.shape[1], ndf.shape[0]),
		(Float *) ndf.data,
		{ }, { }, false, false
	);

	/* Construct projected surface area interpolant data structure */
	m_data->sigma = Warp2D0(
		Vector2u(sigma.shape[1], sigma.shape[0]),
		(Float *) sigma.data,
		{ }, { }, false, false
	);

	/* Construct VNDF warp data structure */
	m_data->vndf = Warp2D2(
		Vector2u(vndf.shape[3], vndf.shape[2]),
		(Float *) vndf.data,
		{{ (uint32_t) phi_i.shape[0],
			(uint32_t) theta_i.shape[0] }},
		{{ (const Float *) phi_i.data,
			(const Float *) theta_i.data }}
	);

	/* Construct Luminance warp data structure */
	m_data->luminance = Warp2D2(
		Vector2u(luminance.shape[3], luminance.shape[2]),
		(Float *) luminance.data,
		{{ (uint32_t) phi_i.shape[0],
			(uint32_t) theta_i.shape[0] }},
		{{ (const Float *) phi_i.data,
			(const Float *) theta_i.data }}
	);

	/* Construct spectral interpolant */
	int size = wavelengths.shape[0];
	//m_data->size = wavelengths.shape[0];
	m_data->wavelengths.resize(size);
	memcpy(m_data->wavelengths.data(), 
	       wavelengths.data,
		   sizeof(float) * size);
	m_data->spectra = Warp2D3(
		Vector2u(spectra.shape[4], spectra.shape[3]),
		(Float *) spectra.data,
		{{ (uint32_t) phi_i.shape[0],
			(uint32_t) theta_i.shape[0],
			(uint32_t) wavelengths.shape[0] }},
		{{ (const Float *) phi_i.data,
			(const Float *) theta_i.data,
			(const Float *) wavelengths.data }},
		false, false
	);
}

// vec3
// brdf::u2_to_s2(const vec2 &u, const vec3 &) const
// {
// 	return vec3(0, 0, 1);
// }

// vec2
// brdf::s2_to_u2(const vec3 &wo, const vec3 &) const
// {
// 	POWITACQ_ASSERT(wo.z >= 0 && "invalid outgoing direction");
// 	return vec2(0);
// }

// -----------------------------------------------------------------------------
// PDF interface
float brdf::pdf(const vec3 &wi, const vec3 &wo) const
{
	vec3 tmp = wi + wo;
	float_t nrm = dot(tmp, tmp);

	if (wi.z > 0 && nrm > 0) {
		vec3 wm = tmp * inversesqrt(nrm);

		if (m_data->reduction >= 2) {
            float sy = wi.y,
                  sx = (m_data->reduction == 4) ? wi.x : sy;

#if 0 //TODO: port
            wi.x = mulsign_neg(wi.x, sx);
            wi.y = mulsign_neg(wi.y, sy);
            wo.x = mulsign_neg(wo.x, sx);
            wo.y = mulsign_neg(wo.y, sy);
#endif
        }

		/* Cartesian -> spherical coordinates */
		float theta_i = acos(wi.z),
		      phi_i   = atan2(wi.y, wi.x),
			  theta_m = acos(wm.z),
			  phi_m   = atan2(wm.y, wm.x);

		/* Spherical coordinates -> unit coordinate system */
		fvec2 u_wi = fvec2(theta2u(theta_i), phi2u(phi_i));
		fvec2 u_wm = fvec2(
			theta2u(theta_m), 
			phi2u(m_data->isotropic ? (phi_m - phi_i) : phi_m)
		);
		u_wm[1] = u_wm[1] - floor(u_wm[1]);

		fvec2 sample;
		float vndf_pdf, params[2] = { phi_i, theta_i };
		std::tie(sample, vndf_pdf) = m_data->vndf.invert(u_wm, params);

		float pdf = 1.f;
		#if 1 // use liminance importance sampling
			pdf = m_data->luminance.eval(sample, params);
		#endif

		float sin_theta_m = sqrt(1 - sqr(wm.z));
		float jacobian = max(2.f * sqr(math::pi) * u_wm[0] *
									sin_theta_m, 1e-6f) * 4.f * dot(wi, wm);

		return vndf_pdf * pdf / jacobian;
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Eval interface
brdf::value_type brdf::eval(const vec3 &wi, const vec3 &wo) const
{
	vec3 tmp = wi + wo;
	float_t nrm = dot(tmp, tmp);

	if (wi.z > 0 && wo.z > 0 && nrm > 0) {
		vec3 wm = tmp * inversesqrt(nrm);

		if (m_data->reduction >= 2) {
            float sy = wi.y,
                  sx = (m_data->reduction == 4) ? wi.x : sy;

#if 0 //TODO: port
            wi.x = mulsign_neg(wi.x, sx);
            wi.y = mulsign_neg(wi.y, sy);
            wo.x = mulsign_neg(wo.x, sx);
            wo.y = mulsign_neg(wo.y, sy);
#endif
        }

		/* Cartesian -> spherical coordinates */
		float theta_i = acos(wi.z),
		      phi_i   = atan2(wi.y, wi.x),
			  theta_m = acos(wm.z),
			  phi_m   = atan2(wm.y, wm.x);

		/* Spherical coordinates -> unit coordinate system */
		fvec2 u_wi = fvec2(theta2u(theta_i), phi2u(phi_i));
		fvec2 u_wm = fvec2(
			theta2u(theta_m), 
			phi2u(m_data->isotropic ? (phi_m - phi_i) : phi_m)
		);
		u_wm[1] = u_wm[1] - floor(u_wm[1]);

		fvec2 sample;
		float vndf_pdf, params[2] = { phi_i, theta_i };
		std::tie(sample, vndf_pdf) = m_data->vndf.invert(u_wm, params);

		brdf::value_type fr = zero_value(); 
		for (int i = 0; i < (int)m_data->wavelengths.size(); ++i) {
            float params_fr[3] = { phi_i, theta_i, m_data->wavelengths[i] };

            fr[i] = m_data->spectra.eval(sample, params_fr);
        }

		if (m_data->jacobian)
            fr *= m_data->ndf.eval(u_wm, params) /
                    (4 * m_data->sigma.eval(u_wi, params));

		return fr;
	}

	return zero_value();
}

// -----------------------------------------------------------------------------
// sample interface
brdf::value_type
brdf::sample(
	const vec2 &u,
	const vec3 &wi,
	vec3 *wo_out,
	float *pdf_out
) const {
	float sx = -1.f, sy = -1.f;

	if (m_data->reduction >= 2) {
		sy = wi.y;
		sx = (m_data->reduction == 4) ? wi.x : sy;
#if 0 // TODO: port
		wi[0] = mulsign_neg(wi[0], sx);
		wi[1] = mulsign_neg(wi[1], sy);
#endif
	}

	float theta_i = acos(wi.z),
		  phi_i   = atan2(wi.y, wi.x);

	float params[2] = { phi_i, theta_i };
	fvec2 u_wi = fvec2(theta2u(theta_i), phi2u(phi_i));
	fvec2 sample = fvec2(u.y, u.x);
	float lum_pdf = 1.f;

	#if 1
		std::tie(sample, lum_pdf) =
			m_data->luminance.sample(sample, params);
	#endif

	fvec2 u_wm;
	float ndf_pdf;
	std::tie(u_wm, ndf_pdf) =
		m_data->vndf.sample(sample, params);

	float phi_m   = u2phi(u_wm[1]),
		  theta_m = u2theta(u_wm[0]);

	if (m_data->isotropic)
		phi_m += phi_i;

	/* Spherical -> Cartesian coordinates */
	float sin_phi_m = sin(phi_m), 
	      cos_phi_m = cos(phi_m), 
		  sin_theta_m = sin(theta_m), 
		  cos_theta_m = cos(theta_m);

	vec3 wm = vec3(
		cos_phi_m * sin_theta_m,
		sin_phi_m * sin_theta_m,
		cos_theta_m
	);

	float jacobian = max(2.f * sqr(math::pi) * u_wm[0] *
								sin_theta_m, 1e-6f) * 4.f * dot(wi, wm);

	vec3 wo = wm * 2.f * dot(wm, wi) - wi;
	float pdf = ndf_pdf * lum_pdf / jacobian;

	brdf::value_type fr = eval(wi, wo);
	if (wo_out)  (*wo_out)  = wo;
	if (pdf_out) (*pdf_out) = pdf;

#if 0 // TODO: port
	bs.wo[0] = mulsign_neg(bs.wo[0], sx);
	bs.wo[1] = mulsign_neg(bs.wo[1], sy);
#endif
	return fr / pdf;
}



} // namespace powitacq

//
//
//// end implementation /////////////////////////////////////////////////////

#endif // DJ_BRDF_IMPLEMENTATION
