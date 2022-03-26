uniform float rsTime;
uniform vec3 up, right;
attribute float swayCoef, addup, addright;

void main() {
		// use cosine so that negative swayCoef is different from positive
    float sway = swayCoef * cos(swayCoef * rsTime);
    vec4 swayvec = vec4(sway, sway, 0, 0);
    vec4 vertex = gl_Vertex + swayvec +
        addup * vec4(up, 0) + addright * vec4(right, 0);
    gl_Position = gl_ModelViewProjectionMatrix * vertex;
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_FrontColor = gl_BackColor = gl_Color;
    gl_FogFragCoord = length(gl_Position);
}