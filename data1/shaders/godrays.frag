uniform vec2 lightPositionOnScreen;
uniform sampler2D sunTexture;
uniform float aspectRatio; //width/height
uniform float sunRadius;

//note - these could be made uniforms to control externally
const float exposure = 0.0034;
const float decay = 1.0;
const float density = 0.84;
const float weight = 5.65;
const int NUM_SAMPLES = 75;

void main() {
    vec2 deltaTextCoord = vec2(gl_TexCoord[0].st - lightPositionOnScreen.xy);
    vec2 textCoo = gl_TexCoord[0].st;
    float adjustedLength = length(vec2(deltaTextCoord.x * aspectRatio, deltaTextCoord.y));
    deltaTextCoord *= 1.0 / float(NUM_SAMPLES) * density;
    float illuminationDecay = 1.0;

    int lim = NUM_SAMPLES;

    if(adjustedLength > sunRadius) {		
		//first simulate the part of the loop for which we won't get any
		//samples anyway
        float ratio = (adjustedLength - sunRadius) / adjustedLength;
        lim = int(float(lim) * ratio);

        textCoo -= deltaTextCoord * lim;
        illuminationDecay *= pow(decay, lim);

		//next set up the following loop so it gets the correct number of
		//samples.
        lim = NUM_SAMPLES - lim;
    }

    gl_FragColor = vec4(0.0);
    for(int i = 0; i < lim; i++) {
        textCoo -= deltaTextCoord;

        vec4 sample = texture2D(sunTexture, textCoo);

        sample *= illuminationDecay * weight;

        gl_FragColor += sample;

        illuminationDecay *= decay;
    }
    gl_FragColor *= exposure;
}