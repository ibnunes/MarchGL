#ifndef CUBEMARCH_H
#define CUBEMARCH_H

#include <map>

#include <computeshader.h>
#include <shader_m.h>
#include <camera.h>

#include "marchingUtils.h"
using namespace std;


typedef struct {
	glm::vec3 p;
	float val;
} VOXEL;

typedef struct {
	glm::vec4 colorMesh;
	glm::vec4 colorLight;
	bool cameraLightSnap;
	glm::vec3 lightPos;

	bool gridOn;
	bool meshOn;
	bool skyboxOn;

	float refractVal;
	bool isRefract;
	bool isReflect;
	float ratioRefractReflect;

	vector<pair<float, float>> levels;
} SHADER_SETTINGS;


typedef struct {
	int renderMode;
	int threadAmount;
	float cubeSize;
	glm::vec3 gridSize;

	bool useWireframe;
} RENDER_SETTINGS;

typedef struct {
	std::string function;
} IMPLICIT_FUNCTION;

typedef struct {
	glm::vec4 p[12];
} TRIANGLES;

class cubeMarch {
	private:
	ComputeShader computeShader;					// Compute Shader
	Shader basicShader;								// Grid shader
	Shader shader;									// Mesh shader
	std::vector<glm::vec3> meshTriangles, normals;	// vertices for the triangles (mesh)
	std::vector<glm::vec3> gridPoints;
	unsigned VAO, meshVAO, gridVAO, gridLinesVAO, tessLevelsVBO;
	RENDER_SETTINGS renderSettings;
	string iFunction;
	glm::ivec3 size;
	int totalVertices = 0;
	int width, height = 0;
	unsigned computeVAO;

	public:
	cubeMarch(void);
	cubeMarch(RENDER_SETTINGS& rS);
	void setIFunction(IMPLICIT_FUNCTION& iF);

	//----- Marching Cubes Algorithm -----
	/*
	 * Checks if the point is inside or outside the sphere
	 */
	float getDensity(glm::vec3 p);

	/*
	 * Returns the interpolated point between the two vertices
	 */
	glm::vec3 getIntersVertice(glm::vec3 p1, glm::vec3 p2, float D1, float D2);
	void generateSingle(glm::vec3 currPoint);
	void generateCPU(void);
	void generateGPU(double iTime);
	void generate(double iTime);

	glm::vec3 getNormal(glm::vec3 p);

	//---- Mesh ----
	void createMesh(void);
	void drawMesh(Camera camera, glm::vec3 trans, SHADER_SETTINGS& settings, RENDER_SETTINGS& rSettings, double iTime);

	//---- Grid ----
	void createGrid();
	void drawGrid(Camera camera);
};

#endif