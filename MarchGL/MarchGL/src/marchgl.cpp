#include "marchgl.h"

#include "mutils.h"

#ifdef __linux__
#include <filesystem>
namespace fs = std::filesystem;
#endif

extern "C" {
	// AMD GPU code should take this code path
	__declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
	// Nvidia takes this code path
	__declspec( dllexport ) unsigned long NvOptimusEnablement = 0x00000001;
}


static void PushStyleCompact() {
	ImGuiStyle& style = ImGui::GetStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, (float) (int) ( style.FramePadding.y * 0.60f )));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, (float) (int) ( style.ItemSpacing.y * 0.60f )));
}

static void PopStyleCompact() {
	ImGui::PopStyleVar(2);
}

static void sortVPair(vector<pair<float, float>>& v) {
	size_t i, j;
	bool swapped;
	for (i = 0; i < v.size() - 1; i++) {
		swapped = false;
		for (j = 0; j < v.size() - i - 1; j++) {
			if (v[j].first > v[j + 1].first) {
				auto tmp = v[j];
				v[j].first = v[j + 1].first;
				v[j].second = v[j + 1].second;

				swap(v[j], v[j + 1]);
				swapped = true;
			}
		}

		// If no two elements were swapped by inner loop,
		// then break
		if (swapped == false)
			break;
	}
}

namespace callback {
	MarchGL* instance = nullptr;

	void bindInstance(MarchGL* i) {
		instance = i;
	}

	MarchGL* getInstance(void) {
		return instance;
	}

	void framebufferSize(GLFWwindow* window, int width, int height) {
		glViewport(0, 0, width, height);
	}

	void mouseBtn(GLFWwindow* window, int button, int action, int mods) {
		bool leftBtnDown = false;
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (GLFW_PRESS == action)
				leftBtnDown = true;
			else if (GLFW_RELEASE == action)
				leftBtnDown = false;
		}

		bool rightBtnDown = false;
		if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			if (GLFW_PRESS == action)
				rightBtnDown = true;
			else if (GLFW_RELEASE == action)
				rightBtnDown = false;
		}

		if (rightBtnDown)
			getInstance()->switchGUIMode(!getInstance()->getGUIMode());


		if (!leftBtnDown) {
			return;
		}

		double x, y;
		glfwGetCursorPos(getInstance()->getWindow(), &x, &y);
	}

	void mouse(GLFWwindow* window, double xpos, double ypos) {
		vec2 pos(xpos, ypos);

		if (getInstance()->getMouseData().first) {
			getInstance()->setMouseData(pos, false);
		}

		vec2 offsets(
			xpos - getInstance()->getMouseData().lastPos.x,
			getInstance()->getMouseData().lastPos.y - ypos
		);

		getInstance()->setMouseData(pos, false);
		if (!getInstance()->getGUIMode())
			getInstance()->getCamera().ProcessMouseMovement(offsets.x, offsets.y);

	}

	void mouseScroll(GLFWwindow* window, double xoffset, double yoffset) {
		getInstance()->getCamera().ProcessMouseScroll(yoffset);
	}
}

const char* MarchGLException::what(void) const throw ( ) {
	return message.c_str();
}

bool MarchGL::initializeGLFW(unsigned int width, unsigned int height, const char* title) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!window)
		return false;

	glfwMakeContextCurrent(window);

	setFrameBufferSizeCallback(callback::framebufferSize);
	setMouseButtonCallback(callback::mouseBtn);
	setCursorPositionCallback(callback::mouse);
	setScrollCallback(callback::mouseScroll);


	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return true;
}

int MarchGL::initializeGLAD(void) {
	return gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
}

void MarchGL::initializeImGUI(void) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	io = ImGui::GetIO(); (void) io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(getWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 450");
}

