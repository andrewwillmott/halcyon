#ifdef GL_ES
precision highp float;
#endif

varying vec2 varyUV;
varying vec4 varyColour;

uniform sampler2D diffuseTexture;
uniform vec3 effectParam1;

void main (void)
{
    vec4 c = varyColour;
    vec4 tc = texture2D(diffuseTexture, varyUV);

    tc.rgb *= effectParam1;

    gl_FragColor = tc;
//    gl_FragColor = vec4(varyUV, 0.0f, 1.0f);
}
