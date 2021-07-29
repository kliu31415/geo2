#version 410 core

layout(std140) uniform block1
{
    //requires 4 vertices in a triangle strip
    vec4 data[204*5];
    //[0] = (x0, y0, x1, y1)
    //[1] = (x2, y2, x_border_cutoff, y_border_cutoff)
    //[2] = (v0 bary x, v0 bary y, v3 bary x, v3 bary y)
    //[3] = inner color
    //[4] = border color
};

flat out int idx;
out vec2 bary_coord;

void main()
{
    idx = gl_InstanceID*5;

    if(gl_VertexID == 0) {
        gl_Position = vec4(data[idx].xy, 0.0, 1.0);
        bary_coord = data[idx+2].xy;
    } else if(gl_VertexID == 1) {
        gl_Position = vec4(data[idx].zw, 0.0, 1.0);
        bary_coord = vec2(data[idx+2].zy);
    } else if(gl_VertexID == 2) {
        gl_Position = vec4(data[idx+1].xy, 0.0, 1.0);
        bary_coord = vec2(data[idx+2].xw);
    } else {
        gl_Position = vec4(data[idx+1][0] + data[idx][2] - data[idx][0],
                           data[idx+1][1] + data[idx][3] - data[idx][1], 0.0, 1.0);
        bary_coord = vec2(data[idx+2].zw);
    }
}
