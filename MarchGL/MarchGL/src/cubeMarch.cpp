#include "cubeMarch.h"

#include "cparse/shunting-yard.h"
#include "cparse/builtin-features.h"

#include <random>
#include <glm/gtx/normal.hpp>


cubeMarch::cubeMarch(void) {
	cout << "\tInitializing CParse:";
	if (renderSettings.renderMode == 0)
		cparse_startup();
	cout << "\t[OK]\n" << endl;

	renderSettings.cubeSize = 0.1f;
	renderSettings.gridSize = glm::vec3(1.f);
	renderSettings.renderMode = 0;
	renderSettings.threadAmount = 1;

	size = glm::ivec3(
		renderSettings.gridSize.x * 2,
		renderSettings.gridSize.y * 2,
		renderSettings.gridSize.z * 2
	);


	std::cout << "LOADING Grid SHADER: ... " << std::endl << std::endl;
	basicShader = Shader("res/shaders/basicColorShader_vs.glsl", "res/shaders/basicColorShader_fs.glsl");
	if (!basicShader.wasSuccessful()) {
		std::cout << "Shader was not successful" << std::endl;
		std::cout << basicShader.getReport() << std::endl;
		return;
	}
	std::cout << std::endl << "[DONE]" << std::endl << std::endl;


	std::cout << "LOADING CubeMarch SHADER: ... " << std::endl << std::endl;
	shader = Shader(
		"res/shaders/mesh_vs.glsl",
		"res/shaders/mesh_fs.glsl",
		"res/shaders/mesh_tcs.glsl",
		"res/shaders/mesh_tes.glsl"
	);
	if (!shader.wasSuccessful()) {
		std::cout << "Shader was not successful" << std::endl;
		std::cout << shader.getReport() << std::endl;
		return;
	}
	std::cout << std::endl << "[DONE]" << std::endl << std::endl;


	std::cout << "LOADING CubeMarch COMPUTE SHADER: ... " << std::endl << std::endl;
	computeShader = ComputeShader("res/shaders/marchingcubes_cs.glsl");
	if (!computeShader.wasSuccessful()) {
		std::cout << "Shader was not successful" << std::endl;
		std::cout << computeShader.getReport() << std::endl;
		return;
	}
	std::cout << std::endl << "[DONE]" << std::endl;


	shader.use();
	shader.setInt("skybox", 0);

	createGrid();
}

cubeMarch::cubeMarch(RENDER_SETTINGS& rS) {
	if (renderSettings.renderMode == 0)
		cparse_startup();

	size = glm::ivec3(
		renderSettings.gridSize.x * 2,
		renderSettings.gridSize.y * 2,
		renderSettings.gridSize.z * 2
	);

	std::cout << "LOADING Grid SHADER: ... " << std::endl << std::endl;
	basicShader = Shader("res/shaders/basicColorShader_vs.glsl", "res/shaders/basicColorShader_fs.glsl");
	if (!basicShader.wasSuccessful()) {
		std::cout << "Shader was not successful" << std::endl;
		std::cout << basicShader.getReport() << std::endl;
		return;
	}
	std::cout << std::endl << "[DONE]" << std::endl << std::endl;


	std::cout << "LOADING CubeMarch SHADER: ... " << std::endl << std::endl;
	shader = Shader(
		"res/shaders/mesh_vs.glsl",
		"res/shaders/mesh_fs.glsl",
		"res/shaders/mesh_tcs.glsl",
		"res/shaders/mesh_tes.glsl"
	);
	bool shaderSuccess = shader.wasSuccessful();
	if (!shaderSuccess) {
		std::cout << "Shader was not successful" << std::endl;
		std::cout << shader.getReport() << std::endl;
		return;
	}
	std::cout << std::endl << "[DONE]" << std::endl << std::endl;


	std::cout << "LOADING CubeMarch COMPUTE SHADER: ... " << std::endl << std::endl;
	computeShader = ComputeShader("res/shaders/marchingcubes_cs.glsl");
	if (!computeShader.wasSuccessful()) {
		std::cout << "Shader was not successful" << std::endl;
		std::cout << computeShader.getReport() << std::endl;
		return;
	}
	std::cout << std::endl << "[DONE]" << std::endl;


	rS.gridSize.x = (float) ( (size_t) ( rS.gridSize.x ) );
	rS.gridSize.y = (float) ( (size_t) ( rS.gridSize.y ) );
	rS.gridSize.z = (float) ( (size_t) ( rS.gridSize.z ) );

	totalVertices = 0;
	renderSettings = rS;

	cout << "ALL SETTINGS" << endl;
	cout << "Cube Size: " << renderSettings.cubeSize << endl;
	cout << "Grid Size: " << renderSettings.gridSize.x << " " << renderSettings.gridSize.y << " " << renderSettings.gridSize.z << endl;
	cout << "Render Mode: " << renderSettings.renderMode << endl;
	cout << "Thread Amount: " << renderSettings.threadAmount << endl;

	createGrid();
}

