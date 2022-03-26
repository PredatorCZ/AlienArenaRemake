uniform vec3 staticLightColor;

varying vec3 StaticLightDir;

void main() {
    gl_FragColor.rgb = vec3(max(1.5 * staticLightColor * normalize(StaticLightDir).z, 0.075));
    gl_FragColor.a = 1.0;
}