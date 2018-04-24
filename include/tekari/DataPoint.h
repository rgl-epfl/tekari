#pragma once

#include "common.h"
#include <cmath>
#include <nanogui/glutil.h>
#include "delaunay.h"
//
//class RawDataPoint
//{
//public:
//    RawDataPoint(float theta, float phi, float intensity)
//    :   m_Values{theta, phi, intensity}
//    {}
//    RawDataPoint(nanogui::Vector3f _values)
//    :   m_Values{ _values }
//    {}
//
//    operator nanogui::Vector3f&() { return m_Values; }
//    operator nanogui::Vector3f() const { return m_Values; }
//
//    float theta() const { return m_Values[0]; }
//    float phi() const { return m_Values[1]; }
//    float intensity() const { return m_Values[2]; }
//
//    del_point2d_t transform() const
//    {
//        return del_point2d_t{   (float)(theta() * cos(phi() * M_PI / 180.0f) / 90.0f),
//                                (float)(theta() * sin(phi() * M_PI / 180.0f) / 90.0f) };
//    }
//
//    RawDataPoint operator+(const RawDataPoint& other) const
//    {
//        return m_Values + other.m_Values;
//    }
//    RawDataPoint& operator+=(const RawDataPoint& other)
//    {
//        m_Values += other.m_Values;
//        return *this;
//    }
//    RawDataPoint operator/(const RawDataPoint& other) const
//    {
//        return m_Values + other.m_Values;
//    }
//    RawDataPoint& operator+=(const RawDataPoint& other)
//    {
//        m_Values += other.m_Values;
//        return *this;
//    }
//
//private:
//    nanogui::Vector3f m_Values;
//};
