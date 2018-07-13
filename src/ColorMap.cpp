#include "tekari/ColorMap.h"

#include "stb_image.h"

using namespace std;

TEKARI_NAMESPACE_BEGIN

const string ColorMap::FOLDER_PATH = "../resources/color_maps/";
const vector<pair<const string, const string>> ColorMap::PREDEFINED_MAPS =
{
    { "Jet",            "jet.png" },
    { "BRG",            "brg.png" },
    { "CMR Map",        "CMRmap.png" },
    { "Cube Helix",     "cubehelix.png" },
    { "Gist Earth",     "gist_earth.png" },
    { "Gist Ncar",      "gist_ncar.png" },
    { "Gist Rainbow",   "gist_rainbow.png" },
    { "Gist Stern",     "gist_stern.png" },
    { "GNU Plot",       "gnu_plot.png" },
    { "GNU Plot 2",     "gnu_plot2.png" },
    { "HSV",            "hsv.png" },
    { "Inferno",        "inferno.png" },
    { "Numpy Spectral", "npy_spectral.png" },
    { "Ocean",          "ocean.png" },
    { "Prism",          "prism.png" },
    { "Rainbow",        "rainbow.png" },
    { "Terrain",        "terrain.png" },
};

ColorMap::ColorMap(const string& name, const string& file_path)
:   m_name(name)
{
    glGenTextures(1,& m_render_id);
    glBindTexture(GL_TEXTURE_1D, m_render_id);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, num_chanels;
    unsigned char* data = stbi_load(file_path.c_str(),& w,& h,& num_chanels, 0);
    if (!data)
    {
        throw runtime_error("Unable to open color map " + file_path);
    }
    if (h != 1)
    {
        stbi_image_free(data);
        throw runtime_error("Wrong color map format " + file_path + " (height should be 1)");
    }
    GLenum format = GL_RGB;
    if (num_chanels == 3) format = GL_RGB;
    if (num_chanels == 4) format = GL_RGBA;
    glTexImage1D(GL_TEXTURE_1D, 0, format, w, 0, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_1D, 0);
    stbi_image_free(data);
}

void ColorMap::bind(unsigned int target)
{
    glActiveTexture(GL_TEXTURE0 + target);
    glBindTexture(GL_TEXTURE_1D, m_render_id);
}
void ColorMap::unbind(unsigned int target)
{
    glActiveTexture(GL_TEXTURE0 + target);
    glBindTexture(GL_TEXTURE_1D, 0);
}

TEKARI_NAMESPACE_END