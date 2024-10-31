#version 420 core

// Uniform model view projection matrix
uniform mat4 MVP;

// Uniform height map values
layout(binding = 0) uniform sampler2D heightMapSampler;
uniform float heightMapScale;

// Uniform nPoints
uniform float nPoints;

// Direction vectors
const vec2 up = vec2(0.0, -1.0);
const vec2 left = vec2(-1.0, 0.0);
const vec2 down = vec2(0.0, 1.0);
const vec2 right = vec2(0.0, 1.0);

const vec2 leftUp = left + up;
const vec2 leftDown = left + down;
const vec2 rightUp = right + up;
const vec2 rightDown = right + down;

// Weights matrix - Used for normal approximation
const mat3 weights = mat3(
	1.0, 5.0, 1.0,
	5.0, 0.25, 5.0,
	1.0, 5.0, 1.0
);

// Input vertex data, different for all executions of this shader
layout(location = 0) in vec3 vertexPosition_ocs;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexTangent;
layout(location = 3) in vec3 vertexBitangent;

// Output data - will be interpolated for each fragment
out vec2 UV;
out vec3 T;
out vec3 B;
out vec3 N;
out vec3 position_ocs;
out float height;

float heightAt(vec2 uv) {
	// Decode height from rgb height map
	vec3 rgbHeight = texture(heightMapSampler, uv).rgb;
	uint r = uint(rgbHeight.r * 255.0);
	uint g = uint(rgbHeight.g * 255.0);
	uint b = uint(rgbHeight.b * 255.0);

	return float((r << 16) + (g << 8) + b) * heightMapScale;
}

float neighbourHeightIn(vec2 direction) {
	return heightAt(vertexUV + (direction / nPoints));
}

vec3 sampleNormal() {
	float nx =
		(weights[0][0] * neighbourHeightIn(leftUp) - weights[0][2] * neighbourHeightIn(rightUp)) +
		(weights[1][0] * neighbourHeightIn(left) - weights[1][2] * neighbourHeightIn(right)) +
		(weights[2][0] * neighbourHeightIn(leftDown) - weights[2][2] * neighbourHeightIn(rightDown));

	float ny = weights[1][1];

	float nz =
		(weights[0][0] * neighbourHeightIn(leftUp) - weights[2][0] * neighbourHeightIn(leftDown)) +
		(weights[0][1] * neighbourHeightIn(up) - weights[2][1] * neighbourHeightIn(down)) +
		(weights[0][2] * neighbourHeightIn(rightUp) - weights[2][2] * neighbourHeightIn(rightDown));

	return normalize(abs(vec3(nx, ny, nz)));
}

void main() {
	// Add height to vertexPosition, in object space
	height = heightAt(vertexUV);
	position_ocs = vertexPosition_ocs + vec3(0.0, height, 0.0);

	// Output position of the vertex, in clip space: MVP * position
	gl_Position = MVP * vec4(position_ocs, 1.0);

	// Output vertex UV
	UV = vertexUV;

	// Compute TBN vectors
	T = normalize(vertexTangent);
	B = normalize(vertexBitangent);
	N = sampleNormal();
	// Gram-Schmidt to orthogonalise T & B with respect to N
	T = normalize(T - dot(T, N) * N);
	B = normalize(B - dot(B, N) * N);
	B = normalize(B - dot(B, T) * T);
}