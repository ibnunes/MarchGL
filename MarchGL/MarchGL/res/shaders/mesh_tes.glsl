#version 460 core
#extension GL_ARB_shading_language_include : require

layout(triangles, equal_spacing, ccw) in;

in vec3 tcsPos[];
in vec3 tcsNormal[];

out vec3 tesPos;
out vec3 tesNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float iTime;
uniform sampler2D noiseTexture;

#define ONE 0.00390625
#define ONEHALF 0.001953125

int[] perm = int[] (
  151,160,137,91,90,15,
  131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
  190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
  88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
  77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
  102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
  135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
  5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
  223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
  129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
  251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
  49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
  138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
);

float hash( vec2 p ) {
	float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*43758.5453123);
}

float fade(float t) {
    return t*t*t*(t*(t*6.0-15.0)+10.0);
}

float noise( vec3 point ) {
    vec3 iPoint = ONE * floor(point) + ONEHALF;
    vec3 fPoint = point - floor(point);
    
    // Noise on (0, 0, z)
    float perm00 = texture(noiseTexture, iPoint.xy).a;
    vec3 grad000 = texture(noiseTexture, vec2(perm00, iPoint.z)).rgb * 4.0 - 1.0;
    float n000 = dot(grad000, fPoint);
    
    vec3  grad001 = texture(noiseTexture, vec2(perm00, iPoint.z + ONE)).rgb * 4.0 - 1.0;
    float n001 = dot(grad001, fPoint - vec3(0.0, 0.0, 1.));
    
    // Noise on (0, 1, z)
    float perm01 = texture(noiseTexture, iPoint.xy + vec2(0.0, ONE)).a;
    vec3  grad010 = texture(noiseTexture, vec2(perm01, iPoint.z)).rgb * 4.0 - 1.0;
    float n010 = dot(grad010, fPoint - vec3(0.0, 1.0, 0.0));
    
    vec3  grad011 = texture(noiseTexture, vec2(perm01, iPoint.z + ONE)).rgb * 4.0 - 1.0;
    float n011 = dot(grad011, fPoint - vec3(0.0, 1.0, 1.0));
    
    // Noise on (1, 0, z)
    float perm10  = texture(noiseTexture, iPoint.xy + vec2(ONE, 0.0)).a ;
    vec3  grad100 = texture(noiseTexture, vec2(perm10, iPoint.z)).rgb * 4.0 - 1.0;
    float n100    = dot(grad100, fPoint - vec3(1.0, 0.0, 0.0));
    
    vec3  grad101 = texture(noiseTexture, vec2(perm10, iPoint.z + ONE)).rgb * 4.0 - 1.0;
    float n101    = dot(grad101, fPoint - vec3(1.0, 0.0, 1.0));
    
    
    // Noise on (1, 1, z)
    float perm11  = texture(noiseTexture, iPoint.xy + vec2(ONE, ONE)).a ;
    vec3  grad110 = texture(noiseTexture, vec2(perm11, iPoint.z)).rgb * 4.0 - 1.0;
    float n110    = dot(grad110, fPoint - vec3(1.0, 1.0, 0.0));
    
    vec3  grad111 = texture(noiseTexture, vec2(perm11, iPoint.z + ONE)).rgb * 4.0 - 1.0;
    float n111    = dot(grad111, fPoint - vec3(1.0, 1.0, 1.0));
    
    // Blend contributions along x
    vec4 n_x = mix(vec4(n000, n001, n010, n011), vec4(n100, n101, n110, n111), fade(fPoint.x));

    // Blend contributions along y
    vec2 n_xy = mix(n_x.xy, n_x.zw, fade(fPoint.y));

    // Blend contributions along z
    float n_xyz = mix(n_xy.x, n_xy.y, fade(fPoint.z));
 
    return n_xyz;
}

float getDensity(vec3 p) {
    float x = p.x;
    float y = p.y;
    float z = p.z;

    // <IFunction>

    return 0.0f;
}

vec3 getNormal(vec3 p) {
    vec2 e = vec2(0.001f, 0.0f);

    return normalize(
        getDensity(p) - vec3(
            getDensity(p - e.xyy),
            getDensity(p - e.yxy),
            getDensity(p - e.yyx)
        )
    );
}

vec3 interpolate3D(vec3 p1, vec3 p2, vec3 p3) {
    return vec3(gl_TessCoord.x) * p1 + vec3(gl_TessCoord.y) * p2 + vec3(gl_TessCoord.z) * p3;
}


// TODO: Understand given constants from this shader and rewrite the whole main function
void main() {
    vec3 uv = gl_TessCoord.xyz;

    vec4 pos0 = gl_in[0].gl_Position;
    vec4 pos1 = gl_in[1].gl_Position;
    vec4 pos2 = gl_in[2].gl_Position;

    vec3 tessCoord  = interpolate3D(tcsPos[0], tcsPos[1], tcsPos[2]);
    vec3 tessNormal = normalize(interpolate3D(tcsNormal[0], tcsNormal[1], tcsNormal[2]));
    
    // Displacement ammount

    float displacement = (noise(tessCoord * cos(iTime)) * 0.25);
    vec3 trueNormal = getNormal(tessCoord);

    tesPos = tessCoord;// + trueNormal * (dot(trueNormal, tessNormal)); //* displacement * 0.25;
    tesNormal = getNormal(tesPos);

    gl_Position = projection * view * vec4(tesPos, 1.0);
}