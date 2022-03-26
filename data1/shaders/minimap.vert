attribute vec2 colordata;

void main(void) {
	vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;

	gl_Position.xywz = vec4(pos.xyw, 0.0);

	gl_FrontColor.a = pos.z / -2.0;

	if(gl_FrontColor.a > 0.0) {
		gl_FrontColor.rgb = vec3(0.5, 0.5 + colordata[0], 0.5);
		gl_FrontColor.a = 1.0 - gl_FrontColor.a;
	} else {
		gl_FrontColor.rgb = vec3(0.5, colordata[0], 0);
		gl_FrontColor.a += 1.0;
	}

	gl_FrontColor.a *= colordata[1];
}