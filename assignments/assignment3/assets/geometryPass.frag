#version 450 core

layout(location = 0) out vec3 gPosition; // Worldspace position
layout(location = 1) out vec3 gNormal; // Worldspace normal
layout(location = 2) out vec3 gAlbedo;

in Surface {
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
	mat3 TBN;
}fs_in;

uniform sampler2D _MainTex;
uniform sampler2D _NormalTex;

vec3 normal;


void main() {
	gPosition = fs_in.WorldPos;

	normal = texture(_NormalTex, fs_in.TexCoord).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(fs_in.TBN * normal);

	gNormal = normalize(fs_in.WorldNormal);
	gAlbedo = texture(_MainTex, fs_in.TexCoord).rgb;
}