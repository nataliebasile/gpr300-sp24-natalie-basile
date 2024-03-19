#include "framebuffer.h"
#include <iostream>

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
		glGenTextures(1, &fb.colorBuffers[0]);
		glBindTexture(GL_TEXTURE_2D, fb.colorBuffers[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height);

		// Create and bind depth buffer texture
		glGenTextures(1, &fb.depthBuffer);
		glBindTexture(GL_TEXTURE_2D, fb.depthBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);

		// Attach color and depth bfufers/textures to FBO
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fb.colorBuffers[0], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb.depthBuffer, 0);

		return fb;
	}

	Framebuffer createGBuffer(unsigned int width, unsigned int height) {
		Framebuffer gb;
		gb.width = width;
		gb.height = height;

		glCreateFramebuffers(1, &gb.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, gb.fbo);

		int formats[3] = {
			GL_RGB32F, // 0 = world position
			GL_RGB16F, // 1 = world normal
			GL_RGB16F  // 2 = albedo
		};

		// Create 3 color textures
		for (size_t i = 0; i < 3; i++) {
			glGenTextures(1, &gb.colorBuffers[i]);
			glBindTexture(GL_TEXTURE_2D, gb.colorBuffers[i]);
			glTexStorage2D(GL_TEXTURE_2D, 1, formats[i], width, height);

			// Clamp to border so we don't wrap when sampling for post processing
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// Attach each texture to a different slot
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, gb.colorBuffers[i], 0);
		}

		// Explicitly tell OpenGL which color attachments we will draw to
		const GLenum drawBuffers[3] = {
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
		};

		glDrawBuffers(3, drawBuffers);

		// Create, bind, and attach depth buffer texture
		glGenTextures(1, &gb.depthBuffer);
		glBindTexture(GL_TEXTURE_2D, gb.depthBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gb.depthBuffer, 0);

		GLenum gboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (gboStatus != GL_FRAMEBUFFER_COMPLETE) {
			printf("\nFramebuffer incomplete %d\n", gboStatus);
		}

		// Clean up global state :)
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return gb;
	}

}
