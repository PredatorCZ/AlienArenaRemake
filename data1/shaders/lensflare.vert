uniform vec3 up, right;
attribute float size, addup, addright;

void main() {
    float dist = length(gl_ModelViewMatrix * gl_Vertex) * 0.01;

		// Flares which are very close are too small to see; fade them out as
		// we get closer.
    float alpha = gl_Color.a;
    if(dist < 2.0)
        alpha *= (dist - 1.0) / 2.0;
    gl_FrontColor = gl_BackColor = vec4(gl_Color.rgb * alpha, 1.0);

    dist = min(dist, 10.0) * size + 1;

    vec4 vertex = gl_Vertex +
        addup * dist * vec4(up, 0) +
        addright * dist * vec4(right, 0);
    gl_Position = gl_ModelViewProjectionMatrix * vertex;
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_FogFragCoord = length(gl_Position);
}