void MarchGL::terminateImGUI(void) {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

Camera& MarchGL::getCamera(void) {
	return camera;
}

mouse_data MarchGL::getMouseData(void) {
	return mouse;
}

void MarchGL::setMouseData(vec2 pos, bool first) {
	mouse = { pos, first };
}

bool MarchGL::getGUIMode(void) {
	return guiMode;
}

void MarchGL::switchGUIMode(bool mode) {
	guiMode = mode;
}

void MarchGL::setFrameBufferSizeCallback(GLFWframebuffersizefun callback) {
	glfwSetFramebufferSizeCallback(window, callback);
}

void MarchGL::setMouseButtonCallback(GLFWmousebuttonfun callback) {
	glfwSetMouseButtonCallback(window, callback);
}

void MarchGL::setCursorPositionCallback(GLFWcursorposfun callback) {
	glfwSetCursorPosCallback(window, callback);
}

void MarchGL::setScrollCallback(GLFWscrollfun callback) {
	glfwSetScrollCallback(window, callback);
}

bool MarchGL::launchSuccessful(void) {
	return success;
}

string MarchGL::getAppDir(void) {
	return appDir;
}

string MarchGL::getAppName(void) {
	return appName;
}

string MarchGL::getApp(void) {
	return appDir + slash + appName;
}

string MarchGL::getShaderDir(void) {
	return shaderDir;
}

GLFWwindow* MarchGL::getWindow(void) {
	return window;
}

unsigned MarchGL::getFPS(void) {
	return appFPS;
}

void MarchGL::setFPS(unsigned fps) {
	appFPS = fps;
}

//----------------------------------------------------main--------------------------------------------

MarchGL::MarchGL(void) { }

MarchGL::MarchGL(Arguments args) {
	try {
		cout << "Launching MarchGL" << endl;

		cout << "\tSetting Global Variables: " << endl;
		if (strcmp(args.rMode, "CPU") == 0) {
			rmode = CPU;
			cout << "\t| Render Mode: CPU" << endl;
		} else if (strcmp(args.rMode, "GPU") == 0) {
			rmode = GPU;
			cout << "\t| Render Mode: GPU" << endl;
		}

		rS.threadAmount = args.threads;
		cout << "\t| Threads: " << rS.threadAmount << endl;

		scr_height = args.height;
		scr_width = args.width;

		sS.colorLight = vec4(1.f);
		sS.colorMesh = vec4(this->SURF_DEFAULT_COLOR, 1.f);
		sS.gridOn = true;
		sS.meshOn = true;
		sS.skyboxOn = true;

		sS.refractVal = 0.75;
		sS.ratioRefractReflect = 0.3;

		rS.gridSize = vec3(1.f);
		rS.cubeSize = 0.1f;

		cout << "\t| Window Resolution: " << scr_width << "x" << scr_height << endl;
		cout << "\t[OK]\n" << endl;


		cout << "\tSetting Relevant Directories: " << endl;
#ifdef __linux
		#warning current_path() may not provide the actual app path!Consider using it in conjunction with argv[0] or similar.
			fs::path appPath(fs::current_path());
#else
		filesystem::path appPath(filesys::getAppPath());
#endif

		appDir = appPath.parent_path().string();
		cout << "\t| App Dir: " << appDir << endl;

		appName = appPath.filename().string();
		cout << "\t| App Name: " << appName << endl;

		shaderDir = appDir + slash + "shaders";
		cout << "\t| Shaders: " << shaderDir << endl;

		resDir = appDir + slash + "res";
		cout << "\t| Resources: " << resDir << endl;
		cout << "\t[OK]\n" << endl;


		cout << "\tInitializing GLFW & GLAD:" << endl;
		setMouseData(vec2(scr_width / 2.f, scr_width / 2.f), true);

		if (!initializeGLFW(scr_width, scr_height, "MarchGL - IsoSurfaces with Marching Cubes"))
			throw MarchGLException("Failed to create the Window");
		cout << "\t| GLFW: Success" << endl;
		if (!initializeGLAD())
			throw MarchGLException("Failed to initialize GLAD");
		cout << "\t| GLAD: Success" << endl;
		cout << "\t[OK]\n" << endl;

		cout << "\tLoading Starting Shaders: " << endl;

		cout << "\t[OK]\n" << endl;

		cout << "[OK]" << endl;
		cout << "Done.\n" << endl;

		marchingCubes = new cubeMarch();
		cubemap = new Cubemap();
		cout << ( "res" + slash + "textures" + slash + "mgl_logo.bmp" ).c_str() << endl;
		cutscene = new cutScene(( "res" + slash + "textures" + slash + "mgl_logo.bmp" ).c_str());

	} catch (const MarchGLException& e) {
		cerr << "[Error]: " << e.what() << endl;
		cout << "Abort Launch!" << endl;

		success = false;
	}

	success = true;
}

void MarchGL::renderCutScene(void) {
	//std::cout << "CUTSCENE" << std::endl;
	cutscene->drawMesh(currentFrame, scr_width, scr_height);
}

void MarchGL::main(void) {
	double prevTime = 0.0;
	double crntTime = 0.0;
	double timeDiff;

	unsigned counter = 0;

	initializeImGUI();

	//cubeMarch sphere = cubeMarch("sphere");
	//cubeMarch s = cubeMarch();
	//s.generateGPU();
	while (!glfwWindowShouldClose(getWindow())) {
		crntTime = glfwGetTime();
		timeDiff = crntTime - prevTime;
		counter++;

		if (timeDiff >= 1.0 / 30.0) {
			setFPS(( 1.0 / timeDiff ) * counter);

			prevTime = crntTime;
			counter = 0;
		}
		refresh();

		renderUI();

		if (crntTime < 6.0f) {
			renderCutScene();
		} else if (cutSceneOn) {
			cutSceneOn = false;

			cutscene->~cutScene();
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

action MarchGL::processInput(void) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (!guiMode) {
		camera.MovementSpeed = ( glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS )
			? 6.f
			: 3.f;

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			camera.ProcessKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camera.ProcessKeyboard(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camera.ProcessKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camera.ProcessKeyboard(RIGHT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			camera.ProcessKeyboard(DOWN, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			camera.ProcessKeyboard(UP, deltaTime);

		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
			return action::CAMERA_RESET;
	}

	glfwSetInputMode(window, GLFW_CURSOR, guiMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

	return action::NO_ACTION;
}

void MarchGL::newFrameUI(void) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void MarchGL::renderUI(void) {
	{
		ImGui::Begin("Shader Settings");

		ImGui::BeginGroup();
		ImGui::Text("Mesh Color");
		ImGui::ColorEdit3("", &sS.colorMesh.x);
		ImGui::EndGroup();

		ImGui::Spacing();

		ImGui::BeginGroup();
		ImGui::Text("Light Color");
		ImGui::ColorEdit3(" ", &sS.colorLight.x);
		ImGui::EndGroup();

		ImGui::Spacing();

		float maxVal = glm::max(glm::max(rS.gridSize.x, rS.gridSize.y), rS.gridSize.z);

		ImGui::BeginGroup();
		ImGui::Text("Light Position");
		ImGui::SliderFloat3(" ", &sS.lightPos.x, -maxVal, maxVal);
		ImGui::Checkbox("Snap light to camera", &sS.cameraLightSnap);
		ImGui::EndGroup();

		ImGui::Spacing();

		ImGui::BeginGroup();
		ImGui::Checkbox("Show Mesh", &sS.meshOn);
		ImGui::SameLine();
		ImGui::Checkbox("Show Grid", &sS.gridOn);
		ImGui::SameLine();
		ImGui::Checkbox("Show Skybox", &sS.skyboxOn);
		ImGui::EndGroup();

		ImGui::Spacing();

		ImGui::BeginGroup();
		ImGui::Checkbox("Reflect", &sS.isReflect);
		ImGui::SameLine();
		ImGui::Checkbox("Refract", &sS.isRefract);

		ImGui::DragFloat("Refract Value", &sS.refractVal);

		ImGui::SliderFloat("Ratio", &sS.ratioRefractReflect, 0.f, 1.f);
		ImGui::EndGroup();

		ImGui::End();
	}

	{
		ImGui::Begin("Render Settings");

		ImGui::RadioButton("CPU", &rS.renderMode, 0);
		ImGui::SameLine();
		ImGui::RadioButton("GPU", &rS.renderMode, 1);

		ImGui::BeginDisabled(rS.renderMode != 0);
		ImGui::InputInt("Thread Amount", &rS.threadAmount);
		ImGui::EndDisabled();

		ImGui::InputFloat("Voxel Size", &rS.cubeSize, 0.00F, 0.00F, "%.2f");
		ImGui::SliderFloat3("Grid Size", &rS.gridSize.x, 0.f, 10.f);

		ImGui::Checkbox("Wireframe Mode", &rS.useWireframe);

		if (ImGui::Button("Apply"))
			marchingCubes = new cubeMarch(rS);


		ImGui::End();
	}

	{
		ImGui::Begin("Implicit Functions");

		ImGui::Text("Function (f = 0)");
		ImGui::InputText(" ", &iF.function);

		if (ImGui::Button("Render")) {
			marchingCubes->setIFunction(iF);
			marchingCubes->generate(glfwGetTime());
		}

		ImGui::End();
	}

	{
		if (ImGui::Begin("Tesselation Shader Properties")) {
			if (sS.levels.size() == 0)
				sS.levels.push_back({ 0.f, 1.f });

			static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
			if (ImGui::BeginTable("table_scrolly", 2, flags, ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
				for (size_t row = 0; row < sS.levels.size(); row++) {
					ImGui::TableNextRow();
					{
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-FLT_MIN);
						string id = format("##DistanceDrag{}", ( row ));
						ImGui::DragFloat(id.c_str(), &( sS.levels[row].first ), 1.f, 0.01f, 0.f, "Distance > %.1f");
					}
					{
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-FLT_MIN);
						string id = format("##LevelDrag{}", ( row ));
						ImGui::DragFloat(id.c_str(), &( sS.levels[row].second ), 1.f, 1.f, 0.f, "Level : %.1f");
					}
				}

				ImGui::EndTable();
			}

			if (ImGui::Button("+")) {
				sS.levels.push_back({ 0.f, 1.f });
			}

			ImGui::SameLine();

			ImGui::BeginDisabled(sS.levels.size() <= 1 && sS.levels[0] == pair{ 0.f, 1.f });
			if (ImGui::Button("-"))
				sS.levels.pop_back();
			ImGui::EndDisabled();

			ImGui::End();
		}
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void MarchGL::refresh(void) {
	callback::bindInstance(this);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glClearColor(bgColor.x, bgColor.y, bgColor.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	newFrameUI();

	{
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
	}

	switch (processInput()) {
		case action::CAMERA_RESET:
			camera.Position = vec3(0.f, 1.f, 0.f);
			cout << "Camera Reset." << endl;
			break;
		case action::CHANGE_COLOR:
			break;
		case action::CHANGE_SHADER:
		case action::SHADER_RELOAD:
			break;
		case action::NO_ACTION:
		default:
			break;
	}

	if (sS.skyboxOn)
		cubemap->draw(camera);

	if (sS.gridOn)
		marchingCubes->drawGrid(camera);

	if (sS.meshOn)
		marchingCubes->drawMesh(camera, vec3(0.0f), sS, rS, currentFrame);
}

void MarchGL::terminate(void) {
	glfwTerminate();
	terminateImGUI();
}

MarchGL::~MarchGL(void) {
	free(marchingCubes);
	free(cubemap);
	free(cutscene);

	terminate();
}
