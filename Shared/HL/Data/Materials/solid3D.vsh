uniform mat4 modelToClip;

attribute vec4 inPosition;

varying vec4 varyColour;

void main()
{
    gl_Position	= modelToClip * inPosition;
    varyColour = vec4(1.0, 0.5, 0.0, 1.0);
}
