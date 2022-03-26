uniform sampler2D surfTexture;
uniform sampler2D HeightTexture;
uniform sampler2D NormalTexture;
uniform sampler2D lmTexture;
uniform sampler2D chromeTex;
uniform int FOG;
uniform int PARALLAX;
uniform int SHINY;

uniform sampler2D liquidTexture;
uniform sampler2D liquidNormTex;
uniform int LIQUID;
uniform float rsTime;

varying float FresRatio;
varying vec4 sPos;
varying vec3 StaticLightDir;
varying float fog;

// results of liquid_effects function
vec4 bloodColor;
vec2 liquidDisplacement;

// this function will be inlined by the GLSL compiler.
void liquid_effects(vec3 relativeEyeDirection, vec4 Offset, vec2 BaseTexCoords) {
    bloodColor = vec4(0.0);
    liquidDisplacement = vec2(0.0);
    if(LIQUID > 0) {
		//for liquid fx scrolling
        vec2 texco = BaseTexCoords;
        texco.t -= rsTime * 1.0 / LIQUID;

        vec2 texco2 = BaseTexCoords;
        texco2.t -= rsTime * 0.9 / LIQUID;
		//shift the horizontal here a bit
        texco2.s /= 1.5;

        vec2 TexCoords = Offset.xy * relativeEyeDirection.xy + BaseTexCoords;

        vec2 noiseVec = normalize(texture2D(liquidNormTex, texco)).xy;
        noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

        vec2 noiseVec2 = normalize(texture2D(liquidNormTex, texco2)).xy;
        noiseVec2 = (noiseVec2 * 2.0 - 0.635) * 0.035;

        if(LIQUID > 2) {
            vec2 texco3 = BaseTexCoords;
            texco3.t -= rsTime * 0.6 / LIQUID;

            vec2 noiseVec3 = normalize(texture2D(liquidNormTex, texco3)).xy;
            noiseVec3 = (noiseVec3 * 2.0 - 0.635) * 0.035;

            vec4 diffuse1 = texture2D(liquidTexture, 2.0 * texco + noiseVec + TexCoords);
            vec4 diffuse2 = texture2D(liquidTexture, 2.0 * texco2 + noiseVec2 + TexCoords);
            vec4 diffuse3 = texture2D(liquidTexture, 2.0 * texco3 + noiseVec3 + TexCoords);
            bloodColor = max(diffuse1, diffuse2);
            bloodColor = max(bloodColor, diffuse3);
        } else {
			// used for water effect only
            liquidDisplacement = noiseVec + noiseVec2;
        }
    }
}

void main(void) {
    vec4 diffuse;
    vec4 lightmap;
    vec4 alphamask;
    float distanceSquared;
    vec3 halfAngleVector;
    float specularTerm;
    float swamp;
    float attenuation;
    vec4 litColour;
    float statshadowval;

    vec3 relativeEyeDirection = normalize(EyeDir);
    vec3 relativeLightDirection = normalize(StaticLightDir);

    vec4 normal = texture2D(NormalTexture, gl_TexCoord[0].xy);
    normal.xyz = 2.0 * (normal.xyz - vec3(0.5));
    vec3 textureColour = texture2D(surfTexture, gl_TexCoord[0].xy).rgb;

    lightmap = texture2D(lmTexture, gl_TexCoord[1].st);
    alphamask = texture2D(surfTexture, gl_TexCoord[0].xy);

	//shadows
    statshadowval = lookup_sunstatic(sPos, 0.2);

    if(PARALLAX > 0) {
        vec4 Offset = texture2D(HeightTexture, gl_TexCoord[0].xy);
        Offset = Offset * 0.04 - 0.02;

		// Liquid effects only get applied if parallax mapping is on for the
		// surface.
        liquid_effects(relativeEyeDirection, Offset, gl_TexCoord[0].st);

		//do the parallax mapping
        vec2 TexCoords = Offset.xy * relativeEyeDirection.xy + gl_TexCoord[0].xy + liquidDisplacement.xy;

        diffuse = texture2D(surfTexture, TexCoords);

        float diffuseTerm = dot(normal.xyz, relativeLightDirection);

        if(diffuseTerm > 0.0) {
            halfAngleVector = normalize(relativeLightDirection + relativeEyeDirection);

            specularTerm = clamp(dot(normal.xyz, halfAngleVector), 0.0, 1.0);
            specularTerm = pow(specularTerm, 32.0);

            litColour = vec4(specularTerm * normal.a + (3.0 * diffuseTerm) * textureColour, 6.0);
        } else {
            litColour = vec4(0.0, 0.0, 0.0, 6.0);
        }

        gl_FragColor = max(litColour, diffuse * 2.0);
        gl_FragColor = (gl_FragColor * lightmap) + bloodColor;
        gl_FragColor = (gl_FragColor * statshadowval);

		// Normalmapping for static lighting. We want any light
		// attenuation to come from the normalmap, not from the normal of
		// the polygon (since the lightmap compiler already accounts for
		// that.) So we calculate how much light we'd lose from the normal
		// of the polygon and give that much back.
        float face_normal_coef = relativeLightDirection[2];
        float normal_coef = diffuseTerm + (1.0 - face_normal_coef);
        gl_FragColor.rgb *= normal_coef;
    } else {
        diffuse = texture2D(surfTexture, gl_TexCoord[0].xy);
        gl_FragColor = (diffuse * lightmap * 2.0);
        gl_FragColor = (gl_FragColor * statshadowval);
    }

    if(DYNAMIC > 0) {
        lightmap = texture2D(lmTexture, gl_TexCoord[1].st);

        float dynshadowval = lookup_dynamic(sPos, 0.2);
        vec3 dynamicColor = computeDynamicLightingFrag(textureColour, normal.xyz, normal.a, dynshadowval);
        gl_FragColor.rgb += dynamicColor;
    }

    gl_FragColor = mix(vec4(0.0, 0.0, 0.0, alphamask.a), gl_FragColor, alphamask.a);

    if(SHINY > 0) {
        vec3 reflection = reflect(relativeEyeDirection, normal.xyz);
        vec3 refraction = refract(relativeEyeDirection, normal.xyz, 0.66);

        vec4 Tl = texture2DProj(chromeTex, vec4(reflection.xy, 1.0, 1.0));
        vec4 Tr = texture2DProj(chromeTex, vec4(refraction.xy, 1.0, 1.0));

        vec4 cubemap = mix(Tl, Tr, FresRatio);

        gl_FragColor = max(gl_FragColor, (cubemap * 0.05 * alphamask.a));
    }

    if(FOG > 0)
        gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
}