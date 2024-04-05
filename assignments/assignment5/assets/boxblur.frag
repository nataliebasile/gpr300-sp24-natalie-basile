#version 450

in vec2 UV;

out vec4 FragColor;

uniform sampler2D _ColorBuffer;
uniform int _BlurAmount;

void main() {
	// Calculates box size based on blur amount
	float edgeSize = _BlurAmount * 2 + 1;
	
	vec2 texelSize = 1.0 / textureSize(_ColorBuffer, 0).xy;
	vec3 totalColor = vec3(0);
	for (int y = -_BlurAmount; y <= _BlurAmount; y++) {
		for (int x = -_BlurAmount; x <= _BlurAmount; x++) {
			vec2 offset = vec2(x, y) * texelSize;
			totalColor += texture(_ColorBuffer, UV + offset).rgb;
		}
	}

	totalColor /= (edgeSize * edgeSize);
	FragColor = vec4(totalColor, 1.0);
}