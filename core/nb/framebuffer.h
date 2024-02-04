#pragma once

#include "../ew/external/glad.h"

namespace nb {
	struct Framebuffer {
		unsigned int fbo = 0;
		unsigned int colorBuffer[8];
		unsigned int depthBuffer = 0;
		unsigned int width, height;
	};

	Framebuffer createFramebuffer(unsigned int width, unsigned int height, int colorFormat);
}