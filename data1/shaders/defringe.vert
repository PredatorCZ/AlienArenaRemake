varying vec2 texcoordul, texcoorduc, texcoordur, texcoordcl, texcoordcc, texcoordcr, texcoordll, texcoordlc, texcoordlr;
uniform vec2 ScaleU; // Should be set to the pixel size of the image

void main() {
    gl_Position = ftransform();

    texcoordul = texcoorduc = texcoordur = texcoordcl = texcoordcc = texcoordcr = texcoordlr = texcoordlc = texcoordlr = gl_MultiTexCoord0.xy;
    texcoordul.y += 1.0 / ScaleU.y;
    texcoorduc.y += 1.0 / ScaleU.y;
    texcoordur.y += 1.0 / ScaleU.y;
    texcoordll.y -= 1.0 / ScaleU.y;
    texcoordlc.y -= 1.0 / ScaleU.y;
    texcoordlr.y -= 1.0 / ScaleU.y;
    texcoordur.x += 1.0 / ScaleU.x;
    texcoordcr.x += 1.0 / ScaleU.x;
    texcoordlr.x += 1.0 / ScaleU.x;
    texcoordul.x -= 1.0 / ScaleU.x;
    texcoordcl.x -= 1.0 / ScaleU.x;
    texcoordll.x -= 1.0 / ScaleU.x;
}