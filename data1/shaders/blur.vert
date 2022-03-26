varying vec2 texcoord1, texcoord2, texcoord3, texcoord4, texcoord5, texcoord6, texcoord7, texcoord8, texcoord9;
uniform vec2 ScaleU;

void main() {
    gl_Position = ftransform();

	// If we do all this math here, and let GLSL do its built-in
	// interpolation of varying variables, the math still comes out right,
	// but it's faster.
    texcoord1 = gl_MultiTexCoord0.xy - 4.0 * ScaleU;
    texcoord2 = gl_MultiTexCoord0.xy - 3.0 * ScaleU;
    texcoord3 = gl_MultiTexCoord0.xy - 2.0 * ScaleU;
    texcoord4 = gl_MultiTexCoord0.xy - ScaleU;
    texcoord5 = gl_MultiTexCoord0.xy;
    texcoord6 = gl_MultiTexCoord0.xy + ScaleU;
    texcoord7 = gl_MultiTexCoord0.xy + 2.0 * ScaleU;
    texcoord8 = gl_MultiTexCoord0.xy + 3.0 * ScaleU;
    texcoord9 = gl_MultiTexCoord0.xy + 4.0 * ScaleU;
}