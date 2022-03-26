uniform float fadeShadow;

varying vec4 sPos;

void main(void) {
    gl_FragColor = vec4(1.0 / fadeShadow * lookup_otherstatic_always(sPos, 0.3));
}