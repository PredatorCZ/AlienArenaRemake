uniform sampler2D baseTex;

void main(void) {
    vec4 alphamask = texture2D(baseTex, gl_TexCoord[0].xy);

    gl_FragColor = vec4(1.0, 1.0, 1.0, alphamask.a);
}