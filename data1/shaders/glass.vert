uniform int FOG;

varying vec3 r;
varying float fog;
varying vec3 orig_normal, normal, vert;

void main(void) {
    anim_compute(false, true);

    gl_Position = gl_ModelViewProjectionMatrix * anim_vertex;

    vec3 u = normalize(vec3(gl_ModelViewMatrix * anim_vertex));
    vec3 n = normalize(gl_NormalMatrix * anim_normal);

    r = reflect(u, n);

    normal = n;
    vert = vec3(gl_ModelViewMatrix * anim_vertex);

    orig_normal = anim_normal;

    if(FOG > 0) {
        fog = (gl_Position.z - gl_Fog.start) / (gl_Fog.end - gl_Fog.start);
        fog = clamp(fog, 0.0, 0.3); //any higher and meshes disappear
    }

	// for mirroring
    gl_TexCoord[0] = gl_MultiTexCoord0;
}