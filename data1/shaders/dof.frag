uniform sampler2D renderedTexture;
uniform sampler2D depthTexture;

const float pi = 3.14159265f;
const float sigma = 3.0f;
const float blurSize = 1.0f / 512.0f; //Note - should be screen width

const vec2 blurMultiplyVecH = vec2(1.0f, 0.0f);
const vec2 blurMultiplyVecV = vec2(0.0f, 1.0f);

void main(void) {
	// Possible better way to use the depth
    float n = 1.0;
    float f = 3000.0;
    float z = texture2D(depthTexture, gl_TexCoord[0].xy).x;
    float blur = (2.0 * n) / (f + n - z * (f - n));

    float numBlurPixelsPerSide = 3.0f * blur;
    vec3 incrementalGaussian;
    incrementalGaussian.x = 1.0f / (sqrt(2.0f * pi) * sigma);
    incrementalGaussian.y = exp(-0.5f / (sigma * sigma));
    incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;

    vec4 avgValue = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    float coefficientSum = 0.0f;

	// Take the central sample first...
    avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy) * incrementalGaussian.x;
    coefficientSum += incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;

    for(float i = 1.0f; i <= numBlurPixelsPerSide; i++) {
        avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy - i * blurSize *
            blurMultiplyVecV) * incrementalGaussian.x;
        avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy + i * blurSize *
            blurMultiplyVecV) * incrementalGaussian.x;
        coefficientSum += 2 * incrementalGaussian.x;
        incrementalGaussian.xy *= incrementalGaussian.yz;
    }

	// Blur Horizontally
    for(float i = 1.0f; i <= numBlurPixelsPerSide; i++) {
        avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy - i * blurSize *
            blurMultiplyVecH) * incrementalGaussian.x;
        avgValue += texture2D(renderedTexture, gl_TexCoord[0].xy + i * blurSize *
            blurMultiplyVecH) * incrementalGaussian.x;
        coefficientSum += 2 * incrementalGaussian.x;
        incrementalGaussian.xy *= incrementalGaussian.yz;
    }

    gl_FragColor = mix(gl_FragColor, avgValue / coefficientSum, 1.0);
	//gl_FragColor = vec4(blur, blur, blur, 1.0); // Uncomment to see depth buffer.
}