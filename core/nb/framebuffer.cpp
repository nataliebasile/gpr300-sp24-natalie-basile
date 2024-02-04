#include "framebuffer.h"

namespace nb {
	Framebuffer createFramebuffer(unsigned int width, unsigned int height, int colorFormat) {

		// Create framebuffer object to return
		Framebuffer fb;
		fb.width = width;
		fb.height = height;

		// Create framebuffer object
		glCreateFramebuffers(1, &fb.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);

		// Create and bind color buffer texture
		glGenTextures(1, &fb.colorBuffer[0]);
		glBindTexture(GL_TEXTURE_2D, fb.colorBuffer[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height);

		// Create and bind depth buffer texture
		glGenTextures(1, &fb.depthBuffer);
		glBindTexture(GL_TEXTURE_2D, fb.depthBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);

		// Attach color and depth bfufers/textures to FBO
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.colorBuffer[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb.depthBuffer, 0);

		return fb;
	}
}
