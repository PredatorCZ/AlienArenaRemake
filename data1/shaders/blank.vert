void main(void) {
    anim_compute(false, false);
    gl_Position = gl_ModelViewProjectionMatrix * anim_vertex;
    gl_TexCoord[0] = gl_MultiTexCoord0;
}