#ifdef GL_ES
// breaks on A7   precision mediump float;
#endif

uniform mat4 modelToClip;

attribute vec4 inPosition;
attribute vec4 inColour;

varying vec4 varyColour;

void main()
{
    gl_Position	= modelToClip * inPosition;
    varyColour = inColour;
}
