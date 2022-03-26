varying vec2 texcoord1, texcoord2, texcoord3, texcoord4, texcoord5, texcoord6, texcoord7, texcoord8, texcoord9;
uniform sampler2D textureSource;

void main() {
    vec4 sum = vec4(0.0);

	// take nine samples
    sum += texture2D(textureSource, texcoord1) * 0.05;
    sum += texture2D(textureSource, texcoord2) * 0.09;
    sum += texture2D(textureSource, texcoord3) * 0.12;
    sum += texture2D(textureSource, texcoord4) * 0.15;
    sum += texture2D(textureSource, texcoord5) * 0.16;
    sum += texture2D(textureSource, texcoord6) * 0.15;
    sum += texture2D(textureSource, texcoord7) * 0.12;
    sum += texture2D(textureSource, texcoord8) * 0.09;
    sum += texture2D(textureSource, texcoord9) * 0.05;

    gl_FragColor = sum;
}