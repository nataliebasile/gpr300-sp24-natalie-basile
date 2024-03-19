#version 450 core

out vec4 FragColor; // The color of this fragment

in vec2 UV;

uniform float _MinBias;
uniform float _MaxBias;

uniform layout(binding = 0) sampler2D _gPositions;
uniform layout(binding = 1) sampler2D _gNormals;
uniform layout(binding = 2) sampler2D _gAlbedo;
uniform layout(binding = 3) sampler2D _ShadowMap;

struct DirLight {
	vec3 dir; // _LightDirection
	vec3 color; // _LightColor
};
uniform DirLight _MainLight;

struct PointLight {
	vec3 position;
	float radius;
	vec4 color;
};
#define MAX_POINT_LIGHTS 64
uniform PointLight _PointLights[MAX_POINT_LIGHTS];

uniform mat4 _LightViewProjection;

uniform vec3 _EyePos;
//uniform vec3 _LightDirection; // Light pointing straight down
//uniform vec3 _LightColor; // White light
uniform vec3 _AmbientColor = vec3(0.3, 0.4, 0.46);

struct Material {
	float Ka; // Ambient coefficient (0-1)
	float Kd; // Diffuse coefficient (0-1)
	float Ks; // Specular coefficient (0-1)
	float Shininess; // Affects size of specular highlight
};
uniform Material _Material;

float calcShadow(sampler2D shadowMap, vec4 lightSpacePos);
vec3 calcDirectionalLight( DirLight _MainLight, vec3 normal, vec3 pos );
vec3 calcPointLight(PointLight light, vec3 normal, vec3 pos);
float attenuateLinear(float d, float radius);
float attenuateExponential(float d, float radius);

vec3 normal, toLight;
float diffuseFactor, specularFactor;

void main() {
	// Sample surface properties from gBuffers
	normal = texture(_gNormals, UV).xyz;
	vec3 worldPos = texture(_gPositions, UV).xyz;
	vec3 albedo = texture(_gAlbedo, UV).xyz;

	vec3 totalLight = vec3(0);

	totalLight += calcDirectionalLight(_MainLight, normal, worldPos);

	// Get color for each light
	for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
		totalLight += calcPointLight(_PointLights[i], normal, worldPos);
	}

	FragColor = vec4(albedo * totalLight, 1.0);

}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 pos) {
	// Direction toward light position
	vec3 diff = light.position - pos;
	toLight = normalize(diff);

	diffuseFactor = 0.5 * max(dot(normal, toLight), 0.0);

	// Direction towards eye
	vec3 toEye = normalize(_EyePos - pos);

	// Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	specularFactor = pow(max(dot(normal,h), 0.0), _Material.Shininess);

	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * light.color.rgb;

	float d = length(diff);
	lightColor *= attenuateLinear(d, light.radius);

	return lightColor;
}

float attenuateLinear(float d, float radius) {
	return clamp((radius - d)/radius, 0.0, 1.0);
}

float attenuateExponential(float d, float radius) {
	float i = clamp(1.0 - pow(d/radius, 4.0), 0.0, 1.0);
	return i * i;
}

vec3 calcDirectionalLight( DirLight _MainLight, vec3 normal, vec3 pos ) {
	// Calculate lightSpacePos and lightDir
	vec4 lightSpacePos = _LightViewProjection * vec4(pos, 1);
	vec3 lightDir = normalize(_MainLight.dir);

	// Light pointing straight down
	toLight = -lightDir;
	diffuseFactor = 0.5 * max(dot(normal, toLight), 0.0);

	// Direction towards eye
	vec3 toEye = normalize(_EyePos - pos);

	// Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	specularFactor = pow(max(dot(normal,h), 0.0), _Material.Shininess);

	// Combination of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _MainLight.color;
	
	// 1: in shadow, 0: out of shadow
	float shadow = calcShadow(_ShadowMap, lightSpacePos);
	lightColor *= (1.0 - shadow);

	// Add some ambient light
	lightColor += _AmbientColor * _Material.Ka;

	return lightColor;
	//vec3 objectColor = texture(_MainTex, fs_in.TexCoord).rgb;
	//vec3 objectColor = texture(albedo, UV).xyz;
	//FragColor = vec4(objectColor * lightColor, 1.0);
	//FragColor = vec4(albedo * lightColor, 1.0);
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