#include <tekari/color_map.h>
#include <tekari_resources.h>
#include <stb_image.h>

TEKARI_NAMESPACE_BEGIN

const vector<pair<const string, pair<const uint8_t*, uint32_t>>> ColorMap::PREDEFINED_MAPS =
{
    { "Jet",            { jet_png, jet_png_size } },
    { "BRG",            { brg_png, brg_png_size } },
    { "CMR Map",        { cmrmap_png, cmrmap_png_size } },
    { "Cube Helix",     { cubehelix_png, cubehelix_png_size } },
    { "Gist Earth",     { gist_earth_png, gist_earth_png_size } },
    { "Gist Ncar",      { gist_ncar_png, gist_ncar_png_size } },
    { "Gist Rainbow",   { gist_rainbow_png, gist_rainbow_png_size } },
    { "Gist Stern",     { gist_stern_png, gist_stern_png_size } },
    { "GNU Plot",       { gnu_plot_png, gnu_plot_png_size } },
    { "GNU Plot 2",     { gnu_plot2_png, gnu_plot2_png_size } },
    { "HSV",            { hsv_png, hsv_png_size } },
    { "Inferno",        { inferno_png, inferno_png_size } },
    { "Numpy Spectral", { npy_spectral_png, npy_spectral_png_size } },
    { "Ocean",          { ocean_png, ocean_png_size } },
    { "Prism",          { prism_png, prism_png_size } },
    { "Rainbow",        { rainbow_png, rainbow_png_size } },
    { "Terrain",        { terrain_png, terrain_png_size } },
};

ColorMap::ColorMap(const string& name, const uint8_t* color_map_str, uint32_t color_map_str_size)
:   m_name(name)
{
    glGenTextures(1, &m_render_id);
    glBindTexture(GL_TEXTURE_2D, m_render_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, num_chanels;
    unsigned char* data = stbi_load_from_memory(color_map_str, color_map_str_size, &w, &h, &num_chanels, 0);
    if (!data)
    {
        throw std::runtime_error("Unable to open color map " + name);
    }
    GLenum format = GL_RGB;
    if (num_chanels == 3) format = GL_RGB;
    if (num_chanels == 4) format = GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
}

void ColorMap::bind(unsigned int target)
{
    glActiveTexture(GL_TEXTURE0 + target);
    glBindTexture(GL_TEXTURE_2D, m_render_id);
}
void ColorMap::unbind(unsigned int target)
{
    glActiveTexture(GL_TEXTURE0 + target);
    glBindTexture(GL_TEXTURE_2D, 0);
}

TEKARI_NAMESPACE_END