#include "shadowmap.h"

namespace nb {
	ShadowMap createShadowMap(unsigned int width, unsigned int height) {

		ShadowMap sm;
		sm.width = width;
		sm.height = height;

		// Create framebuffer object
		glCreateFramebuffers(1, &sm.sfbo);
		glBindFramebuffer(GL_FRAMEBUFFER, sm.sfbo);

		// Create and bind depth texture aka shadowmap
		glGenTextures(1, &sm.depthTexture);
		glBindTexture(GL_TEXTURE_2D, sm.depthTexture);

		// 16 bit depth values, width and height are resolution of shadow map
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		// Attach depth texture (shadowmap) to fbo
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sm.depthTexture, 0);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		return sm;
	}
}