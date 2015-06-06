#ifdef GL_ES
precision mediump float;
#endif

attribute vec2 inPosition;
attribute vec4 inColour;

varying vec4 varyColour;

void main()
{
    gl_Position	= vec4(inPosition.xy, 0, 1);
    varyColour = inColour;
}
