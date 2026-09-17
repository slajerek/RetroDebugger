#version 150

uniform vec4 scale;
uniform vec2 view_size;

uniform mat4 rotation;

in vec4 position;
in vec2 tex;

out vec2 tex_coord;

void main() {
    gl_Position = position * scale * rotation;
    tex_coord = (tex * (view_size - 1.0) + 0.5) / view_size;
}
