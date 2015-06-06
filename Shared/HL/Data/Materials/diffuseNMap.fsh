#ifdef GL_ES
    precision mediump float;
#endif

varying vec2 varyUV;
uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

void kill()
{
#ifdef GL_ESXXX
    gl_FragColor.a = 0.0;
#else
    discard;
#endif
}

void main()
{
    vec4 diffuse = texture2D(diffuseMap, varyUV.st, 0.0);
    vec4 nmap    = texture2D(normalMap,  varyUV.st, 0.0);

    gl_FragColor = diffuse;

    if (gl_FragColor.a < 0.5)
        kill();

    // gl_FragColor.rgb += nmap.a;
}
