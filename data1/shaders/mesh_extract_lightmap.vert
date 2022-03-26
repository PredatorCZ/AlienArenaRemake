uniform vec3 staticLightPosition;

varying vec3 StaticLightDir;

void main() {
    anim_compute(true, true);
    StaticLightDir = tangentSpaceTransform * staticLightPosition;
    gl_Position = gl_MultiTexCoord0;
    gl_Position.xy *= 2.0;
    gl_Position.xy -= vec2(1.0);
}