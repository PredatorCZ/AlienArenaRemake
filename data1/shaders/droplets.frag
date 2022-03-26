uniform sampler2D drSource;
uniform sampler2D drTex;

void main(void) {
    vec3 noiseVec;
    vec3 noiseVec2;
    vec2 displacement;

    displacement = gl_TexCoord[1].st;

    noiseVec = normalize(texture2D(drTex, displacement.xy)).xyz;
    noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

    displacement = gl_TexCoord[2].st;

    noiseVec2 = normalize(texture2D(drTex, displacement.xy)).xyz;
    noiseVec2 = (noiseVec2 * 2.0 - 0.635) * 0.035;

	//clamp edges to prevent artifacts
    if(gl_TexCoord[0].s > 0.1 && gl_TexCoord[0].s < 0.992)
        displacement.x = gl_TexCoord[0].s + noiseVec.x + noiseVec2.x;
    else
        displacement.x = gl_TexCoord[0].s;

    if(gl_TexCoord[0].t > 0.1 && gl_TexCoord[0].t < 0.972)
        displacement.y = gl_TexCoord[0].t + noiseVec.y + noiseVec2.y;
    else
        displacement.y = gl_TexCoord[0].t;

    gl_FragColor = texture2D(drSource, displacement.xy);
}