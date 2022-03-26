// common declarations, must be used the same way in every script if they're
// used at all
attribute vec4 tangent;

#if DYNAMIC >= 1
uniform vec3 lightPosition[DYNAMIC];
varying vec3 LightVec[DYNAMIC];
#endif

varying vec3 EyeDir;

vec4 viewVertex;
mat3 tangentSpaceTransform;

// arguments given in model space
void computeDynamicLightingVert(vec4 vertex, vec3 normal, vec4 tangent) {
	vec3 bitangent = tangent.w * cross(normal, tangent.xyz);
	tangentSpaceTransform = transpose(mat3(gl_NormalMatrix * tangent.xyz, gl_NormalMatrix * bitangent, gl_NormalMatrix * normal));

	viewVertex = gl_ModelViewMatrix * vertex;

	EyeDir = tangentSpaceTransform * (-viewVertex.xyz);


#if DYNAMIC >= 1
	LightVec[0] = tangentSpaceTransform * (lightPosition[0] - viewVertex.xyz);
#endif
#if DYNAMIC >= 2
	LightVec[1] = tangentSpaceTransform * (lightPosition[1] - viewVertex.xyz);
#endif
#if DYNAMIC >= 3
	LightVec[2] = tangentSpaceTransform * (lightPosition[2] - viewVertex.xyz);
#endif
#if DYNAMIC >= 4
	LightVec[3] = tangentSpaceTransform * (lightPosition[3] - viewVertex.xyz);
#endif
#if DYNAMIC >= 5
	LightVec[4] = tangentSpaceTransform * (lightPosition[4] - viewVertex.xyz);
#endif
#if DYNAMIC >= 6
	LightVec[5] = tangentSpaceTransform * (lightPosition[5] - viewVertex.xyz);
#endif
#if DYNAMIC >= 7
	LightVec[6] = tangentSpaceTransform * (lightPosition[6] - viewVertex.xyz);
#endif
#if DYNAMIC >= 8
	LightVec[7] = tangentSpaceTransform * (lightPosition[7] - viewVertex.xyz);
#endif
}

// Mesh Animation
uniform mat3x4 bonemats[70]; // Keep this equal to SKELETAL_MAX_BONEMATS
uniform int GPUANIM; // 0 for none, 1 for IQM skeletal, 2 for MD2 lerp

// MD2 only
uniform float lerp; // 1.0 = all new vertex, 0.0 = all old vertex
// IQM only
attribute vec4 weights;
attribute vec4 bones;

// MD2 only
attribute vec3 oldvertex;
attribute vec3 oldnormal;
attribute vec4 oldtangent;

// anim_compute () output
vec4 anim_vertex;
vec3 anim_normal;
vec4 anim_tangent;

// if dotangent is true, compute anim_tangent and anim_tangent_w
// if donormal is true, compute anim_normal
// hopefully the if statements for these booleans will get optimized out
void anim_compute(bool dotangent, bool donormal) {
	if(GPUANIM == 1) {
		mat3x4 m = bonemats[int(bones.x)] * weights.x;
		m += bonemats[int(bones.y)] * weights.y;
		m += bonemats[int(bones.z)] * weights.z;
		m += bonemats[int(bones.w)] * weights.w;

		anim_vertex = vec4(gl_Vertex * m, gl_Vertex.w);
		if(donormal)
			anim_normal = vec4(gl_Normal, 0.0) * m;
		if(dotangent)
			anim_tangent = vec4(vec4(tangent.xyz, 0.0) * m, tangent.w);
	} else if(GPUANIM == 2) {
		anim_vertex = mix(vec4(oldvertex, 1), gl_Vertex, lerp);
		if(donormal)
			anim_normal = normalize(mix(oldnormal, gl_Normal, lerp));
		if(dotangent)
			anim_tangent = mix(oldtangent, tangent, lerp);
	} else {
		anim_vertex = gl_Vertex;
		if(donormal)
			anim_normal = gl_Normal;
		if(dotangent)
			anim_tangent = tangent;
	}
}
