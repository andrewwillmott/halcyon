#ifdef GL_ES
    precision mediump float;
#endif

varying vec2 varyUV;
varying vec4 varyColour;

uniform sampler2D diffuseTexture;

void main (void)
{
    vec4 c = varyColour;
    float ta = texture2D(diffuseTexture, varyUV).a;

    c.a *= ta;

    gl_FragColor = c;
}
