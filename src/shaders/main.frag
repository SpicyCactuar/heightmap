#version 420 core

// Input
in vec2 UV;
in vec3 T;
in vec3 B;
in vec3 N;
in vec3 position_ocs;
in float height;

// Output
out vec3 color;

// Uniform samplers
layout(binding = 1) uniform sampler2D grassSampler;
layout(binding = 2) uniform sampler2D grassRoughnessSampler;
layout(binding = 3) uniform sampler2D grassNormalSampler;
layout(binding = 4) uniform sampler2D rocksSampler;
layout(binding = 5) uniform sampler2D rocksRoughnessSampler;
layout(binding = 6) uniform sampler2D rocksNormalSampler;
layout(binding = 7) uniform sampler2D snowSampler;
layout(binding = 8) uniform sampler2D snowRoughnessSampler;
layout(binding = 9) uniform sampler2D snowNormalSampler;

// Uniform model & view matrices
uniform mat4 M;
uniform mat4 V;

// Uniform light properties
uniform vec3 lightDirection_wcs;

// Uniform normal mode - Output normals as colour
uniform bool normalMode;

// Camera in view space - Always at (0, 0, 0)
const vec3 camera_vcs = vec3(0.0, 0.0, 0.0);

// Tiling constants
const float tiles = 10.0;

// Lighting constants
const vec3 specularIntensity = vec3(0.01, 0.01, 0.01);
const vec3 ambientIntensity = vec3(0.2, 0.2, 0.2);
const vec3 lightColour = vec3(1.0, 1.0, 1.0);

// Terrain interpolation constants
const float grassHeight = 0.0;
const float rocksHeight = grassHeight + 0.5;
const float snowHeight = rocksHeight + 2.0;

vec4 heightInterpolation(sampler2D grassTexture, sampler2D rocksTexture, sampler2D snowTexture) {
	vec2 tiledUV = tiles * UV;
	float grassRocksFactor = clamp((height - rocksHeight) * 4.0, 0.0, 1.0);
	float rocksSnowFactor = clamp((height - snowHeight) * 4.0, 0.0, 1.0);

	vec4 grassRockMix = mix(texture(grassTexture, tiledUV), texture(rocksTexture, tiledUV), grassRocksFactor);
	return mix(grassRockMix, texture(snowTexture, tiledUV), rocksSnowFactor);
}

vec3 interpolateTerrain() {
	return heightInterpolation(grassSampler, rocksSampler, snowSampler).rgb;
}

float interpolateRoughness() {
	return heightInterpolation(grassRoughnessSampler, rocksRoughnessSampler, snowRoughnessSampler).r;
}

vec3 interpolateNormal() {
	vec3 normal = heightInterpolation(grassNormalSampler, rocksNormalSampler, snowNormalSampler).rgb;
	return normalize(2.0 * normal - 1.0);
}

mat3 computeTBN() {
	vec3 T_vcs = (V * M * vec4(T, 0.0)).xyz;
	vec3 B_vcs = (V * M * vec4(B, 0.0)).xyz;
	vec3 N_vcs = (V * M * vec4(N, 0.0)).xyz;

	return mat3(T_vcs, B_vcs, N_vcs);
}

vec3 blinnPhongLighting() {
	// Ambient
	vec3 ambient = lightColour * ambientIntensity;

	// Normal of the computed fragment, in camera space
	vec3 n = normalize(computeTBN() * interpolateNormal());

	// Direction of the light (from the fragment to the light)
	// Negate light to make the vector point out of the fragment
	vec3 vl = normalize(V * M * vec4(-lightDirection_wcs, 0.0)).xyz;

	// No direct lighting if point is not facing light
	if (dot(n, vl) < 0.0) {
		return ambient;
	}

	// Diffuse
	float cosThetaDiffuse = max(dot(n, vl), 0.0);
	vec3 diffuse = lightColour * cosThetaDiffuse;

	// Specular
	// Eye vector (towards the camera)
	// Vector that goes from the vertex to the camera, in camera space
	vec3 position_vcs = (V * M * vec4(position_ocs, 1.0)).xyz;
	vec3 eyeDirection_vcs = camera_vcs - position_vcs;
	vec3 ve = normalize(V * M * vec4(eyeDirection_vcs, 0.0)).xyz;
	// Direction in which the triangle reflects the light
	vec3 vb = normalize(vl + ve);

	// roughness is a grayscale texture => r = g = b
	// We can retrieve any component, choose first one
	float roughness = interpolateRoughness().r;
	float shininess = clamp((2.0 / (pow(roughness, 4) + 1e-2)) - 2.0, 0.0, 500.0);
	float cosThetaSpecular = max(dot(n, vb), 0.0);
	vec3 specular = lightColour * specularIntensity * pow(cosThetaSpecular, shininess);

	return ambient + diffuse + specular;
}

void main() {
	if (normalMode) {
		color = abs(computeTBN() * interpolateNormal());
	} else {
		color = blinnPhongLighting() * interpolateTerrain();
	}
}