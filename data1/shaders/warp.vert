// Old-style per-vertex water effects
// These days commonly combined with transparency to form a mist volume effect
// Be sure to check the warptest map when you change this
// TODO: both passes into a single run of this shader, using a fragment
// shader. If each pass has an alpha of n, then the the output (incl. alpha)
// should be n * pass2 + (n - n*n) * pass1.

uniform float time;
uniform int warpvert;
uniform int envmap; // select which pass

// = 1/2 wave amplitude on each axis
// = 1/4 wave amplitude on both axes combined
const float wavescale = 2.0;

void main() {
    gl_FrontColor = gl_Color;

		// warping effect
    vec4 vert = gl_Vertex;

    if(warpvert != 0) {
        vert[2] += wavescale *
            (sin(vert[0] * 0.025 + time) * sin(vert[2] * 0.05 + time) +
            sin(vert[1] * 0.025 + time * 2) * sin(vert[2] * 0.05 + time) -
            2 // top of brush = top of waves
        );
    }

    gl_Position = gl_ModelViewProjectionMatrix * vert;

    if(envmap == 0) // main texture
    {
        gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(gl_MultiTexCoord0.s + 4.0 * sin(gl_MultiTexCoord0.t / 8.0 + time), gl_MultiTexCoord0.t + 4.0 * sin(gl_MultiTexCoord0.s / 8.0 + time), 0, 1);
    } else // env map texture (if enabled)
    {
        gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    }
}