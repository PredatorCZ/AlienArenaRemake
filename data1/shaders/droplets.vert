uniform float drTime;

void main(void) {
    gl_Position = ftransform();

	//for vertical scrolling
    vec4 texco = gl_MultiTexCoord0;
    texco.t = texco.t + drTime * 1.0;
    gl_TexCoord[1] = texco;

    texco = gl_MultiTexCoord0;
    texco.t = texco.t + drTime * 0.8;
    gl_TexCoord[2] = texco;

    gl_TexCoord[0] = gl_MultiTexCoord0;
}