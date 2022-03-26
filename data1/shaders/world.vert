uniform vec3 staticLightPosition;
uniform float rsTime;
uniform int LIQUID;
uniform int FOG;
uniform int PARALLAX;
uniform int SHINY;

const float Eta = 0.66;
const float FresnelPower = 5.0;
const float F = ((1.0 - Eta) * (1.0 - Eta)) / ((1.0 + Eta) * (1.0 + Eta));

varying float FresRatio;
varying vec3 LightDir;
varying vec3 StaticLightDir;
varying vec4 sPos;
varying float fog;

void main(void) {
	sPos = gl_Vertex;

	gl_Position = ftransform();

	computeDynamicLightingVert(gl_Vertex, gl_Normal, tangent);

	if(SHINY > 0) {
		vec3 norm = vec3(0, 0, 1);

		vec3 refeyeDir = viewVertex.xyz / viewVertex.w;
		refeyeDir = normalize(refeyeDir);

		FresRatio = F + (1.0 - F) * pow((1.0 - dot(refeyeDir, norm)), FresnelPower);
	}

	gl_FrontColor = gl_Color;

	if(PARALLAX > 0) {
		StaticLightDir = tangentSpaceTransform * ((gl_ModelViewMatrix * vec4(staticLightPosition, 1.0)).xyz - viewVertex.xyz);
	}

	// pass any active texunits through
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_MultiTexCoord1;

	if(FOG > 0) {
		fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
		fog = clamp(fog, 0.0, 1.0);
	}
}