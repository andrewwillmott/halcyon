#ifdef GL_ES
    precision mediump float;
    #define LP lowp
    #define MP mediump
#else
    #define LP
    #define MP
#endif

varying MP vec2 varyUV;
varying LP vec4 varyColour;

uniform sampler2D diffuseTexture;

void main (void)
{
    vec4 c = varyColour;
    vec4 tc = texture2D(diffuseTexture, varyUV);

    gl_FragColor = c * tc;
//    gl_FragColor = vec4(varyUV, 0.0f, 1.0f);
}
