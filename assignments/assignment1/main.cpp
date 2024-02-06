#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/cameraController.h>
#include <ew/transform.h>
#include <ew/texture.h>

#include <nb/framebuffer.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

const int numShaders = 5;
int blurAmount = 2;

ew::Camera camera;
ew::CameraController cameraController;

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

enum PPShaders {
	noPP, invertPP, boxBlurPP
}curShader;

void setPPShader(std::vector<ew::Shader> shaders, PPShaders shader);

int main() {
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	// OpenGL variables
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); // Back face culling
	glEnable(GL_DEPTH_TEST); // Depth testing

	// Dummy VAO
	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);

	// Shaders
	ew::Shader lit = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader invert = ew::Shader("assets/postprocessing.vert", "assets/invert.frag");
	ew::Shader boxblur = ew::Shader("assets/postprocessing.vert", "assets/boxblur.frag");
	// Create vector of shaders
	std::vector<ew::Shader> shaders;
	shaders.reserve(numShaders);
	shaders.push_back(lit);
	shaders.push_back(invert);
	shaders.push_back(boxblur);
	curShader = PPShaders::boxBlurPP;

	// Framebuffers
	nb::Framebuffer framebuffer = nb::createFramebuffer(screenWidth, screenHeight, GL_RGB16F);
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("\nFramebuffer incomplete %d\n", fboStatus);
	}

	// Textures
	GLuint buildingTexture = ew::loadTexture("assets/Building_Color.png");
	GLuint normalTexture = ew::loadTexture("assets/Building_NormalGL.png");

	// Models
	ew::Model monkeyModel = ew::Model("assets/suzanne.fbx");
	

	// Transforms
	ew::Transform monkeyTransform;

	// Camera
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); // Look at center of the scene
	camera.fov = 60.0f; // Vertical field of view, in degrees
	camera.aspectRatio = (float)screenWidth / screenHeight;


	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		//RENDER
		// Bind to framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		
		glClearColor(0.6f,0.8f,0.92f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// MY CODE HERE

		// Binding textures
		glBindTextureUnit(0, buildingTexture);
		glBindTextureUnit(1, normalTexture);

		// Camera movement
		cameraController.move(window, &camera, deltaTime);

		// Rotate model around Y axis
		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		lit.use();
		lit.setMat4("_Model", monkeyTransform.modelMatrix());
		lit.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		lit.setInt("_MainTex", 0);
		lit.setInt("_NormalTex", 1);
		lit.setVec3("_EyePos", camera.position);
		lit.setFloat("_Material.Ka", material.Ka);
		lit.setFloat("_Material.Kd", material.Kd);
		lit.setFloat("_Material.Ks", material.Ks);
		lit.setFloat("_Material.Shininess", material.Shininess);

		monkeyModel.draw(); // Draws monkey model using current shader

		// Bind back to front buffer (0)
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Set post-processing shader
		setPPShader(shaders, curShader);

		// Set variables based on chosen post-processing shader
		switch (curShader) {
		case PPShaders::noPP:
			break;
		case PPShaders::invertPP:
			invert.setInt("_ColorBuffer", 0);
			break;
		case PPShaders::boxBlurPP:
			boxblur.setInt("_ColorBuffer", 0);
			boxblur.setInt("_BlurAmount", blurAmount);
			break;
		}

		glBindTextureUnit(0, framebuffer.colorBuffer[0]);
		glBindVertexArray(dummyVAO);

		// Draw fullscreen quad
		glDrawArrays(GL_TRIANGLES, 0, 6);

		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0.0, 0.0, 5.0f);
	camera->target = glm::vec3(0.0);
	controller->yaw = controller->pitch = 0.0;
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::Button("Reset Camera")) {
		resetCamera(&camera, &cameraController);
	}

	// Material GUI
	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}

	// Shaders list GUI
	const char* listbox_shaders[] = { "Invert", "Box Blur" };
	static int listbox_current = 1;
	ImGui::ListBox("Shader", &listbox_current, listbox_shaders, IM_ARRAYSIZE(listbox_shaders), 4);

	// Set shader based on list item selected
	curShader = static_cast<PPShaders>(listbox_current + 1);

	// If box blur shader, show slider for blur amount
	if (curShader == PPShaders::boxBlurPP) {
		ImGui::SliderInt("Blur Amount", &blurAmount, 0, 25);
	}

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

/// <summary>
/// Sets PP shader based on enumerator in shaders vector
/// </summary>
/// <param name="shaders">Shaders vector</param>
/// <param name="shader">Shader enumerator</param>
void setPPShader(std::vector<ew::Shader> shaders, PPShaders shader) {
	shaders[shader].use();
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
	camera.aspectRatio = (float)width / height;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

