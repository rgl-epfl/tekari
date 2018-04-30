#pragma once

#include <string>
#include <iostream>
#include "common.h"

TEKARI_NAMESPACE_BEGIN

class BaseMetadataElement
{
public:
    BaseMetadataElement(const std::string& name)
        : m_Name(name)
    {}
    virtual std::string toString() const = 0;

    const std::string& name() const { return m_Name; }
private:
    std::string m_Name;
};

template<typename T>
class MetadataElement : public BaseMetadataElement
{
public:
    MetadataElement(const std::string& name, T value)
        : BaseMetadataElement(name), m_Value(value)
    {}
    std::string toString() const override { return std::to_string(m_Value); }
    T getValue() const { return m_Value; }

private:
    T m_Value;
};

template<>
class MetadataElement<std::string> : public BaseMetadataElement
{
public:
    MetadataElement(const std::string& name, const std::string& value)
        : BaseMetadataElement(name), m_Value(value)
    {}
    std::string toString() const override { return m_Value; }
    std::string getValue() const { return m_Value; }

private:
    std::string m_Value;
};

TEKARI_NAMESPACE_END