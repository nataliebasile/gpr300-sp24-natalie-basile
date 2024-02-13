#pragma once

#include <glm/glm.hpp>

namespace nb {
	struct Light {
		glm::vec3 direction = { 0.0, -1.0, -1.0 }; // default light pointing straight down
		glm::vec3 color = glm::vec3(1.0); // default white color
		void changeDirection(glm::vec3 dir) { direction = dir; };
		void changeColor(glm::vec3 col) { color = col; };
	};

	Light createLight(glm::vec3 direction, glm::vec3 color) {
		Light light{ direction, color };
		return light;
	}
}