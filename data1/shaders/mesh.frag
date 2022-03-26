uniform vec3 staticLightColor;
uniform vec3 totalLightPosition, totalLightColor;
uniform sampler2D baseTex;
uniform sampler2D normalTex;
uniform sampler2D fxTex;
uniform sampler2D fx2Tex;
uniform sampler2D lightmapTexture;
// 0 means no lightmap, 1 means lightmap using the main texcoords, and 2
// means lightmap using its own set of texcoords.
uniform int lightmap;
uniform int FOG;
uniform int TEAM;
uniform int useFX;
uniform int useCube;
uniform int useGlow;
uniform float useShell;
uniform float shellAlpha;
uniform int fromView;

const float SpecularFactor = 0.50;
//next group could be made uniforms if we want to control this
const float MaterialThickness = 2.0; //this val seems good for now
const vec3 ExtinctionCoefficient = vec3(0.80, 0.12, 0.20); //controls subsurface value
const float RimScalar = 10.0; //intensity of the rim effect

varying vec4 sPos;
varying vec3 StaticLightDir;
varying float fog;
varying float FresRatio;
varying vec3 SSlightVec, SSEyeDir, worldNormal;

float halfLambert(in vec3 vect1, in vec3 vect2) {
    float product = dot(vect1, vect2);
    return product * 0.5 + 0.5;
}

float blinnPhongSpecular(in vec3 normalVec, in vec3 lightVec, in float specPower) {
    vec3 halfAngle = normalize(normalVec + lightVec);
    return pow(clamp(0.0, 1.0, dot(normalVec, halfAngle)), specPower);
}

