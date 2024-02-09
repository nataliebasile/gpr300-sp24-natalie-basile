#pragma once

#include <glm/glm.hpp>

namespace nb {
	struct Light {
		glm::vec3 direction = { 0.0, -1.0, -1.0 }; // default light pointing straight down
		glm::vec3 color = glm::vec3(1.0); // default white color
	};

	Light createLight(glm::vec3 direction, glm::vec3 color);
	Light changeDirection(glm::vec3 direction);
	Light changeColor(glm::vec3 color);
}