void cubeMarch::generateSingle(glm::vec3 currPoint) {
	VOXEL voxel[8];
	int bin = 0b00000000;

	//std::cout << "Defining CurrPoint and Density: ... ";
	for (size_t i = 0; i < 8; i++) {
		voxel[i].p = glm::vec3(
			currPoint.x + renderSettings.cubeSize * tbl::vertexOffset[i][0],
			currPoint.y + renderSettings.cubeSize * tbl::vertexOffset[i][1],
			currPoint.z + renderSettings.cubeSize * tbl::vertexOffset[i][2]
		);

		voxel[i].val = getDensity(voxel[i].p);
		if (voxel[i].val < 0)
			bin |= 1 << i;
	}

	//std::cout << "[DONE]" << std::endl;
	if (bin == 0b00000000 || bin == 0b11111111)
		return;

	int edgeFlag = tbl::edgeTable[bin];

	glm::vec3 edgeVertices[12];


	if (edgeFlag & 1) edgeVertices[0] = getIntersVertice(voxel[0].p, voxel[1].p, voxel[0].val, voxel[1].val); //edge 0
	if (edgeFlag & 2) edgeVertices[1] = getIntersVertice(voxel[1].p, voxel[2].p, voxel[1].val, voxel[2].val); //edge 1
	if (edgeFlag & 4) edgeVertices[2] = getIntersVertice(voxel[2].p, voxel[3].p, voxel[2].val, voxel[3].val); //edge 2
	if (edgeFlag & 8) edgeVertices[3] = getIntersVertice(voxel[3].p, voxel[0].p, voxel[3].val, voxel[0].val); //edge 3 
	if (edgeFlag & 16) edgeVertices[4] = getIntersVertice(voxel[4].p, voxel[5].p, voxel[4].val, voxel[5].val); //edge 4
	if (edgeFlag & 32) edgeVertices[5] = getIntersVertice(voxel[5].p, voxel[6].p, voxel[5].val, voxel[6].val); //edge 5
	if (edgeFlag & 64) edgeVertices[6] = getIntersVertice(voxel[6].p, voxel[7].p, voxel[6].val, voxel[7].val); //edge 6
	if (edgeFlag & 128) edgeVertices[7] = getIntersVertice(voxel[7].p, voxel[4].p, voxel[7].val, voxel[4].val); //edge 7
	if (edgeFlag & 256) edgeVertices[8] = getIntersVertice(voxel[0].p, voxel[4].p, voxel[0].val, voxel[4].val); //edge 8
	if (edgeFlag & 512) edgeVertices[9] = getIntersVertice(voxel[1].p, voxel[5].p, voxel[1].val, voxel[5].val); //edge 9
	if (edgeFlag & 1024) edgeVertices[10] = getIntersVertice(voxel[2].p, voxel[6].p, voxel[2].val, voxel[6].val); //edge 10
	if (edgeFlag & 2048) edgeVertices[11] = getIntersVertice(voxel[3].p, voxel[7].p, voxel[3].val, voxel[7].val); //edge 11

	//std::cout << "Adding vertices and normals to buffers: ... ";
	for (size_t n = 0; tbl::triTable[bin][n] != -1; n += 3) {
		glm::vec3 a, b, c, normal;

		a = edgeVertices[tbl::triTable[bin][n]];
		b = edgeVertices[tbl::triTable[bin][n + 1]];
		c = edgeVertices[tbl::triTable[bin][n + 2]];

		meshTriangles.push_back(a);
		meshTriangles.push_back(b);
		meshTriangles.push_back(c);
		totalVertices += 3;

		normals.push_back(getNormal(a));
		normals.push_back(getNormal(b));
		normals.push_back(getNormal(c));
	}
	//std::cout << "[DONE]" << std::endl;
}


