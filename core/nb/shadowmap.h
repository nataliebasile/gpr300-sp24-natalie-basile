#pragma once

#include "../ew/external/glad.h"

namespace nb {
	struct ShadowMap {
		unsigned int sfbo = 0;
		unsigned int depthTexture = 0; //actual shadowmap texture
		unsigned int width, height;
	};

	ShadowMap createShadowMap(unsigned int width, unsigned int height);

}