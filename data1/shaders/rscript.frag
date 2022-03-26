uniform sampler2D mainTexture;
uniform sampler2D mainTexture2;
uniform sampler2D lightmapTexture;
uniform sampler2D blendTexture0;
uniform sampler2D blendTexture1;
uniform sampler2D blendTexture2;
uniform sampler2D blendTexture3;
uniform sampler2D blendTexture4;
uniform sampler2D blendTexture5;
uniform sampler2D blendNormalmap0;
uniform sampler2D blendNormalmap1;
uniform sampler2D blendNormalmap2;
uniform int numblendtextures, numblendnormalmaps;
uniform int static_normalmaps;
uniform int FOG;
uniform vec2 blendscales[6];
uniform int normalblendindices[3];
// 0 means no lightmap, 1 means lightmap using the main texcoords, and 2
// means lightmap using its own set of texcoords.
uniform int lightmap;

varying float fog;
varying vec3 orig_normal;
varying vec3 orig_coord;
varying vec3 StaticLightDir;
varying vec4 sPos;

// This is tri-planar projection, based on code from NVIDIA's GPU Gems
// website. Potentially could be replaced with bi-planar projection, for
// roughly 1/3 less sampling overhead but also less accuracy, or
// alternately, for even less overhead and *greater* accuracy, this fancy
// thing: http://graphics.cs.williams.edu/papers/IndirectionI3D08/

vec4 triplanar_sample(sampler2D tex, vec3 blend_weights, vec2 scale) {
    return blend_weights[0] * texture2D(tex, orig_coord.yz * scale) +
        blend_weights[1] * texture2D(tex, orig_coord.zx * scale) +
        blend_weights[2] * texture2D(tex, orig_coord.xy * scale);
}

