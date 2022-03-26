//#! vertexOnly
uniform vec3 staticLightColor;
uniform vec3 staticLightPosition;
uniform vec3 totalLightPosition;
uniform vec3 meshPosition;
uniform mat3 meshRotation;
uniform float time;
uniform int FOG;
uniform int TEAM;
uniform float useShell; // doubles as shell scale
uniform int fromView;
uniform int useCube;
// For now, only applies to vertexOnly. If 0, don't do the per-vertex shading.
uniform int doShading;
// 0 means no lightmap, 1 means lightmap using the main texcoords, and 2
// means lightmap using its own set of texcoords.
uniform int lightmap;

const float Eta = 0.66;
const float FresnelPower = 5.0;
const float F = ((1.0 - Eta) * (1.0 - Eta)) / ((1.0 + Eta) * (1.0 + Eta));

varying vec4 sPos;
varying vec3 StaticLightDir;
varying float fog;
varying float FresRatio;
varying vec3 SSlightVec, SSEyeDir, worldNormal;

void subScatterVS(in vec4 ecVert) {
    if(useShell == 0 && useCube == 0) {
        SSEyeDir = vec3(gl_ModelViewMatrix * anim_vertex);
        SSlightVec = totalLightPosition - ecVert.xyz;
    }
}

void main() {
    fog = 0.0;

    anim_compute(true, true);

    if(useShell > 0)
        anim_vertex += normalize(vec4(anim_normal, 0)) * useShell;

    gl_Position = gl_ModelViewProjectionMatrix * anim_vertex;
    subScatterVS(gl_Position);

    sPos = vec4((meshRotation * anim_vertex.xyz) + meshPosition, 1.0);

    worldNormal = normalize(gl_NormalMatrix * anim_normal);

    computeDynamicLightingVert(anim_vertex, anim_normal, anim_tangent);
    StaticLightDir = tangentSpaceTransform * staticLightPosition;

    vec4 neyeDir = gl_ModelViewMatrix * anim_vertex;

    if(useShell > 0) {
        gl_TexCoord[0] = vec4((anim_vertex[1] + anim_vertex[0]) / 40.0, anim_vertex[2] / 40.0 - time, 0.0, 1.0);
    } else {
        gl_TexCoord[0] = gl_MultiTexCoord0;
			//for scrolling fx
        vec4 texco = gl_MultiTexCoord0;
        texco.s = texco.s + time * 1.0;
        texco.t = texco.t + time * 2.0;
        gl_TexCoord[2] = texco;
    }

    if(lightmap == 1)
        gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord0;
    else if(lightmap == 2)
        gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;

	// vertexOnly is defined as const, so this branch should get optimized out.
    if(vertexOnly == 1) {
		// We try to bias toward light instead of shadow, but then make
		// the contrast between light and shadow more pronounced.
        float lightness;
        if(doShading == 1) {
            lightness = max(normalize(StaticLightDir).z, 0.0) * 3.0 + 0.25;
            lightness += lightness * lightness * lightness;
        } else {
            lightness = 1.0;
        }
        gl_FrontColor = gl_BackColor = vec4(staticLightColor * lightness, 1.0);
        if(FOG == 1) // TODO: team colors with normalmaps disabled!
            gl_FogFragCoord = length(gl_Position);
    } else {
        if(useCube == 1) {
            vec3 refeyeDir = neyeDir.xyz / neyeDir.w;
            refeyeDir = normalize(refeyeDir);

            FresRatio = F + (1.0 - F) * pow((1.0 - dot(refeyeDir, worldNormal)), FresnelPower);
        }

        if(TEAM > 0) {
            fog = (gl_Position.z - 100.0) / 1000.0;
            if(TEAM == 3)
                fog = clamp(fog, 0.0, 0.5);
            else
                fog = clamp(fog, 0.0, 0.75);
        } else if(FOG == 1) {
            fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
            fog = clamp(fog, 0.0, 0.3); //any higher and meshes disappear
        }
    }
}