#ifdef GL_ES
    precision mediump float;
#endif

varying vec2 varyUV;
varying vec4 varyColour;

uniform sampler2D diffuseTexture;

void main (void)
{
    vec4 c = varyColour;
    float f = texture2D(diffuseTexture, varyUV).a;
    c.a *= f;
    gl_FragColor = c;

//    vec4 tex = texture2D(diffuseTexture, varyUV);
//    gl_FragColor = vec4(tex.r, tex.a, tex.b, 1.0);
//    gl_FragColor = vec4(varyUV, 0.0f, 1.0f);
}
