#version 150

uniform int texture_filter;
uniform sampler2D last_frame;
uniform sampler2D this_frame;
uniform vec2 source_size;

in vec2 tex_coord;

out vec4 output_color;

/*
 * Adapted from GPL 2.0 licenced bicubic implemenation at:
 * https://github.com/hizzlekizzle/quark-shaders/blob/master/Bicubic.shader/bicubic.fs
 */

float weight(float x)
{
    float ax = abs(x);
    // Mitchel-Netravali coefficients.
    // Best psychovisual result.
    const float B = 1.0 / 3.0;
    const float C = 1.0 / 3.0;

    if (ax < 1.0) {
        return (
            pow(x, 2.0) * (
                (12.0 - 9.0 * B - 6.0 * C) * ax +
                (-18.0 + 12.0 * B + 6.0 * C)
            ) +
            (6.0 - 2.0 * B)
        ) / 6.0;

    } else if ((ax >= 1.0) && (ax < 2.0)) {
        return (
            pow(x, 2.0) * (
                (-B - 6.0 * C) * ax +
                (6.0 * B + 30.0 * C)
            ) +
            (-12.0 * B - 48.0 * C) * ax +
            (8.0 * B + 24.0 * C)
        ) / 6.0;

    } else {
        return 0.0;
    }
}

vec4 weight4(float x)
{
    return vec4(
        weight(x + 1.0),
        weight(x),
        weight(1.0 - x),
        weight(2.0 - x));
}

vec3 pixel(float xpos, float ypos)
{
    vec4 last_frame_color = texture(last_frame, vec2(xpos, ypos));
    vec4 this_frame_color = texture(this_frame, vec2(xpos, ypos));

    return (last_frame_color.rgb * (1.0 - this_frame_color.a)) + (this_frame_color.rgb * this_frame_color.a);
}

vec3 line(float ypos, vec4 xpos, vec4 linetaps)
{
    return
        pixel(xpos.r, ypos) * linetaps.r +
        pixel(xpos.g, ypos) * linetaps.g +
        pixel(xpos.b, ypos) * linetaps.b +
        pixel(xpos.a, ypos) * linetaps.a;
}

void main()
{
    vec2 stepxy = 1.0 / source_size.xy;
    vec2 pos = tex_coord.xy + stepxy * 0.5;
    vec2 f = fract(pos / stepxy);

    vec4 linetaps   = weight4(f.x);
    vec4 columntaps = weight4(f.y);

    // make sure all taps added together is exactly 1.0, otherwise some
    // (very small) distortion can occur
    linetaps   /= linetaps.r   + linetaps.g   + linetaps.b   + linetaps.a;
    columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

    vec2 xystart = (-1.5 - f) * stepxy + pos;
    vec4 xpos = vec4(
        xystart.x,
        xystart.x + stepxy.x,
        xystart.x + stepxy.x * 2.0,
        xystart.x + stepxy.x * 3.0);

    output_color.a = 1.0;
    output_color.rgb = line(xystart.y, xpos, linetaps) * columntaps.r + line(xystart.y + stepxy.y, xpos, linetaps) * columntaps.g + line(xystart.y + stepxy.y * 2.0, xpos, linetaps) * columntaps.b + line(xystart.y + stepxy.y * 3.0, xpos, linetaps) * columntaps.a;
}