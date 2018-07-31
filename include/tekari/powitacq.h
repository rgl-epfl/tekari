/* powitacq.h: Self-contained evaluation and sampling code for

     An Adaptive Parameterization for Efficient Material
     Acquisition and Rendering

   by Jonathan Dupuy and Wenzel Jakob

   This file only contains the interface. To also compile
   the implementation, specify

      #define POWITACQ_IMPLEMENTATION 1

   before including this header file. The namespace "powitacq"
   refers to the internal name of the project ("acquisition
   using power iterations).

   This proof-of-concept implementation involves some
   inefficiencies that should be removed in a
   "production" setting.

   1. It returns the full captured spectrum using a
      std::valarray, which causes dynamic
      memory allocation at every BRDF evaluation.

      In practice, the rendering system may only want
      to evaluate a fixed subset of the wavelengths,
      which could furthermore be stored on the stack.

   2. The implementation doesn't rely on vectorization
      to accelerate simultaneous evaluation at multiple
      wavelengths.

*/

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <array>
#include <valarray>

#define POWITACQ_NAMESPACE_BEGIN  namespace powitacq {
#define POWITACQ_NAMESPACE_END    }
#define POWITACQ_DIM(V)           template <size_t D = Dim, std::enable_if_t<(D >= V), int> = 0>

POWITACQ_NAMESPACE_BEGIN

// *****************************************************************************
// Generic vector class

template <typename Type, size_t Dim> struct Vector {
    Vector() = default;

    /// Constant initialization
    Vector(Type v) {
        for (size_t i = 0; i < Dim; ++i)
            values[i] = v;
    }

    /// Initialization with individual components
    template <typename... Args, std::enable_if_t<(sizeof...(Args) >= 2), int> = 0>
    Vector(Args... args) : values { (Type) args... } { }

    /// Converting copy-constructor
    template <typename Type2> Vector(const Vector<Type2, Dim> &v) {
        for (size_t i = 0; i < Dim; ++i)
            values[i] = (Type) v[i];
    }

    /// Element-wise access
    Type &operator[](size_t i) { return values[i]; }
    const Type &operator[](size_t i) const { return values[i]; }

    POWITACQ_DIM(1)       Type &x()       { return values[0]; }
    POWITACQ_DIM(1) const Type &x() const { return values[0]; }

    POWITACQ_DIM(2)       Type &y()       { return values[1]; }
    POWITACQ_DIM(2) const Type &y() const { return values[1]; }

    POWITACQ_DIM(3)       Type &z()       { return values[2]; }
    POWITACQ_DIM(3) const Type &z() const { return values[2]; }

    Type values[Dim];
};

using Vector2f = Vector<float, 2>;
using Vector3f = Vector<float, 3>;

// *****************************************************************************
// BRDF API

/// Data type used to represent spectra
using Spectrum = std::valarray<float>;

class BRDF {
    struct Data;
    std::unique_ptr<Data> m_data;
public:
    // ctor / dtor
    BRDF(const std::string &path_to_file);
    ~BRDF();

    /// get the wavelengths sample points
    const Spectrum &wavelengths() const;

    /// evaluate f_r * cos
    Spectrum eval(const Vector3f &wi, const Vector3f &wo) const;

    /// importance sample f_r * cos using two uniform variates
    Spectrum sample(const Vector2f &u,
                    const Vector3f &wi,
                    Vector3f *wo = nullptr,
                    float *pdf = nullptr) const;

    /// importance sample f_r * cos using two uniform variates
    float sample(const Vector2f &u,
                    const Vector3f &wi,
                    size_t wave_length_index,
                    Vector3f *wo = nullptr,
                    float *pdf = nullptr) const;

    /// evaluate the PDF of a sample
    float pdf(const Vector3f &wi, const Vector3f &wo) const;

private:
    Spectrum zero() const;
};

POWITACQ_NAMESPACE_END

#ifdef POWITACQ_IMPLEMENTATION
#  include "powitacq.inl"
#endif
