#version 450

in vec4 LightSpacePos;
in Surface {
	vec3 WorldPos; // Vertex position in world space
	vec3 WorldNormal; // Vertex normal in world space
	vec2 TexCoord;
	mat3 TBN; // TBN matrix
}fs_in;

out vec4 FragColor; // The color of this fragment

uniform float _MinBias;
uniform float _MaxBias;


uniform sampler2D _MainTex; // 2D texture sampler
uniform sampler2D _NormalTex;
uniform sampler2D _ShadowMap;

uniform vec3 _EyePos;
uniform vec3 _LightDirection; // Light pointing straight down
uniform vec3 _LightColor; // White light
uniform vec3 _AmbientColor = vec3(0.3, 0.4, 0.46);

struct Material {
	float Ka; // Ambient coefficient (0-1)
	float Kd; // Diffuse coefficient (0-1)
	float Ks; // Specular coefficient (0-1)
	float Shininess; // Affects size of specular highlight
};
uniform Material _Material;

float calcShadow(sampler2D shadowMap, vec4 lightSpacePos);
vec3 normal;
vec3 toLight;

void main() {
	// Make sure fragment normal is still length 1 after interpolation
	normal = texture(_NormalTex, fs_in.TexCoord).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(fs_in.TBN * normal);
	vec3 lightDir = normalize(_LightDirection);

	// Light pointing straight down
	toLight = -lightDir;
	float diffuseFactor = 0.5 * max(dot(normal, toLight), 0.0);

	// Direction towards eye
	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);

	// Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h), 0.0), _Material.Shininess);

	// Combination of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;
	
	// 1: in shadow, 0: out of shadow
	float shadow = calcShadow(_ShadowMap, LightSpacePos);
	lightColor *= (1.0 - shadow);

	// Add some ambient light
	lightColor += _AmbientColor * _Material.Ka;

	vec3 objectColor = texture(_MainTex, fs_in.TexCoord).rgb;
	FragColor = vec4(objectColor * lightColor, 1.0);
}

float calcShadow(sampler2D shadowMap, vec4 lightSpacePos) {
	// Homogeneous Clip space to NDC [-w, w] to [-1, 1]
	vec3 sampleCoord = lightSpacePos.xyz / lightSpacePos.w;

	// Convert from [-1, 1] to [0, 1]
	sampleCoord = sampleCoord * 0.5 + 0.5;

	float bias = max(_MaxBias * (1.0 - dot(normal, toLight)), _MinBias);
	float myDepth = sampleCoord.z - bias;

	// Percentage Closer Filtering
	float totalShadow = 0;
	vec2 texelOffset = 1.0 / textureSize(_ShadowMap, 0);
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			vec2 uv = sampleCoord.xy + vec2(x * texelOffset.x, y * texelOffset.y);
			totalShadow += step(texture(_ShadowMap, uv).r, myDepth);
		}
	}
	return totalShadow /= 9.0;
}