void cubeMarch::generateCPU(void) {
	for (float x = -renderSettings.gridSize.x; x < renderSettings.gridSize.x; x += renderSettings.cubeSize)
		for (float y = -renderSettings.gridSize.y; y < renderSettings.gridSize.y; y += renderSettings.cubeSize)
			for (float z = -renderSettings.gridSize.z; z < renderSettings.gridSize.z; z += renderSettings.cubeSize)
				generateSingle(glm::vec3(x, y, z));
}

//---------------------------------------------------------GPU------------------------------------

void cubeMarch::generateGPU(double iTime) {

	glm::ivec3 sizeGrid = ( renderSettings.gridSize * glm::vec3(2) ) / renderSettings.cubeSize;

	cout << "Going to change the CS" << endl;
	cout << "iFunction: " << iFunction << endl;

	int localSizes[] = { 1, 1, 1 };


	computeShader.recompileWithFunctions(iFunction);
	shader.recompileWithFunctions(iFunction);

	//changeComputeShader(sizeGrid, iFunction, localSizes);

	int globalSizes[] = {
		sizeGrid.x / localSizes[0],
		sizeGrid.y / localSizes[1],
		sizeGrid.z / localSizes[2]
	};

	//compile the CS
	//ComputeShader computeShader("res/shaders/computeShader.cs");

	cout << "Compiled with sucess" << endl;
	//computation sizes
	int maxWorkGroupSize[3], workGroupCounts[3], workGroupInvocations[3];
	//limit for the local size (inside the shader)
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSize[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSize[2]);

	//limit for the dispatch size
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workGroupCounts[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workGroupCounts[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workGroupCounts[2]);


	//printf("MAXIMUM WORK GROUP SIZE: %d %d %d\n", maxWorkGroupSize[0], maxWorkGroupSize[1], maxWorkGroupSize[2]);
	//printf("MAXIMUM WORK GROUP COUNT: %d %d %d\n", workGroupCounts[0], workGroupCounts[1], workGroupCounts[2]);


	if (sizeGrid.x > workGroupCounts[0] || sizeGrid.y > workGroupCounts[1] || sizeGrid.z > workGroupCounts[2]) {
		printf("ERROR: WORK GROUP COUNTS ARE TOO BIG\n");
		return;
	}




	//create buffers
	unsigned int gridBuffer, edgeTableVBO, triTableVBO; //inputs
	unsigned int triangles, normals; //outputs
	glGenBuffers(1, &gridBuffer);
	glGenBuffers(1, &edgeTableVBO);
	glGenBuffers(1, &triTableVBO);
	glGenBuffers(1, &triangles);
	glGenBuffers(1, &normals);


	//bind buffers
	//glBindBuffer(GL_SHADER_STORAGE_BUFFER, gridBuffer);
	//glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(vec4) * gridPoints.size(), &gridPoints[0], GL_STATIC_DRAW);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gridBuffer); //atrib=0

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, edgeTableVBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(tbl::edgeTable), tbl::edgeTable, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, edgeTableVBO); //atrib=1

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triTableVBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(tbl::triTable), tbl::triTable, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, triTableVBO); //atrib=2

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangles);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(TRIANGLES) * sizeGrid.x * sizeGrid.y * sizeGrid.z, NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, triangles); //atrib=3

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, normals);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(TRIANGLES) * sizeGrid.x * sizeGrid.y * sizeGrid.z, NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, normals); //atrib=4


	cout << "Starting compute shader" << endl;
	cout << "Total size " << sizeGrid.x * sizeGrid.y * sizeGrid.z << endl;
	cout << "Sizes: " << sizeGrid.x << " " << sizeGrid.y << " " << sizeGrid.z << endl;

	//compute shader
	computeShader.use();

	computeShader.setFloat("dist", renderSettings.cubeSize);
	computeShader.setInt("x_size", (int) ( renderSettings.gridSize.x ));
	computeShader.setInt("y_size", (int) ( renderSettings.gridSize.y ));
	computeShader.setInt("z_size", (int) ( renderSettings.gridSize.z ));
	computeShader.setInt("MX", sizeGrid.x);
	computeShader.setInt("MY", sizeGrid.y);
	computeShader.setInt("MZ", sizeGrid.z);
	computeShader.setFloat("iTime", iTime);

	cout << "Global sizes: " << globalSizes[0] << " " << globalSizes[1] << " " << globalSizes[2] << " " << endl;
	cout << "Local sizes: " << localSizes[0] << " " << localSizes[1] << " " << localSizes[2] << " " << endl;
	glDispatchCompute(globalSizes[0], globalSizes[1], globalSizes[2]);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	cout << "Computation Done" << endl;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	//VAO and vertices/normals buffer
	glGenVertexArrays(1, &meshVAO);
	glBindVertexArray(meshVAO);
	glBindBuffer(GL_ARRAY_BUFFER, triangles);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*) 0);
	glEnableVertexAttribArray(0);
	//glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind

	glBindBuffer(GL_ARRAY_BUFFER, normals);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*) 0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	//total vertices
	totalVertices = sizeGrid.x * sizeGrid.y * sizeGrid.z * 12;
}


