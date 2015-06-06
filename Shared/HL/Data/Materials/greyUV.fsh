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

    c *= tc;
    float s = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    s = 0.4 + 0.2 * s;

    gl_FragColor = vec4(s, s, s, c.a);
}
