uniform vec4 exponent;
uniform sampler2D textureSource;

void main(void) {
    gl_FragColor = pow(texture2D(textureSource, gl_TexCoord[0].st), exponent);
}