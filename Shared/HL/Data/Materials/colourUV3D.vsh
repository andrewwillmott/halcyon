#ifdef GL_ES
// breaks on A7   precision mediump float;
    #define LP lowp
    #define MP mediump
#else
    #define LP
    #define MP
#endif

uniform mat4 modelToClip;

attribute vec4 inPosition;
attribute vec4 inColour;
attribute vec2 inUV;

varying vec2 varyUV;
varying LP vec4 varyColour;

void main (void)
{
	gl_Position	= modelToClip * inPosition;
    varyUV = inUV;
    varyColour = inColour;
}
