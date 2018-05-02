#pragma once

#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include "common.h"
#include "MetadataElement.h"

TEKARI_NAMESPACE_BEGIN

class Metadata
{
public:

    void parse(const std::string& line);
    
    template<typename T>
    void addElement(const std::string& name, const T& value);
    
    template<typename T>
    T getElementValue(const std::string& name) const;

    unsigned int elementsCount() const { return m_Elements.size(); }
    const std::shared_ptr<const BaseMetadataElement> getElement(unsigned int index) const
    {
        return m_Elements[index];
    }

private:
    static std::string stripQuoteMarks(const std::string& word) { return word.substr(1, word.size() - 2);; }

    std::vector<std::shared_ptr<BaseMetadataElement>> m_Elements;
};

TEKARI_NAMESPACE_END