#ifdef GL_ES
    precision mediump float;
    #define LP lowp
    #define MP mediump
#else
    #define LP
    #define MP
#endif

uniform mat4 modelToClip;

attribute vec4 inPosition;
attribute vec2 inUV;

varying vec2 varyUV;

void main (void)
{
	gl_Position	= modelToClip * inPosition;
    varyUV = inUV;
}
