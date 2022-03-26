// Functions and other declarations for the GLSL standard fragment library.
// Prepended to every GLSL fragment shader. We rely on the GLSL compiler to
// throw away anything that a given shader doesn't use.

#if DYNAMIC >= 1
uniform vec3 lightAmount[DYNAMIC];
uniform float lightCutoffSquared[DYNAMIC];
varying vec3 LightVec[DYNAMIC];
#endif

varying vec3 EyeDir;

	// increase this number to tone down dynamic lighting more
const float attenuation_boost = 1.5;

	// normal argument given in tangent space
void ComputeForLight(const vec3 normal, const float specular, const vec3 nEyeDir, const vec3 textureColor3, const vec3 LightVec, const vec3 lightAmount, const float lightCutoffSquared, const float shadowCoef, inout vec3 ret) {
    float distanceSquared = dot(LightVec, LightVec);
    if(distanceSquared < lightCutoffSquared) {
			// If we get this far, the fragment is within range of the 
			// dynamic light
        vec3 attenuation = clamp(vec3(1.0) - attenuation_boost * vec3(distanceSquared) / lightAmount, vec3(0.0), vec3(1.0));
        vec3 relativeLightDirection = LightVec / sqrt(distanceSquared);
        float diffuseTerm = max(0.0, dot(relativeLightDirection, normal));
        vec3 specularAdd = vec3(0.0, 0.0, 0.0);

        if(diffuseTerm > 0.0) {
            vec3 halfAngleVector = normalize(relativeLightDirection + nEyeDir);

            float specularTerm = clamp(dot(normal, halfAngleVector), 0.0, 1.0);
            specularTerm = pow(specularTerm, 32.0);

            specularAdd = vec3(specular * specularTerm / 2.0);
        }
        vec3 swamp = attenuation * attenuation;
        swamp *= swamp;
        swamp *= swamp;
        swamp *= swamp;

        ret += shadowCoef * (((vec3(0.5) - swamp) * diffuseTerm + swamp) * textureColor3 + specularAdd) * attenuation;
    }
}
vec3 computeDynamicLightingFrag(const vec3 textureColor, const vec3 normal, const float specular, const float shadowCoef) {
    vec3 ret = vec3(0.0);
    vec3 nEyeDir = normalize(EyeDir);
    vec3 textureColor3 = textureColor * 3.0;

#if DYNAMIC >= 1
    ComputeForLight(normal, specular, nEyeDir, textureColor3, LightVec[0], lightAmount[0], lightCutoffSquared[0], shadowCoef, ret);
#endif
#if DYNAMIC >= 2
    ComputeForLight(normal, specular, nEyeDir, textureColor3, LightVec[1], lightAmount[1], lightCutoffSquared[1], 1.0, ret);
#endif
#if DYNAMIC >= 3
    ComputeForLight(normal, specular, nEyeDir, textureColor3, LightVec[2], lightAmount[2], lightCutoffSquared[2], 1.0, ret);
#endif
#if DYNAMIC >= 4
    ComputeForLight(normal, specular, nEyeDir, textureColor3, LightVec[3], lightAmount[3], lightCutoffSquared[3], 1.0, ret);
#endif
#if DYNAMIC >= 5
    ComputeForLight(normal, specular, nEyeDir, textureColor3, LightVec[4], lightAmount[4], lightCutoffSquared[4], 1.0, ret);
#endif
#if DYNAMIC >= 6
    ComputeForLight(normal, specular, nEyeDir, textureColor3, LightVec[5], lightAmount[5], lightCutoffSquared[5], 1.0, ret);
#endif
#if DYNAMIC >= 7
    ComputeForLight(normal, specular, nEyeDir, textureColor3, LightVec[6], lightAmount[6], lightCutoffSquared[6], 1.0, ret);
#endif
#if DYNAMIC >= 8
    ComputeForLight(normal, specular, nEyeDir, textureColor3, LightVec[7], lightAmount[7], lightCutoffSquared[7], 1.0, ret);
#endif
    return ret;
}

// Shadow Mapping
uniform int sunstatic_enabled, otherstatic_enabled;
uniform shadowsampler_t sunstatic_texture, otherstatic_texture, dynamic_texture;
uniform float sunstatic_pixelOffset, otherstatic_pixelOffset, dynamic_pixelOffset;

#ifndef AMD_GPU
#define fudge _fudge
float lookup(vec2 offSet, sampler2DShadow Map, vec4 ShadowCoord) {
    return shadow2DProj(Map, ShadowCoord + vec4(offSet * ShadowCoord.w, 0.05, 0.0)).w;
}
#else
#define fudge 0.0
float lookup(vec2 offSet, sampler2D Map, vec4 ShadowCoord) {
    vec4 shadowCoordinateWdivide = (ShadowCoord + vec4(offSet * ShadowCoord.w, 0.0, 0.0)) / ShadowCoord.w;
		// Used to lower moir pattern and self-shadowing
    shadowCoordinateWdivide.z += 0.0005;

    float distanceFromLight = texture2D(Map, shadowCoordinateWdivide.xy).z;

    if(ShadowCoord.w > 0.0)
        return distanceFromLight < shadowCoordinateWdivide.z ? 0.25 : 1.0;
    return 1.0;
}
#endif

float lookupShadow(shadowsampler_t Map, vec4 ShadowCoord, float offset, float _fudge) {
    float shadow = 1.0;

    if(ShadowCoord.w > 1.0) {
        vec2 o = mod(floor(gl_FragCoord.xy), 2.0);

        shadow += lookup((vec2(-1.5, 1.5) + o) * offset, Map, ShadowCoord);
        shadow += lookup((vec2(0.5, 1.5) + o) * offset, Map, ShadowCoord);
        shadow += lookup((vec2(-1.5, -0.5) + o) * offset, Map, ShadowCoord);
        shadow += lookup((vec2(0.5, -0.5) + o) * offset, Map, ShadowCoord);
        shadow *= 0.25;
    }
    shadow += fudge;
    if(shadow > 1.0)
        shadow = 1.0;

    return shadow;
}
float lookup_sunstatic(vec4 sPos, float _fudge) {
    if(sunstatic_enabled == 0)
        return 1.0;
    vec4 coord = gl_TextureMatrix[5] * sPos;
    return lookupShadow(sunstatic_texture, coord, sunstatic_pixelOffset, fudge);
}
float lookup_otherstatic_always(vec4 sPos, float _fudge) {
    vec4 coord = gl_TextureMatrix[6] * sPos;
    if(coord.w <= 1.0)
        return 1.0;
    return lookupShadow(otherstatic_texture, coord, otherstatic_pixelOffset, fudge);
}
float lookup_otherstatic(vec4 sPos, float _fudge) {
    if(otherstatic_enabled == 0)
        return 1.0;
    return lookup_otherstatic_always(sPos, fudge);
}
float lookup_dynamic(vec4 sPos, float _fudge) {
    vec4 coord = gl_TextureMatrix[7] * sPos;
    if(coord.w <= 1.0)
        return 1.0;
    return lookupShadow(dynamic_texture, coord, dynamic_pixelOffset, fudge);
}