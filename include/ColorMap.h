#pragma once

#include <nanogui\glutil.h>
#include "stb_image.h"

class ColorMap
{
public:
	ColorMap(const std::string& filePath)
	{
		glGenTextures(1, &m_RenderId);
		glBindTexture(GL_TEXTURE_1D, m_RenderId);

		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		int w, h, numChanels;
		unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &numChanels, 0);
		if (!data)
		{
			throw std::runtime_error("Unable to open color map " + filePath);
		}
		if (h != 1)
		{
			throw std::runtime_error("Wrong color map format " + filePath + " (height should be 1)");
		}
		GLenum format = GL_RGB;
		if (numChanels == 3) format = GL_RGB;
		if (numChanels == 4) format = GL_RGBA;
		glTexImage1D(GL_TEXTURE_1D, 0, format, w, 0, format, GL_UNSIGNED_BYTE, data);
		glBindTexture(GL_TEXTURE_1D, 0);
		stbi_image_free(data);
	}

	void bind(unsigned int target = 0)
	{
		glActiveTexture(GL_TEXTURE0 + target);
		glBindTexture(GL_TEXTURE_1D, m_RenderId);
	}
	void unbind(unsigned int target = 0)
	{
		glActiveTexture(GL_TEXTURE0 + target);
		glBindTexture(GL_TEXTURE_1D, 0);
	}

private:
	unsigned int m_RenderId;
};