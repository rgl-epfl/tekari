#pragma once

#include "common.h"
#include "stb_image.h"

TEKARI_NAMESPACE_BEGIN

class ColorMap
{
public:
    static const std::string FOLDER_PATH;
    static const std::vector<std::pair<const std::string, const std::string>> PREDEFINED_MAPS;

public:
    ColorMap(const std::string& name, const std::string& filePath);

    void bind(unsigned int target = 0);
    void unbind(unsigned int target = 0);

    unsigned int id() const { return mRenderId; }
    const std::string& name() const { return mName; }

private:
    unsigned int mRenderId;
    std::string mName;
};

TEKARI_NAMESPACE_END