void main() {
    vec3 litColor;
    vec4 fx;
    vec4 glow;
    vec4 scatterCol = vec4(0.0);
    float shadowval = 1.0;

    vec3 textureColour = texture2D(baseTex, gl_TexCoord[0].xy).rgb * 1.1;
    vec3 normal = 2.0 * (texture2D(normalTex, gl_TexCoord[0].xy).xyz - vec3(0.5));

    vec4 alphamask = texture2D(baseTex, gl_TexCoord[0].xy);
    vec4 specmask = texture2D(normalTex, gl_TexCoord[0].xy);

    if(useShell == 0)
        shadowval = lookup_otherstatic(sPos, 0.2) * lookup_sunstatic(sPos, 0.2);

    if(useShell == 0 && useCube == 0 && specmask.a < 1.0 && lightmap == 0) {
        vec4 SpecColor = vec4(totalLightColor, 1.0) / 2.0;

		//overall brightness should be more of a factor than distance(for example, hold your hand up to block the sun)
        float attenuation = length(clamp(totalLightColor, 0, .05)) * 0.1;
        vec3 wNorm = worldNormal;
        vec3 eVec = normalize(SSEyeDir);
        vec3 lVec = normalize(SSlightVec);

        vec4 dotLN = vec4(halfLambert(lVec, wNorm) * attenuation);

        vec3 indirectLightComponent = vec3(MaterialThickness * max(0.0, dot(-wNorm, lVec)));
        indirectLightComponent += MaterialThickness * halfLambert(-eVec, lVec);
        indirectLightComponent *= attenuation;
        indirectLightComponent.rgb *= ExtinctionCoefficient.rgb;

        vec3 rim = vec3(1.0 - max(0.0, dot(wNorm, eVec)));
        rim *= rim;
        rim *= max(0.0, dot(wNorm, lVec)) * SpecColor.rgb;

        scatterCol = dotLN + vec4(indirectLightComponent, 1.0);
        scatterCol.rgb += (rim * RimScalar * attenuation * scatterCol.a);
        scatterCol.rgb += vec3(blinnPhongSpecular(wNorm, lVec, SpecularFactor * 2.0) * attenuation * SpecColor * scatterCol.a * 0.05);
        scatterCol.rgb *= totalLightColor;
        scatterCol.rgb /= (specmask.a * specmask.a);//we use the spec mask for scatter mask, presuming non-spec areas are always soft/skin
    }

    vec3 relativeLightDirection = normalize(StaticLightDir);

    float diffuseTerm = dot(normal, relativeLightDirection);
    if(diffuseTerm > 0.0) {
        vec3 relativeEyeDirection = normalize(EyeDir);
        vec3 halfAngleVector = normalize(relativeLightDirection + relativeEyeDirection);

        float specularTerm = clamp(dot(normal, halfAngleVector), 0.0, 1.0);
        specularTerm = pow(specularTerm, 32.0);

        litColor = vec3(specularTerm);
        if(lightmap == 0)
            litColor += (3.0 * diffuseTerm) * textureColour;
    } else {
        litColor = vec3(0.0);
    }

    if(lightmap != 0) {
        gl_FragColor.rgb = litColor + 2.0 * texture2D(lightmapTexture, gl_TexCoord[1].st).rgb * textureColour;
        gl_FragColor.a = 1.0;
    } else if(useShell == 0) {
        litColor = litColor * shadowval * staticLightColor;
        gl_FragColor.rgb = max(litColor, textureColour * 0.15);
        gl_FragColor.a = 1.0;
    } else {
        gl_FragColor.rgb = max(litColor, textureColour * 0.25) * staticLightColor;
        gl_FragColor.a = shellAlpha;
    }

    vec3 dynamicColor = computeDynamicLightingFrag(textureColour, normal, specmask.a, 1.0);
    gl_FragColor.rgb += dynamicColor;

	//moving fx texture
    if(useFX > 0)
        fx = texture2D(fxTex, gl_TexCoord[2].xy);
    else
        fx = vec4(0.0, 0.0, 0.0, 0.0);

    gl_FragColor = mix(fx, gl_FragColor + scatterCol, alphamask.a);

    if(useCube > 0 && specmask.a < 1.0) {
        vec3 relEyeDir;

        if(fromView > 0)
            relEyeDir = normalize(StaticLightDir);
        else
            relEyeDir = normalize(EyeDir);

        vec3 reflection = reflect(relEyeDir, normal);
        vec3 refraction = refract(relEyeDir, normal, 0.66);

        vec4 Tl = texture2D(fx2Tex, reflection.xy);
        vec4 Tr = texture2D(fx2Tex, refraction.xy);

        vec4 cubemap = mix(Tl, Tr, FresRatio);

        cubemap.rgb = max(gl_FragColor.rgb, cubemap.rgb * litColor);

        gl_FragColor = mix(cubemap, gl_FragColor, specmask.a);
    }

    if(useGlow > 0) {
        glow = texture2D(fxTex, gl_TexCoord[0].xy);
        gl_FragColor = mix(gl_FragColor, glow, glow.a);
    }

    if(TEAM == 1) {
        gl_FragColor = mix(gl_FragColor, vec4(0.3, 0.0, 0.0, 1.0), fog);
        if(dot(worldNormal, EyeDir) <= 0.01) {
            gl_FragColor = max(vec4(1.0, 0.2, 0.0, 1.0) * (fog * 2.0), gl_FragColor);
        }
    } else if(TEAM == 2) {
        gl_FragColor = mix(gl_FragColor, vec4(0.0, 0.1, 0.4, 1.0), fog);
        if(dot(worldNormal, EyeDir) <= 0.01) {
            gl_FragColor = max(vec4(0.0, 0.2, 1.0, 1.0) * (fog * 2.0), gl_FragColor);
        }
    } else if(TEAM == 3) {
        gl_FragColor = mix(gl_FragColor, vec4(0.0, 0.4, 0.3, 1.0), fog);
        if(dot(worldNormal, EyeDir) <= 0.01) {
            gl_FragColor = max(vec4(0.2, 1.0, 0.0, 1.0) * (fog * 2.0), gl_FragColor);
        }
    } else if(FOG > 0)
        gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
}