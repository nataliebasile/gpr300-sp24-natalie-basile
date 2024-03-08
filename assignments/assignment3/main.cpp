#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/cameraController.h>
#include <ew/transform.h>
#include <ew/texture.h>
#include <ew/procGen.h>

#include <nb/framebuffer.h>
#include <nb/shadowmap.h>
#include <nb/light.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

// Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

// Shaders
std::vector<ew::Shader> shaders;
const int numShaders = 5;
int blurAmount = 2;

// Framebuffers
nb::Framebuffer framebuffer;
nb::Framebuffer gBuffer;
nb::ShadowMap shadowMap;

// Camera
ew::Camera camera;
ew::CameraController cameraController;
ew::Camera shadowCamera;

// Shadow
float shadowCamDistance = 10;
float shadowCamOrthoHeight = 3;
float minBias = 0.005, maxBias = 0.015;

struct PointLight {
	glm::vec3 position;
	float radius;
	glm::vec4 color;
};

// Lighting
glm::vec3 lightDir{ -0.5, -1, -0.5 }, lightCol{ 1, 1, 1 };
nb::Light light = nb::createLight(lightDir, lightCol);
const int MAX_POINT_LIGHTS = 64;
PointLight pointLights[MAX_POINT_LIGHTS]{
	PointLight{ {0, 5, 0}, 5, {0, 0.5, 0.5, 1} },
	PointLight{ {5, 5, 5}, 5, {0.5, 0, 0.5, 1} },
	PointLight{ {5, 5, -5}, 5, {0.5, 0.5, 0, 1} },
	PointLight{ {-5, 5, -5}, 5, {0, 0.5, 0, 1} },
	PointLight{ {-5, 5, 5}, 5, {0.5, 0, 0, 1} }
};

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
	glCullFace(GL_FRONT); // Back face culling
	glEnable(GL_DEPTH_TEST); // Depth testing

	// Dummy VAO
	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);

	// Shaders
	ew::Shader lit = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader defLit = ew::Shader("assets/postprocessing.vert", "assets/deferredLit.frag");
	ew::Shader gBufferShader = ew::Shader("assets/geometryPass.vert", "assets/geometryPass.frag");
	ew::Shader depthOnly = ew::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");
	ew::Shader noPP = ew::Shader("assets/postprocessing.vert", "assets/nopostprocessing.frag");
	ew::Shader invert = ew::Shader("assets/postprocessing.vert", "assets/invert.frag");
	ew::Shader boxblur = ew::Shader("assets/postprocessing.vert", "assets/boxblur.frag");

	// Create vector of post processing shaders
	shaders.reserve(numShaders);
	shaders.push_back(noPP);
	shaders.push_back(invert);
	shaders.push_back(boxblur);
	curShader = PPShaders::noPP;

	// Framebuffers
	framebuffer = nb::createFramebuffer(screenWidth, screenHeight, GL_RGB16F);
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("\nFramebuffer incomplete %d\n", fboStatus);
	}

	// Gbuffers
	gBuffer = nb::createGBuffer(screenWidth, screenHeight);

	// Shadowmap
	shadowMap = nb::createShadowMap(screenWidth, screenHeight);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("\nShadowmap incomplete %d\n", fboStatus);
	}

	// Textures
	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");
	GLuint defaultNormalTexture = ew::loadTexture("assets/Default_normal.jpg");
	GLuint buildingTexture = ew::loadTexture("assets/Building_Color.png");
	GLuint normalTexture = ew::loadTexture("assets/Building_NormalGL.png");

	// Models & Meshes
	ew::Model monkeyModel = ew::Model("assets/suzanne.fbx");
	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(10, 10, 5));

	// Transforms
	ew::Transform monkeyTransform;
	ew::Transform planeTransform;
	planeTransform.position = glm::vec3(0, -2, 0);

	// Camera
	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); // Look at center of the scene
	camera.fov = 60.0f; // Vertical field of view, in degrees
	camera.aspectRatio = (float)screenWidth / screenHeight;

	// Shadow camera
	shadowCamera.target = glm::vec3(0.0f, 0.0f, 0.0f); // Look at center of the scene
	shadowCamera.position = shadowCamera.target - light.direction * shadowCamDistance; // HAS TO BE A FLOAT OR WILL BREAK???
	shadowCamera.orthographic = true;
	shadowCamera.orthoHeight = shadowCamOrthoHeight;
	shadowCamera.aspectRatio = 1;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		// === GEOMETRY PASS ===
		{
			// Bind to Gbuffer
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.fbo);
			glViewport(0, 0, gBuffer.width, gBuffer.height);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glCullFace(GL_BACK); // Front face culling

			glBindTextureUnit(0, defaultNormalTexture);
			glBindTextureUnit(1, brickTexture);
			glBindTextureUnit(2, buildingTexture);
			glBindTextureUnit(3, normalTexture);

			gBufferShader.use();
			gBufferShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());

			gBufferShader.setInt("_MainTex", 2);
			gBufferShader.setInt("_NormalTex", 3);
			gBufferShader.setMat4("_Model", monkeyTransform.modelMatrix());
			monkeyModel.draw();

			gBufferShader.setInt("_MainTex", 1);
			gBufferShader.setInt("_NormalTex", 0);
			gBufferShader.setMat4("_Model", planeTransform.modelMatrix());
			planeMesh.draw();
		}

		// === SHADOWMAP PASS ===
		{
			// Bind to shadow framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.sfbo);
			glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glCullFace(GL_FRONT); // Front face culling

			depthOnly.use();
			depthOnly.setMat4("_ViewProjection", shadowCamera.projectionMatrix() * shadowCamera.viewMatrix());

			depthOnly.setMat4("_Model", monkeyTransform.modelMatrix());
			monkeyModel.draw();

			depthOnly.setMat4("_Model", planeTransform.modelMatrix());
			planeMesh.draw();
		}

		// === LIGHTING PASS ===
		{
			// Bind to framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
			glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glCullFace(GL_BACK); // Back face culling

			// Binding textures
			glBindTextureUnit(0, gBuffer.colorBuffers[0]);
			glBindTextureUnit(1, gBuffer.colorBuffers[1]);
			glBindTextureUnit(2, gBuffer.colorBuffers[2]);
			glBindTextureUnit(3, shadowMap.depthTexture);

			// Camera movement
			cameraController.move(window, &camera, deltaTime);

			// Rotate model around Y axis
			monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

			defLit.use();
			defLit.setMat4("_LightViewProjection", shadowCamera.projectionMatrix() * shadowCamera.viewMatrix());
			defLit.setFloat("_MinBias", minBias);
			defLit.setFloat("_MaxBias", maxBias);

			defLit.setInt("_gPositions", 0);
			defLit.setInt("_gNormals", 1);
			defLit.setInt("_gAlbedo", 2);
			defLit.setInt("_ShadowMap", 3);

			defLit.setVec3("_EyePos", camera.position);
			defLit.setVec3("_LightDirection", light.direction);
			defLit.setVec3("_LightColor", light.color);
			defLit.setFloat("_Material.Ka", material.Ka);
			defLit.setFloat("_Material.Kd", material.Kd);
			defLit.setFloat("_Material.Ks", material.Ks);
			defLit.setFloat("_Material.Shininess", material.Shininess);

			glBindVertexArray(dummyVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// === POST-PROCESSING PASS ===
		{
			glBindTextureUnit(0, framebuffer.fbo);
			// Bind back to front buffer (0)
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Set post-processing shader
			setPPShader(shaders, curShader);

			// Set variables based on chosen post-processing shader
			switch (curShader) {
			case PPShaders::noPP:
				noPP.setInt("_ColorBuffer", 0);
				break;
			case PPShaders::invertPP:
				invert.setInt("_ColorBuffer", 0);
				break;
			case PPShaders::boxBlurPP:
				boxblur.setInt("_ColorBuffer", 0);
				boxblur.setInt("_BlurAmount", blurAmount);
				break;
			}
		}

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

	// Light GUI
	if (ImGui::CollapsingHeader("Light")) {
		if (ImGui::ColorEdit3("Color", &lightCol[0]))
		{
			light.changeColor(lightCol);
		}
		if (ImGui::DragFloat3("Direction", &lightDir[0],0.05)) {
			light.changeDirection(lightDir);
			shadowCamera.position = shadowCamera.target - light.direction * shadowCamDistance;
		}
	}

	// Shadowmap camera GUI
	if (ImGui::CollapsingHeader("Shadowmap Camera")) {
		if (ImGui::SliderFloat("Distance", &shadowCamDistance, 0.0f, 50.f)) {
			shadowCamera.position = shadowCamera.target - light.direction * shadowCamDistance;
		}
		if (ImGui::SliderFloat("Ortho Height", &shadowCamOrthoHeight, 0.0f, 50.0f)) {
			shadowCamera.orthoHeight = shadowCamOrthoHeight;
		}
		ImGui::SliderFloat("Min Bias", &minBias, 0.0f, 0.05f);
		ImGui::SliderFloat("Max Bias", &maxBias, 0.0f, 0.5f);
	}

	// Shaders list GUI
	const char* listbox_shaders[] = { "No Post Processing", "Invert", "Box Blur" };
	static int listbox_current = 0;
	if (ImGui::CollapsingHeader("Post Processing Shaders")) {
		ImGui::ListBox("Shader", &listbox_current, listbox_shaders, IM_ARRAYSIZE(listbox_shaders), 4);
	}

	// Set shader based on list item selected
	curShader = static_cast<PPShaders>(listbox_current);

	// If box blur shader, show slider for blur amount
	if (curShader == PPShaders::boxBlurPP) {
		ImGui::SliderInt("Blur Amount", &blurAmount, 0, 25);
	}

	ImGui::End();

	// Shadow map debug render
	ImGui::Begin("Shadow Map");
	ImGui::BeginChild("Shadow Map");

	ImVec2 windowSize = ImGui::GetWindowSize();
	ImGui::Image((ImTextureID)shadowMap.depthTexture, windowSize, ImVec2(0, 1), ImVec2(1, 0));

	ImGui::EndChild();
	ImGui::End();

	// GBuffers debug render
	ImGui::Begin("GBuffers");
	ImVec2 texSize = ImVec2(gBuffer.width / 4, gBuffer.height / 4);
	for (size_t i = 0; i < 3; i++) {
		ImGui::Image((ImTextureID)gBuffer.colorBuffers[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
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

