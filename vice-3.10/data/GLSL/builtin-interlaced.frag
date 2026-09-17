#version 150

uniform sampler2D last_frame;
uniform sampler2D this_frame;
in vec2 tex_coord;
out vec4 output_color;

void main() {
    vec4 last_frame_color = texture(last_frame, tex_coord);
    vec4 this_frame_color = texture(this_frame, tex_coord);

    output_color.a = 1.0;
    output_color.rgb = (last_frame_color.rgb * (1.0 - this_frame_color.a)) + (this_frame_color.rgb * this_frame_color.a);
}
