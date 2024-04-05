#version 450

in vec2 UV;

out vec4 FragColor;

uniform sampler2D _ColorBuffer;

void main() {
	FragColor = vec4(texture(_ColorBuffer, UV).rgb, 1.0);

}