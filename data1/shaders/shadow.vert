varying vec4 sPos;

void main(void) {
	sPos = gl_Vertex;

	gl_Position = ftransform();

	gl_Position.z -= 0.05; //eliminate z-fighting on some drivers
}