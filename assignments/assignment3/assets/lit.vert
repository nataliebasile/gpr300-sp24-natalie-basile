#version 450

// Vertex Attributes
layout(location = 0) in vec3 vPos; // Vertex position in model space
layout(location = 1) in vec3 vNormal; // Vertex position in model space
layout(location = 2) in vec3 vTangent; // Tangent
layout(location = 3) in vec2 vTexCoord; // Vertex texture coordinate (UV)

uniform mat4 _Model; // Model -> World Matrix
uniform mat4 _ViewProjection; // Combined View -> Projection Matrix
uniform mat4 _LightViewProjection; // View + Projection of light source camera

out vec4 LightSpacePos;
out Surface {
	vec3 WorldPos; // Vertex position in world space
	vec3 WorldNormal; // Vertex normal in world space
	vec2 TexCoord;
	mat3 TBN; // TBN matrix
}vs_out;


void main() {
	// Transform vertex position to World Space
	vs_out.WorldPos = vec3(_Model * vec4(vPos, 1.0));

	// Vertex position in light space to pass to fragment shader
	LightSpacePos = _LightViewProjection * _Model * vec4(vPos, 1);

	// Transform vertex normal to World Space using Normal Matrix
	vs_out.WorldNormal = transpose(inverse(mat3(_Model))) * vNormal;

	// TBN Matrix
	vec3 T = normalize(vec3(_Model * vec4(vTangent, 0.0)));
	vec3 B = normalize(vec3(_Model * vec4(cross(vNormal, T), 0.0)));
	vec3 N = normalize(vec3(_Model * vec4(vNormal, 0.0)));
	vs_out.TBN = mat3(T, B, N);

	vs_out.TexCoord = vTexCoord;
	
	// Transform vertex position to homogeneous clip space
	gl_Position = _ViewProjection * _Model * vec4(vPos, 1.0);
}