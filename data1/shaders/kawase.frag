varying vec2 texcoord1, texcoord2, texcoord3, texcoord4;
uniform sampler2D textureSource;

void main() {
    gl_FragColor = (texture2D(textureSource, texcoord1) +
        texture2D(textureSource, texcoord2) +
        texture2D(textureSource, texcoord3) +
        texture2D(textureSource, texcoord4)) / 4.0;
}