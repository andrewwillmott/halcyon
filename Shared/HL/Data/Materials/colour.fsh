#ifdef GL_ES
    precision mediump float;
    #define LP lowp
    #define MP mediump
#else
    #define LP
    #define MP
#endif


varying LP vec4 varyColour;

void main (void)
{
    gl_FragColor = varyColour;
}