void cubeMarch::generate(double iTime) {
	if (iFunction == "")
		return;

	meshTriangles.clear();
	normals.clear();

	std::cout << "Running Marching Cube" << std::endl;


	if (renderSettings.renderMode == 0) {
		generateCPU();
		createMesh();
	} else {
		generateGPU(iTime);
	}

	std::cout << std::endl << "[DONE]" << std::endl << std::endl;
}

//------------------------------------------------------------------------------------------------


void cubeMarch::setIFunction(IMPLICIT_FUNCTION& iF) { iFunction = iF.function; }

#pragma region Grid
//----grid----
void cubeMarch::createGrid() {
	std::cout << "Create Grid: ... " << std::endl;

	gridPoints.clear();

	glm::vec3 divided = renderSettings.gridSize * glm::vec3(2.f) * renderSettings.cubeSize;

	cout << divided.x << " " << divided.y << " " << divided.z << endl;

	for (float x = -renderSettings.gridSize.x; x < renderSettings.gridSize.x + renderSettings.cubeSize; x += renderSettings.cubeSize)
		gridPoints.push_back(glm::vec3(x, 0, 0));

	for (float y = -renderSettings.gridSize.y; y < renderSettings.gridSize.y + renderSettings.cubeSize; y += renderSettings.cubeSize)
		gridPoints.push_back(glm::vec3(0, y, 0));

	for (float z = -renderSettings.gridSize.z; z < renderSettings.gridSize.z + renderSettings.cubeSize; z += renderSettings.cubeSize)
		gridPoints.push_back(glm::vec3(0, 0, z));

	unsigned VBO;
	glGenVertexArrays(1, &gridVAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(gridVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, gridPoints.size() * sizeof(glm::vec3), &gridPoints[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*) 0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec3), (void*) ( sizeof(glm::vec3) ));

	std::cout << "[DONE]" << std::endl;
}

void cubeMarch::drawGrid(Camera camera) {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;

	constexpr float ratio = (float) 1920 / (float) 1080;
	projection = glm::perspective(glm::radians(camera.Zoom), ratio, 0.1f, 100.0f);
	view = camera.GetViewMatrix();
	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
	model = glm::translate(model, glm::vec3(0.f));

	basicShader.use();
	basicShader.setMat4("projection", projection);
	basicShader.setMat4("view", view);
	basicShader.setMat4("model", model);

	glBindVertexArray(gridVAO);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glPointSize(5.0);
	glDrawArrays(GL_POINTS, 0, gridPoints.size());
}
#pragma endregion

//-----marching cubes algorithm-----

