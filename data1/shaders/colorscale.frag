uniform vec3 scale;
uniform sampler2D textureSource;

void main(void) {
    gl_FragColor = texture2D(textureSource, gl_TexCoord[0].st) * vec4(scale, 1.0);
}