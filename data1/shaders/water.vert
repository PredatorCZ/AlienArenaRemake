uniform vec3 LightPos;
uniform float time;
uniform int REFLECT;
uniform int FOG;

const float Eta = 0.66;
const float FresnelPower = 2.5;
const float F = ((1.0 - Eta) * (1.0 - Eta)) / ((1.0 + Eta) * (1.0 + Eta));

varying float FresRatio;
varying vec3 lightDir;
varying vec3 eyeDir;
varying float fog;

void main(void) {
    gl_Position = ftransform();

    vec4 viewVertex = gl_ModelViewMatrix * gl_Vertex;
    vec3 binormal = tangent.w * cross(gl_Normal, tangent.xyz);
    mat3 tangentSpaceTransform = transpose(mat3(tangent.xyz, binormal, gl_Normal));

    lightDir = tangentSpaceTransform * ((gl_ModelViewMatrix * vec4(LightPos, 1.0)).xyz - viewVertex.xyz);

    vec4 neyeDir = gl_ModelViewMatrix * gl_Vertex;
    vec3 refeyeDir = neyeDir.xyz / neyeDir.w;
    refeyeDir = normalize(refeyeDir);

		// The normal is always 0, 0, 1 because water is always up. Thus, 
		// dot (refeyeDir,norm) is always refeyeDir.z
    FresRatio = F + (1.0 - F) * pow((1.0 - refeyeDir.z), FresnelPower);

    eyeDir = tangentSpaceTransform * (-viewVertex.xyz);

    vec4 texco = gl_MultiTexCoord0;
    if(REFLECT > 0) {
        texco.s = texco.s - LightPos.x / 256.0;
        texco.t = texco.t + LightPos.y / 256.0;
    }
    gl_TexCoord[0] = texco;

    texco = gl_MultiTexCoord0;
    texco.s = texco.s + time * 0.05;
    texco.t = texco.t + time * 0.05;
    gl_TexCoord[1] = texco;

    texco = gl_MultiTexCoord0;
    texco.s = texco.s - time * 0.05;
    texco.t = texco.t - time * 0.05;
    gl_TexCoord[2] = texco;

    if(FOG > 0) {
        fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
        fog = clamp(fog, 0.0, 1.0);
    }
}