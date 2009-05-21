uniform sampler2D fbtexture;
uniform sampler2D distortiontexture;
uniform float frametime;
uniform vec2 fxPos;
uniform int fxType;
uniform vec3 fxColor;

void main(void)
{
	vec3 noiseVec;
	vec2 displacement;
	float scaledTimer;

	scaledTimer = frametime*0.1;

    	displacement = gl_TexCoord[0].st;

	displacement.x -= fxPos.x*0.002;
	displacement.y -= fxPos.y*0.002;

	noiseVec = normalize(texture2D(distortiontexture, displacement.xy)).xyz;
	noiseVec = (noiseVec * 2.0 - 0.635) * 0.035;

	gl_FragColor = texture2D(fbtexture, gl_TexCoord[0].st);
	
	vec4 fxTex = texture2D(fbtexture, gl_TexCoord[0].st + noiseVec.xy);

	gl_FragColor = mix(gl_FragColor, fxTex, 1.0);

	if(fxType == 2)
		gl_FragColor = mix(gl_FragColor, vec4(fxColor, 1.0), 0.5);

}