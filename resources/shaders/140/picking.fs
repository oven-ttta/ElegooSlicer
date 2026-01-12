#version 140

uniform vec3 object_id_color;

void main()
{
    gl_FragColor = vec4(object_id_color, 1.0);
}

