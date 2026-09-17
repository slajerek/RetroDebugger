#version 150

uniform sampler2D this_frame;
in vec2 tex_coord;
out vec4 output_color;

void main() {
    output_color = texture(this_frame, tex_coord);
}
