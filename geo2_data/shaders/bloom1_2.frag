#version 410 core

in vec2 tex_coord;

out vec4 frag_color;

uniform sampler2D texture_in;
uniform float sw[100]; //interleaved weights and steps. MAX_ITER in the program must be 0.5*len(sw) - 1
uniform int num_iter;
uniform bool is_horizontal;

void main()
{
    vec2 tex_dim_inv = 1.0 / textureSize(texture_in, 0);
    vec3 col;
    
    if(is_horizontal) {
        col = sw[2*0] * texture(texture_in, tex_coord).rgb;
        for(int i=1; i<=num_iter; i++) {
            col += sw[2*i] * texture(texture_in, tex_coord + vec2(tex_dim_inv.x * sw[2*i + 1], 0)).rgb;
            col += sw[2*i] * texture(texture_in, tex_coord - vec2(tex_dim_inv.x * sw[2*i + 1], 0)).rgb;
        }
    } else {
        col = sw[2*0] * texture(texture_in, tex_coord).rgb;
        for(int i=1; i<=num_iter; i++) {
            col += sw[2*i] * texture(texture_in, tex_coord + vec2(0, tex_dim_inv.y * sw[2*i + 1])).rgb;
            col += sw[2*i] * texture(texture_in, tex_coord - vec2(0, tex_dim_inv.y * sw[2*i + 1])).rgb;
        }
    }

    frag_color = vec4(col, 1.0);
}
