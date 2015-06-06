uniform	mat4 modelToClip;

attribute vec4 inPosition;  

void main()
{
    gl_Position = modelToClip * inPosition;
}