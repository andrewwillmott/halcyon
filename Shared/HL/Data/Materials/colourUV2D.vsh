#ifdef GL_ES
    precision mediump float;
#endif

attribute vec2 inPosition;
attribute vec2 inTexCoord;
attribute vec4 inColour;

varying vec2 varyUV;
varying vec4 varyColour;

void main()
{
    gl_Position	= vec4(inPosition.xy, 0, 1);
    varyUV = inTexCoord;
    varyColour = inColour;
}