//returns the density of the point p
float cubeMarch::getDensity(glm::vec3 p) {
	double result = 0.f;
	TokenMap vars;

	vars["x"] = p.x;
	vars["y"] = p.y;
	vars["z"] = p.z;

	try {
		packToken calc = calculator::calculate(iFunction.c_str(), vars);

		result = calc.asDouble();
	} catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}

	return result;
}

glm::vec3 cubeMarch::getNormal(glm::vec3 p) {
	glm::vec2 e(0.001f, 0.0f);

	return glm::normalize(
		getDensity(p) - glm::vec3(
			getDensity(p - glm::vec3(e.x, e.y, e.y)),
			getDensity(p - glm::vec3(e.y, e.x, e.y)),
			getDensity(p - glm::vec3(e.y, e.y, e.x))
		)
	);
}

//returns the middle point between the two vertices
glm::vec3 cubeMarch::getIntersVertice(glm::vec3 p1, glm::vec3 p2, float D1, float D2) {
	if (abs(D1) < 0.00001)
		return p1;

	if (abs(D2) < 0.00001)
		return p2;

	if (abs(D1 - D2) < 0.00001)
		return p1;

	float t = -D1 / ( D2 - D1 );

	return ( 1 - t ) * p1 + t * p2;
}

//----mesh----
void cubeMarch::createMesh() {
	std::cout << "Create Mesh: ... ";

	if (meshTriangles.empty() || normals.empty()) {
		std::cout << "Buffers are empty. Can't proceed action." << std::endl;
		return;
	}

	unsigned VBO, CBO;
	glGenVertexArrays(1, &meshVAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &CBO);
	glBindVertexArray(meshVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, meshTriangles.size() * sizeof(glm::vec3), &meshTriangles[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);

	glBindBuffer(GL_ARRAY_BUFFER, CBO);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);

	glPatchParameteri(GL_PATCH_VERTICES, 3);

	glBindVertexArray(0);

	std::cout << "[DONE]" << std::endl;
}

void cubeMarch::drawMesh(Camera camera, glm::vec3 trans, SHADER_SETTINGS& settings, RENDER_SETTINGS& rSettings, double iTime) {
	//std::cout << "Draw Mesh: ...";

	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;

	constexpr float ratio = (float) 1920 / (float) 1080;
	projection = glm::perspective(glm::radians(camera.Zoom), ratio, 0.1f, 100.0f);
	view = camera.GetViewMatrix();
	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
	model = glm::translate(model, trans);

	size_t levelIndex = 0;
	vector<glm::vec2> temp;
	for (auto it = settings.levels.begin(); it != settings.levels.end(); ++it) {
		//shader.setVec2("tessLevels[" + to_string(levelIndex) + "]", it->first, it->second);
		temp.push_back({ it->first, it->second });
		levelIndex++;
	}

	glGenBuffers(1, &tessLevelsVBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, tessLevelsVBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec2) * temp.size(), temp.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, tessLevelsVBO); //atrib=0

	shader.use();

	shader.setMat4("projection", projection);
	shader.setMat4("view", view);
	shader.setMat4("model", model);

	shader.setVec3("camPos", camera.Position);
	shader.setVec3("camDir", camera.Front);

	shader.setVec4("objectColor", settings.colorMesh);
	shader.setFloat("iTime", iTime);

	shader.setVec4("lamp.lightColor", settings.colorLight);
	shader.setVec3("lamp.lightPos", ( settings.cameraLightSnap ) ? camera.Position : settings.lightPos);
	shader.setVec3("lamp.viewPos", camera.Position);

	shader.setBool("isReflect", settings.isReflect);
	shader.setBool("isRefract", settings.isRefract);
	shader.setFloat("refractVal", settings.refractVal);
	shader.setFloat("ratioRefractReflect", settings.ratioRefractReflect);

	shader.setInt("tessLevelSize", levelIndex);
	shader.setFloat("voxelDetail", renderSettings.cubeSize);

	glBindVertexArray(meshVAO);

	( rSettings.useWireframe )
		? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)
		: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDrawArrays(GL_PATCHES, 0, totalVertices);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	//glDrawArrays(GL_TRIANGLES, 0, 3);

	//std::cout << "[DONE]" << "\n";
}