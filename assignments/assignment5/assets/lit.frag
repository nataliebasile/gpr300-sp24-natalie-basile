#version 450

in Surface {
	vec3 WorldPos; // Vertex position in world space
	vec3 WorldNormal; // Vertex normal in world space
	vec2 TexCoord;
	mat3 TBN; // TBN matrix
}fs_in;

out vec4 FragColor; // The color of this fragment

uniform sampler2D _MainTex; // 2D texture sampler
uniform sampler2D _NormalTex;

uniform vec3 _EyePos;
uniform vec3 _LightDirection = vec3(0.0, -1.0, 0.0); // Light pointing straight down
uniform vec3 _LightColor = vec3(1.0); // White light
uniform vec3 _AmbientColor = vec3(0.3, 0.4, 0.46);

struct Material {
	float Ka; // Ambient coefficient (0-1)
	float Kd; // Diffuse coefficient (0-1)
	float Ks; // Specular coefficient (0-1)
	float Shininess; // Affects size of specular highlight
};
uniform Material _Material;

void main() {
	// Make sure fragment normal is still length 1 after interpolation
	vec3 normal = texture(_NormalTex, fs_in.TexCoord).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(fs_in.TBN * normal);


	// Light pointing straight down
	vec3 toLight = -_LightDirection;
	float diffuseFactor = 0.5 * max(dot(normal, toLight), 0.0);

	// Direction towards eye
	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);

	// Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h), 0.0), _Material.Shininess);

	// Combination of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;

	// Add some ambient light
	lightColor += _AmbientColor * _Material.Ka;

	vec3 objectColor = texture(_MainTex, fs_in.TexCoord).rgb;
	FragColor = vec4(objectColor * lightColor, 1.0);
}