varying vec3 lightDir;
varying vec3 eyeDir;
varying float FresRatio;

varying float fog;

uniform sampler2D refTexture;
uniform sampler2D normalMap;
uniform sampler2D baseTexture;

uniform float TRANSPARENT;
uniform int FOG;

void main(void) {
    vec4 refColor;

    vec3 vVec = normalize(eyeDir);

    refColor = texture2D(refTexture, gl_TexCoord[0].xy);

    vec3 bump = normalize(texture2D(normalMap, gl_TexCoord[1].xy).xyz - 0.5);
    vec3 secbump = normalize(texture2D(normalMap, gl_TexCoord[2].xy).xyz - 0.5);
    vec3 modbump = mix(secbump, bump, 0.5);

    vec3 reflection = reflect(vVec, modbump);
    vec3 refraction = refract(vVec, modbump, 0.66);

    vec4 Tl = texture2D(baseTexture, reflection.xy);

    vec4 Tr = texture2D(baseTexture, refraction.xy);

    vec4 cubemap = mix(Tl, Tr, FresRatio);

    gl_FragColor = mix(cubemap, refColor, 0.5);

    gl_FragColor.a = TRANSPARENT;

    if(FOG > 0)
        gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);

}