void main() {

    vec4 mainColor = texture2D(mainTexture, gl_TexCoord[0].st);
    vec4 normal = vec4(0.0, 0.0, 1.0, 0.0);

    if(numblendtextures == 0) {
        gl_FragColor = mainColor;
    } else {
        vec4 mainColor2 = vec4(0.0);
        if(numblendtextures > 3)
            mainColor2 = texture2D(mainTexture2, gl_TexCoord[0].st);

        float totalbrightness = dot(mainColor.rgb, vec3(1.0)) +
            dot(mainColor2.rgb, vec3(1.0));
        mainColor.rgb /= totalbrightness;
        mainColor2.rgb /= totalbrightness;

        vec3 blend_weights = abs(normalize(orig_normal));
        blend_weights = (blend_weights - vec3(0.2)) * 7;
        blend_weights = max(blend_weights, 0);
        blend_weights /= (blend_weights.x + blend_weights.y + blend_weights.z);

		// Sigh, GLSL doesn't allow you to index arrays of samplers with
		// variables.
        gl_FragColor = vec4(0.0);

		// TODO: go back to switch-case as soon as we start using GLSL
		// 1.30. 
        if(mainColor.r > 0.0)
            gl_FragColor += triplanar_sample(blendTexture0, blend_weights, blendscales[0]) * mainColor.r;
        if(numblendtextures > 1) {
            if(mainColor.g > 0.0)
                gl_FragColor += triplanar_sample(blendTexture1, blend_weights, blendscales[1]) * mainColor.g;
            if(numblendtextures > 2) {
                if(mainColor.b > 0.0)
                    gl_FragColor += triplanar_sample(blendTexture2, blend_weights, blendscales[2]) * mainColor.b;
                if(numblendtextures > 3) {
                    if(mainColor2.r > 0.0)
                        gl_FragColor += triplanar_sample(blendTexture3, blend_weights, blendscales[3]) * mainColor2.r;
                    if(numblendtextures > 4) {
                        if(mainColor2.g > 0.0)
                            gl_FragColor += triplanar_sample(blendTexture4, blend_weights, blendscales[4]) * mainColor2.g;
                        if(numblendtextures > 5) {
                            if(mainColor2.b > 0.0)
                                gl_FragColor += triplanar_sample(blendTexture5, blend_weights, blendscales[5]) * mainColor2.b;
                        }
                    }
                }
            }
        }

        if((DYNAMIC > 0 || static_normalmaps != 0) && numblendnormalmaps > 0) {
            float totalnormal = 0.0;

            normal = vec4(0.0);

            float normalcoef = normalblendindices[0] >= 3 ? mainColor2[normalblendindices[0] - 3] : mainColor[normalblendindices[0]];
            if(normalcoef > 0.0) {
                totalnormal += normalcoef;
                normal += triplanar_sample(blendNormalmap0, blend_weights, blendscales[normalblendindices[0]]) * normalcoef;
            }
            if(numblendnormalmaps > 1) {
                normalcoef = normalblendindices[1] >= 3 ? mainColor2[normalblendindices[1] - 3] : mainColor[normalblendindices[1]];
                if(normalcoef > 0.0) {
                    totalnormal += normalcoef;
                    normal += triplanar_sample(blendNormalmap1, blend_weights, blendscales[normalblendindices[1]]) * normalcoef;
                }
                if(numblendnormalmaps > 2) {
                    normalcoef = normalblendindices[2] >= 3 ? mainColor2[normalblendindices[2] - 3] : mainColor[normalblendindices[2]];
                    if(normalcoef > 0.0) {
                        totalnormal += normalcoef;
                        normal += triplanar_sample(blendNormalmap2, blend_weights, blendscales[normalblendindices[2]]) * normalcoef;
                    }
                }
            }

			// We substitute "straight up" as the normal for channels that
			// don't have corresponding normalmaps.
            normal.xyz += vec3(0.5, 0.5, 1.0) * (1.0 - totalnormal);

            normal.xyz = 2.0 * (normal.xyz - vec3(0.5));
        }
    }

    gl_FragColor *= gl_Color;
    vec3 textureColor = gl_FragColor.rgb;

    if(lightmap != 0)
        gl_FragColor *= 2.0 * texture2D(lightmapTexture, gl_TexCoord[1].st);

    if(static_normalmaps != 0 && numblendnormalmaps > 0) {
		// We want any light attenuation to come from the normalmap, not
		// from the normal of the polygon (since the lightmap compiler
		// already accounts for that.) So we calculate how much light we'd
		// lose from the normal of the polygon and give that much back.

        vec3 RelativeLightDirection = normalize(StaticLightDir);
		// note that relativeLightDirection[2] == dot (RelativeLightDirection, up)
        float face_normal_coef = RelativeLightDirection[2];
        float normalmap_normal_coef = dot(normal.xyz, RelativeLightDirection);
        float normal_coef = normalmap_normal_coef + (1.0 - face_normal_coef);
        gl_FragColor.rgb *= normal_coef;

		//add specularity for appropriate areas
        if(normalmap_normal_coef > 0.0) {
            vec3 relativeEyeDirection = normalize(EyeDir);
            vec3 halfAngleVector = normalize(RelativeLightDirection + relativeEyeDirection);

            float specularTerm = clamp(dot(normal.xyz, halfAngleVector), 0.0, 1.0);
            specularTerm = pow(specularTerm, 32.0) * normal.a;

			//Take away some of the effect of areas that are in shadow
            float shadowTerm = clamp(length(texture2D(lightmapTexture, gl_TexCoord[1].st).rgb) / 0.3, 0.15, 0.85);

			//if a pretty dark area, let's try and subtly remove some specularity, without making too sharp of a line artifact
            if(shadowTerm < 0.25) {
                specularTerm *= shadowTerm * 2.0;
            }

            gl_FragColor.rgb = max(gl_FragColor.rgb, vec3(specularTerm + normalmap_normal_coef * textureColor * shadowTerm));
        }
    }

    gl_FragColor.rgb *= lookup_sunstatic(sPos, 0.2);

    if(DYNAMIC > 0) {
        vec3 dynamicColor = computeDynamicLightingFrag(textureColor, normal.xyz, normal.a, 1.0);
        gl_FragColor.rgb += dynamicColor;
    }

    if(FOG > 0)
        gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
}