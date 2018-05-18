#pragma once

#include <string>
#include <iostream>
#include "common.h"

TEKARI_NAMESPACE_BEGIN

class BaseMetadataElement
{
public:
    BaseMetadataElement(const std::string& name)
        : mName(name)
    {}
    virtual std::string toString() const = 0;

    const std::string& name() const { return mName; }
private:
    std::string mName;
};

template<typename T>
class MetadataElement : public BaseMetadataElement
{
public:
    MetadataElement(const std::string& name, T value)
        : BaseMetadataElement(name), mValue(value)
    {}
    std::string toString() const override { return std::to_string(mValue); }
    T getValue() const { return mValue; }

private:
    T mValue;
};

template<>
class MetadataElement<std::string> : public BaseMetadataElement
{
public:
    MetadataElement(const std::string& name, const std::string& value)
        : BaseMetadataElement(name), mValue(value)
    {}
    std::string toString() const override { return mValue; }
    std::string getValue() const { return mValue; }

private:
    std::string mValue;
};

TEKARI_NAMESPACE_END