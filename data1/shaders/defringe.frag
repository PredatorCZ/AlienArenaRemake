varying vec2 texcoordul, texcoorduc, texcoordur, texcoordcl, texcoordcc, texcoordcr, texcoordll, texcoordlc, texcoordlr;
uniform sampler2D textureSource;

void main() {
    vec4 center = texture2D(textureSource, texcoordcc);

	// fast path for non-transparent samples
    if(center.a > 0.0) {
        gl_FragColor = center;
        return;
    }

	// for opaque samples, we borrow color from neighbor pixels
    vec4 sum = vec4(0.0);
    vec4 sample;

    sample = texture2D(textureSource, texcoordul);
    sum += vec4(sample.xyz * sample.a, sample.a);
    sample = texture2D(textureSource, texcoorduc);
    sum += vec4(sample.xyz * sample.a, sample.a);
    sample = texture2D(textureSource, texcoordur);
    sum += vec4(sample.xyz * sample.a, sample.a);
    sample = texture2D(textureSource, texcoordcl);
    sum += vec4(sample.xyz * sample.a, sample.a);
    sample = texture2D(textureSource, texcoordcr);
    sum += vec4(sample.xyz * sample.a, sample.a);
    sample = texture2D(textureSource, texcoordll);
    sum += vec4(sample.xyz * sample.a, sample.a);
    sample = texture2D(textureSource, texcoordlc);
    sum += vec4(sample.xyz * sample.a, sample.a);
    sample = texture2D(textureSource, texcoordlr);
    sum += vec4(sample.xyz * sample.a, sample.a);

	// if there are no neighboring non-transparent pixels at all, nothing
	// we can do
    if(sum.a == 0.0) {
        gl_FragColor = center;
        return;
    }

    gl_FragColor = vec4(sum.xyz / sum.a, 0.0);
}