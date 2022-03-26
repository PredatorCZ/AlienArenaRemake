uniform vec3 LightPos;
uniform vec3 left;
uniform vec3 up;
uniform sampler2D refTexture;
uniform sampler2D mirTexture;
uniform int FOG;
uniform int type; // 1 means mirror only, 2 means glass only, 3 means both

varying vec3 r;
varying float fog;
varying vec3 orig_normal, normal, vert;

void main(void) {
    vec3 light_dir = normalize(LightPos - vert);
    vec3 eye_dir = normalize(-vert.xyz);
    vec3 ref = normalize(-reflect(light_dir, normal));

    float ld = max(dot(normal, light_dir), 0.0);
    float ls = 0.75 * pow(max(dot(ref, eye_dir), 0.0), 0.70); //0.75 specular, .7 shininess

    float m = -1.0 * sqrt(r.x * r.x + r.y * r.y + (r.z + 1.0) * (r.z + 1.0));

    vec3 n_orig_normal = normalize(orig_normal);
    vec2 coord_offset = vec2(dot(n_orig_normal, left), dot(n_orig_normal, up));
    vec2 glass_coord = -vec2(r.x / m + 0.5, r.y / m + 0.5);
    vec2 mirror_coord = vec2(-gl_TexCoord[0].s, gl_TexCoord[0].t);

    vec4 mirror_color, glass_color;
    if(type == 1)
        mirror_color = texture2D(mirTexture, mirror_coord.st);
    else if(type == 3)
        mirror_color = texture2D(mirTexture, mirror_coord.st + coord_offset.st);
    if(type != 1)
        glass_color = texture2D(refTexture, glass_coord.st + coord_offset.st / 2.0);

    if(type == 3)
        gl_FragColor = 0.3 * glass_color + 0.3 * mirror_color * vec4(ld + ls + 0.35);
    else if(type == 2)
        gl_FragColor = glass_color / 2.0;
    else if(type == 1)
        gl_FragColor = mirror_color;

    if(FOG > 0)
        gl_FragColor = mix(gl_FragColor, gl_Fog.color, fog);
}