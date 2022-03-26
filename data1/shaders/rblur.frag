uniform sampler2D rtextureSource;
uniform vec3 radialBlurParams;

const float sampleDist = 2.0;
const float sampleStrength = 2.0;

void main() {
    float samples[10];
    samples[0] = -0.08;
    samples[1] = -0.05;
    samples[2] = -0.03;
    samples[3] = -0.02;
    samples[4] = -0.01;
    samples[5] = 0.01;
    samples[6] = 0.02;
    samples[7] = 0.03;
    samples[8] = 0.05;
    samples[9] = 0.08;

    vec2 dir = vec2(0.5) - gl_TexCoord[0].st;
    float dist = sqrt(dir.x * dir.x + dir.y * dir.y);
    dir = dir / dist;

    vec4 color = texture2D(rtextureSource, gl_TexCoord[0].st);
    vec4 sum = color;

    for(int i = 0; i < 10; i++)
        sum += texture2D(rtextureSource, gl_TexCoord[0].st + dir * samples[i] * sampleDist);

    sum *= 1.0 / 11.0;
    float t = dist * sampleStrength;
    t = clamp(t, 0.0, 1.0);

    gl_FragColor = mix(color, sum, t);

	// Shade depending on effect.
    gl_FragColor *= vec4(radialBlurParams, 1.0);
}