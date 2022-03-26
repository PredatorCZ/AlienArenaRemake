uniform sampler2D fbtexture;
uniform sampler2D distortiontexture;
uniform float intensity;

void main(void) {
    vec2 noiseVec;
    vec4 displacement;

    noiseVec = normalize(texture2D(distortiontexture, gl_TexCoord[0].st)).xy;

	// It's a bit of a rigomorole to do partial screen updates. This can
	// go away as soon as the distort texture is a screen sized buffer.
    displacement = gl_TexCoord[0] + vec4((noiseVec * 2.0 - vec2(0.6389, 0.6339)) * intensity, 0, 0);
    gl_FragColor = texture2D(fbtexture, (gl_TextureMatrix[0] * displacement).st);
}