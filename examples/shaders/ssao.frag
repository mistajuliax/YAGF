#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// From paper http://graphics.cs.williams.edu/papers/AlchemyHPG11/
// and improvements here http://graphics.cs.williams.edu/papers/SAOHPG12/

layout(set = 0, binding = 0, std140) uniform Matrixes
{
	mat4 ProjectionMatrix;
	float radius;
	float tau;
	float beta;
	float epsilon;
};
layout(set = 0, binding = 2) uniform texture2D dtex;
layout(set = 1, binding = 3) uniform sampler s;

vec3 getXcYcZc(float x, float y, float zC)
{
	// We use perspective symetric projection matrix hence P(0,2) = P(1, 2) = 0
	float xC= (2 * x - 1.) * zC / ProjectionMatrix[0][0];
	float yC= (2 * y - 1.) * zC / ProjectionMatrix[1][1];
	return vec3(xC, yC, zC);
}

layout(location = 0) out vec4 FragColor;

void main(void)
{
#define SAMPLES 16
	float invSamples = 1. / SAMPLES;
	vec2 screen = vec2(1024, 1024);
    float sigma = 1.;
    float k = 1.;

	vec2 uv = gl_FragCoord.xy / screen;
	float lineardepth = texture(sampler2D(dtex, s), uv).x;

	vec3 FragPos = getXcYcZc(uv.x, uv.y, lineardepth);
	// get the normal of current fragment
	vec3 ddx = dFdx(FragPos);
	vec3 ddy = dFdy(FragPos);
	vec3 norm = normalize(cross(ddy, ddx));
	float r = radius / FragPos.z;

	int x = int(gl_FragCoord.xy.x), y = int(gl_FragCoord.xy .y);
	float phi = 3. * (x ^ y) + x * y;
	float bl = 0.0;
	float m = log2(r) + 6 + log2(invSamples);
	float theta = 2. * 3.14 * tau * .5 * invSamples + phi;
	vec2 rotations = vec2(cos(theta), sin(theta)) * screen;
	vec2 offset = vec2(cos(invSamples), sin(invSamples));
	for(int i = 0; i < SAMPLES; ++i) {
		float alpha = (i + .5) * invSamples;
		rotations = vec2(rotations.x * offset.x - rotations.y * offset.y, rotations.x * offset.y + rotations.y * offset.x);
		float h = r * alpha;
		vec2 localoffset = h * rotations;
		m = m + .5;
		ivec2 ioccluder_uv = ivec2(x, y) + ivec2(localoffset);
		if (ioccluder_uv.x < 0 || ioccluder_uv.x > screen.x || ioccluder_uv.y < 0 || ioccluder_uv.y > screen.y) continue;
		float LinearoccluderFragmentDepth = texture(sampler2D(dtex, s), vec2(ioccluder_uv) / screen).x;//, max(m, 0.)).x;
		vec3 OccluderPos = getXcYcZc(ioccluder_uv.x, ioccluder_uv.y, LinearoccluderFragmentDepth);
		vec3 vi = OccluderPos - FragPos;
		bl += max(0.f, dot(vi, norm) - FragPos.z * beta) / (dot(vi, vi) + epsilon);
	}
	FragColor = vec4(max(pow(1.f - min(2. * sigma * bl * invSamples, 0.99), k), 0.));
}