#version 450

in vec3 Normal; // Interpolated of this fragment

out vec4 FragColor; // The color of this fragment

void main() {
	// Shade with 0-1 normal
	FragColor = vec4(Normal * 0.5 + 0.5, 1.0);
}