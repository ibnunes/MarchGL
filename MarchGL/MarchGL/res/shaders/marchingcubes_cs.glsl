#version 450 core
#extension GL_ARB_shading_language_include : require

struct TRIANGLES {
    vec4 p[12];
};

layout(std430, binding = 1) buffer Input2 {
    int edgeTable[256] ;
};


layout(std430, binding = 2) buffer Input3 {
    int triTable[256][16];
};


layout(std140, binding = 3) buffer Output {
    TRIANGLES allTriangles [];
};

layout(std140, binding = 4) buffer OutputNormals {
    TRIANGLES allNormals [];
};



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



//float dist = 1.0f;
//float radius = 1.0f;
//int obj = 0;

//uniform float radius;
uniform float dist;
//uniform int obj; //0-sphere 1-torus

//lim of the x, y, and z axis
uniform int x_size;
uniform int y_size;
uniform int z_size;

uniform int MX;
uniform int MY;
uniform int MZ;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;


vec4 getIntersVertice(vec4 p1, vec4 p2, float D1, float D2) {

    if (abs(D1) < 0.00001)
        return p1;

    if (abs(D2) < 0.00001)
        return p2;

    if (abs(D1 - D2) < 0.00001)
        return p1;

    float t = -D1 / (D2 - D1);

    return ( 1 - t ) * p1 + t * p2;
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

void main() {

    uint index_x = gl_GlobalInvocationID.x;
    uint index_y = gl_GlobalInvocationID.y;
    uint index_z = gl_GlobalInvocationID.z;

    //int bin = 0b00000000;
    int bin[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    //8 points of the vertex
    vec4 p[8];
    p[0] = vec4(index_x * dist - x_size, index_y * dist - y_size, index_z * dist - z_size, 1.0f);
    //p[0] = voxels[index_x][index_y][index_z];
    p[1] = p[0] + vec4(dist, 0.0f, 0.0f, 0.0f);
    p[2] = p[0] + vec4(dist, dist, 0.0f, 0.0f);
    p[3] = p[0] + vec4(0.0f, dist, 0.0f, 0.0f);
    p[4] = p[0] + vec4(0.0f, 0.0f, dist, 0.0f);
    p[5] = p[0] + vec4(dist, 0.0f, dist, 0.0f);
    p[6] = p[0] + vec4(dist, dist, dist, 0.0f);
    p[7] = p[0] + vec4(0.0f, dist, dist, 0.0f);



    //densities
    float d[8];
    for (int i = 0; i < 8; i++) {
        d[i] = getDensity(p[i].xyz);

        //check if is inside the sphere
        if (d[i] < 0.0f)
            bin[7 - i] = 1;
    }


    //convert to int
    int bin_int = 0;
    int allBin[] = { 128, 64, 32, 16, 8, 4, 2, 1 };

    //0011 0010
    for (int i = 7; i >= 0; i--)
        bin_int += bin[i] * allBin[i];


    int edgeFlag = edgeTable[bin_int];
    vec4 edgeVertices[12];


    uint width = (x_size * 2) ;
    uint height = (y_size * 2);
    uint depth = z_size * 2;
    uint def_index = index_x + index_y * MX + index_z * MY * MX;
    for (int i = 0; i < 12; i++) {
        //allTriangles[index_x][index_y][index_z].p[i] = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        //allNormals[index_x][index_y][index_z].p[i] = vec4(0.0f, 0.0f, 0.0f, 0.0f);

        allTriangles[def_index].p[i] = vec4(0.0f, 0.0f, 0.0f, 0.0f);
        allNormals[def_index].p[i] = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }


    if (edgeFlag != 0) {
        if (( edgeFlag & 1 ) != 0)
            edgeVertices[0] = getIntersVertice(p[0], p[1], d[0], d[1]); //edge 0
        if (( edgeFlag & 2 ) != 0)
            edgeVertices[1] = getIntersVertice(p[1], p[2], d[1], d[2]); //edge 1
        if (( edgeFlag & 4 ) != 0)
            edgeVertices[2] = getIntersVertice(p[2], p[3], d[2], d[3]); //edge 2
        if (( edgeFlag & 8 ) != 0)
            edgeVertices[3] = getIntersVertice(p[3], p[0], d[3], d[0]); //edge 3
        if (( edgeFlag & 16 ) != 0)
            edgeVertices[4] = getIntersVertice(p[4], p[5], d[4], d[5]); //edge 4
        if (( edgeFlag & 32 ) != 0)
            edgeVertices[5] = getIntersVertice(p[5], p[6], d[5], d[6]); //edge 5
        if (( edgeFlag & 64 ) != 0)
            edgeVertices[6] = getIntersVertice(p[6], p[7], d[6], d[7]); //edge 6
        if (( edgeFlag & 128 ) != 0)
            edgeVertices[7] = getIntersVertice(p[7], p[4], d[7], d[4]); //edge 7
        if (( edgeFlag & 256 ) != 0)
            edgeVertices[8] = getIntersVertice(p[0], p[4], d[0], d[4]); //edge 8
        if (( edgeFlag & 512 ) != 0)
            edgeVertices[9] = getIntersVertice(p[1], p[5], d[1], d[5]); //edge 9
        if (( edgeFlag & 1024 ) != 0)
            edgeVertices[10] = getIntersVertice(p[2], p[6], d[2], d[6]); //edge 10
        if (( edgeFlag & 2048 ) != 0)
            edgeVertices[11] = getIntersVertice(p[3], p[7], d[3], d[7]); //edge 11

        //index if the work groups were 1d
        //uint def_index = index_x * (gl_NumWorkGroups.y * gl_NumWorkGroups.z) + index_y * gl_NumWorkGroups.z + index_z;

        //def_index = (index_z - 1) * gl_NumWorkGroups.x * gl_NumWorkGroups.y+ (index_x - 1) * gl_NumWorkGroups.y+ index_y;

        for (int n = 0; triTable[bin_int][n] != -1; n += 3) {
            allTriangles[def_index].p[n] = edgeVertices[triTable[bin_int][n]];
            allTriangles[def_index].p[n + 1] = edgeVertices[triTable[bin_int][n + 1]];
            allTriangles[def_index].p[n + 2] = edgeVertices[triTable[bin_int][n + 2]];


            //normalization
            vec3 a = vec3(allTriangles[def_index].p[n].x, allTriangles[def_index].p[n].y, allTriangles[def_index].p[n].z);
            vec3 b = vec3(allTriangles[def_index].p[n + 1].x, allTriangles[def_index].p[n + 1].y, allTriangles[def_index].p[n + 1].z);
            vec3 c = vec3(allTriangles[def_index].p[n + 2].x, allTriangles[def_index].p[n + 2].y, allTriangles[def_index].p[n + 2].z);

            //vec3 normal = -normalize(cross(b-a,c-a));



            allNormals[def_index].p[  n  ] = vec4(getNormal(a), 0.0f);
            allNormals[def_index].p[n + 1] = vec4(getNormal(b), 0.0f);
            allNormals[def_index].p[n + 2] = vec4(getNormal(c), 0.0f);
        }
    }
}