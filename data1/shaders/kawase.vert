// for an explanation of how this works, see these references:
// https://software.intel.com/en-us/blogs/2014/07/15/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms
// http://www.daionet.gr.jp/~masa/archives/GDC2003_DSTEAL.ppt
varying vec2 texcoord1, texcoord2, texcoord3, texcoord4;

// scale should be desired blur size (i.e. 9 for 9x9 blur) / 2 - 1. Since
// the desired blur size is always an odd number, scale always has a
// fractional part of 0.5. Blurs are created by running successive Kawase
// filters at increasing scales until the desired size is reached. Divide
// scale value by resolution of the input texture to get this scale vector.
uniform vec2 ScaleU;

void main() {
    gl_Position = ftransform();

	// If we do all this math here, and let GLSL do its built-in
	// interpolation of varying variables, the math still comes out right,
	// but it's faster.
    texcoord1 = gl_MultiTexCoord0.xy + ScaleU * vec2(-1, -1);
    texcoord2 = gl_MultiTexCoord0.xy + ScaleU * vec2(-1, 1);
    texcoord3 = gl_MultiTexCoord0.xy + ScaleU * vec2(1, -1);
    texcoord4 = gl_MultiTexCoord0.xy + ScaleU * vec2(1, 1);
}