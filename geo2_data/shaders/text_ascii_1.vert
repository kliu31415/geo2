#version 410 core

//requires 4 vertices in a triangle strip

layout(std140) uniform characters
{
    //requires 4 vertices in a triangle strip
    vec4 data[341*3];
    //[0] = (x, y, w, h)
    //[1] = (font size, character (as int), (x1%: u16, y1%: u16), (x2%: u16, y2%: u16))
    //[2] = color
};

out vec2 tex_coord;

flat out int idx;

void main()
{
    idx = gl_InstanceID * 3;

    vec2 offset_mult = vec2(gl_VertexID%2, gl_VertexID/2);
    gl_Position = vec4(data[idx].xy + data[idx].zw * offset_mult, 0, 1);
    vec2 lower_left = unpackUnorm2x16(floatBitsToUint(data[idx+1][2]));
    vec2 upper_right_offset = unpackUnorm2x16(floatBitsToUint(data[idx+1][3]));
    tex_coord = lower_left + offset_mult * upper_right_offset;
}
