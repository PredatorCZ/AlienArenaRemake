uniform vec3 staticLightPosition;
uniform int envmap;
uniform int numblendtextures;
uniform int FOG;
// 0 means no lightmap, 1 means lightmap using the main texcoords, and 2
// means lightmap using its own set of texcoords.
uniform int lightmap;
uniform vec3 meshPosition;
uniform mat3 meshRotation;

varying float fog;
varying vec3 orig_normal;
varying vec3 orig_coord;
varying vec3 StaticLightDir;
varying vec4 sPos;

void main() {
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_FrontColor = gl_BackColor = gl_Color;

	sPos = vec4((meshRotation * gl_Vertex.xyz) + meshPosition, 1.0);

	vec4 maincoord;

	if(envmap == 1) {
		maincoord.st = normalize(gl_Position.xyz).xy;
		maincoord.pq = vec2(0.0, 1.0);
	} else {
		maincoord = gl_MultiTexCoord0;
	}

	// XXX: tri-planar projection requires the vertex normal, so don't use
	// the blendmap RScript command on BSP surfaces yet!
	if(numblendtextures != 0) {
		orig_normal = gl_Normal.xyz;
		orig_coord = (gl_TextureMatrix[0] * gl_Vertex).xyz;
		gl_TexCoord[0] = maincoord;
	} else {
		gl_TexCoord[0] = gl_TextureMatrix[0] * maincoord;
	}

	if(lightmap == 1)
		gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord0;
	else if(lightmap == 2)
		gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;

	if(FOG > 0) {
		fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
		fog = clamp(fog, 0.0, 1.0);
	}

	computeDynamicLightingVert(gl_Vertex, gl_Normal, tangent);

	StaticLightDir = tangentSpaceTransform * ((gl_ModelViewMatrix * vec4(staticLightPosition, 1.0)).xyz - viewVertex.xyz);
}