// Includes/Defines
// ----------------
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// Classes
// -------
class mMath
{
public:
	static float lerp(float a, float b, float f)
	{
		return (a * (1.0f - f)) + (b * f);
	}

	static float clamp(float v, float min, float max)
	{
		if (v < min)
			return min;
		if (v > max)
			return max;
		return v;
	}
};

// Declarations
// ------------
int init();
void renderLoop();
void processInput(GLFWwindow*);
void initBuffers();
void getFps();
void updateImGui();
void updateWindowResolution();
bool fpsLimiter();
void framebuffer_size_callback(GLFWwindow*, int width, int height);

// Variables
// ---------
GLFWwindow* window;
unsigned int VAO;
unsigned int EBO;
Shader* simpleShader;
Shader* raveShader;
Shader* workingShader;
bool useRaveShader;
unsigned int texture1;
unsigned int texture2;
glm::mat4 transform(1.0f);
int sWidth = 1280;
int sHeight = 720;
float camZoomAmnt = -500.0f;
glm::vec3 cubeRotAmount(0);
float boxSpinSpeed = 0.0f;
float boxSpinAmount = 0;
bool unlimitedFramerate = false;
int maxFramerate = 120;
float camRotSpeed = 4.5f;
float camRotSmoothing = 0.95f;
float camClipNear = 0.1f;
float camClipFar = 100000;
int boxAmount = 30;
glm::vec3 camPos;

// imgui
bool enableVsyncCheckbox = true;
bool fullscreenCheckbox = false;
int vsyncAmount = 1;
bool enableOuterWireframeBoxes = true;
bool enableInnerWireframeBoxes = false;
float outerWireframeThickness = 2.0f;
float outerWireframeScale = 1.05f;
float innerWireframeThickness = 2.0f;

// fps
double prevTime = 0.0;
double crntTime = 0.0;
double timeDif;
unsigned int frameCounter = 0;
int fps = 0;
float ms = 0.0f;

// Definitions
// -----------
int main()
{
	if (init() != 0)
		return -1;

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	simpleShader = new Shader("SimpleVertex.vert", "SimpleFragment.frag");
	raveShader = new Shader("RaveVertex.vert", "SimpleFragment.frag");

	initBuffers();

	while (!glfwWindowShouldClose(window))
	{
		if (!enableVsyncCheckbox && !fpsLimiter() && !unlimitedFramerate)
			continue;

		renderLoop();
		getFps();
	}

	glfwTerminate();
	return 0;
}

void updateImGui()
{
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("OpenGL", (bool*)0, flags))
	{
		if (ImGui::Button("Exit"))
		{
			glfwSetWindowShouldClose(window, true);
		}
		ImGui::Text(("FPS: " + std::to_string(fps) + " fps").c_str());
		ImGui::Text(("Frame Time: " + std::to_string(ms) + "ms").c_str());

		ImGui::Separator();

		if (ImGui::BeginTabBar("Tab Bar", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Window"))
			{
				ImGui::Text(("Resolution: " + std::to_string(sWidth) + "x" + std::to_string(sHeight)).c_str());
				ImGui::Separator();

				if (ImGui::TreeNode("Framerate"))
				{
					if (ImGui::Checkbox("Vsync", &enableVsyncCheckbox))
					{
						glfwSwapInterval(enableVsyncCheckbox ? vsyncAmount : 0);
					}
					if (enableVsyncCheckbox)
					{
						if (ImGui::TreeNode("Vsync Limit"))
						{
							if (ImGui::RadioButton("Full", &vsyncAmount, 1))
								glfwSwapInterval(enableVsyncCheckbox ? vsyncAmount : 0);
							if (ImGui::RadioButton("Half", &vsyncAmount, 2))
								glfwSwapInterval(enableVsyncCheckbox ? vsyncAmount : 0);
							if (ImGui::RadioButton("Quarter", &vsyncAmount, 4))
								glfwSwapInterval(enableVsyncCheckbox ? vsyncAmount : 0);
							if (ImGui::RadioButton("Sixth", &vsyncAmount, 6))
								glfwSwapInterval(enableVsyncCheckbox ? vsyncAmount : 0);

							ImGui::TreePop();
						}
					}
					else
					{
						ImGui::Checkbox("Unlimited", &unlimitedFramerate);

						if (!unlimitedFramerate)
							ImGui::SliderInt("Max Framerate", &maxFramerate, 15, 360, "%d fps", ImGuiSliderFlags_AlwaysClamp);
					}
					ImGui::TreePop();
				}
				if (ImGui::TreeNode("Resolution"))
				{
					if (ImGui::Checkbox("Fullscreen", &fullscreenCheckbox))
					{
						updateWindowResolution();
					}

					ImGui::TreePop();
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Scene Settings"))
			{
				if (ImGui::SliderFloat("Box Spin Speed", &boxSpinSpeed, 0.0f, 1000.0f, "%.2f"))
				{
					boxSpinAmount = 0;
				}
				ImGui::SliderInt("Box Count", &boxAmount, 1, 30, boxAmount != 1 ? "%d boxes" : "%d box");

				ImGui::Checkbox("Enable Outer Wireframe Boxes", &enableOuterWireframeBoxes);

				if (enableOuterWireframeBoxes)
				{
					ImGui::SliderFloat("Outer Wireframe Thickness", &outerWireframeThickness, 0.1f, 10.0f, "%.1f");
					ImGui::SliderFloat("Outer Wireframe Scale", &outerWireframeScale, 1.0f, 1.5f, "%.2f");
				}

				ImGui::Checkbox("Use Inner Wireframe Boxes", &enableInnerWireframeBoxes);

				if (enableInnerWireframeBoxes)
					ImGui::SliderFloat("Inner Wireframe Thickness", &innerWireframeThickness, 0.1f, 10.0f, "%.1f");

				ImGui::Checkbox("Use Rave Shader", &useRaveShader);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Camera Settings"))
			{
				ImGui::SliderFloat("Camera Rotation Amount", &camRotSpeed, 0.1f, 15.0f, "%.1f");
				ImGui::SliderFloat("Camera Clip Near", &camClipNear, 0.01f, 10.0f, "%.2f");
				ImGui::SliderFloat("Camera Clip Far", &camClipFar, 50, 100000.0f, "%.0f");
				float* arr[] = { &camPos.x, &camPos.y, &camPos.z };
				ImGui::SliderFloat3("Camera Position", *arr, -50, 50, "%.3f", 0);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}
}

void updateWindowResolution()
{
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	sWidth = mode->width;
	sHeight = mode->height;

	if (fullscreenCheckbox)
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, sWidth, sHeight, GLFW_DONT_CARE);
	else
	{
		sWidth /= 1.5f;
		sHeight /= 1.5f;
		glfwSetWindowMonitor(window, NULL, sWidth / 4, sHeight / 4, sWidth, sHeight, GLFW_DONT_CARE);
	}

	if (enableVsyncCheckbox)
		glfwSwapInterval(vsyncAmount);
	else
		glfwSwapInterval(0);

	glViewport(0, 0, sWidth, sHeight);

}

float lastLimiterTime;
bool fpsLimiter()
{
	if (glfwGetTime() < lastLimiterTime + 1.0 / maxFramerate)
		return false;
	lastLimiterTime += 1.0 / maxFramerate;
	return true;
}

void getFps()
{
	crntTime = glfwGetTime();
	timeDif = crntTime - prevTime;
	frameCounter++;

	if (timeDif >= 1.0 / 30.0)
	{
		fps = (1.0 / timeDif) * frameCounter;
		ms = (timeDif / frameCounter) * 1000;
		prevTime = crntTime;
		frameCounter = 0;
	}
}

glm::vec3 cubeRotInputVec(0);
float zoomMax = -100.0f;
float zoomMin = 4.0f;

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camZoomAmnt = mMath::lerp(camZoomAmnt, zoomMax, 0.01f);
	else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camZoomAmnt = mMath::lerp(camZoomAmnt, zoomMin, 0.05f);
	else
		camZoomAmnt = mMath::lerp(camZoomAmnt, 0, 0.05f);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cubeRotInputVec.x += camRotSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cubeRotInputVec.x -= camRotSpeed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cubeRotInputVec.y += camRotSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cubeRotInputVec.y -= camRotSpeed;

	cubeRotInputVec *= camRotSmoothing;
	cubeRotAmount = cubeRotInputVec;
}

void initBuffers()
{
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};

	// VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// VBO
	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Texture Coords
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}

void renderLoop()
{
	processInput(window);

	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ImGui
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture2);

	// 3D
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(cubeRotAmount.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(cubeRotAmount.y), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::translate(view, glm::vec3(camPos.x, camPos.y, camPos.z + camZoomAmnt - 6.0f));
	glm::mat4 projection;
	projection = glm::perspective(glm::radians(65.0f), (float)sWidth / sHeight, camClipNear, camClipFar);


	if (useRaveShader)
		workingShader = raveShader;
	else
		workingShader = simpleShader;

	glUniformMatrix4fv(glGetUniformLocation(workingShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(glGetUniformLocation(workingShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(workingShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform1f(glGetUniformLocation(workingShader->ID, "time"), glfwGetTime());

	glEnable(GL_DEPTH_TEST);

	workingShader->use();
	glBindVertexArray(VAO);

	for (int i = 0; i < boxAmount; i++)
	{
		if (!enableInnerWireframeBoxes)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else
		{
			glLineWidth(innerWireframeThickness);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		glUniform3f(glGetUniformLocation(workingShader->ID, "colorAdd"), 1.0f, 1.0f, 1.0f);
		glUniformMatrix4fv(glGetUniformLocation(workingShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, 0, 36);

		if (enableOuterWireframeBoxes)
		{
			glLineWidth(outerWireframeThickness);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glUniform3f(glGetUniformLocation(workingShader->ID, "colorAdd"), 2.0f, 2.0f, 2.0f);
			model = glm::scale(model, glm::vec3(outerWireframeScale));
			glUniformMatrix4fv(glGetUniformLocation(workingShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		boxSpinAmount += boxSpinSpeed * (float)glfwGetTime() / 50000;

		model = glm::translate(model, glm::vec3(0, 0, -1));
		model = glm::scale(model, glm::vec3(1.5f));
		model = glm::rotate(model, glm::radians(boxSpinAmount) / boxAmount, glm::vec3(0, 0, 1.0f));
	}

	updateImGui();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Swap & Poll
	glfwSwapBuffers(window);
	glfwPollEvents();
}

int init()
{
	LOG("Initializing...");

	// Initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(sWidth, sHeight, "OpenGL", NULL, NULL);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	if (window == NULL)
	{
		LOG("Failed to create GLFW window");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	// Initialize GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		LOG("Failed to initialize GLAD");
		return -1;
	}

	glViewport(0, 0, sWidth, sHeight);

	// Register resize events
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwSwapInterval(1);
	updateWindowResolution();

	LOG("Done.");

	return 0;
}

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
	if (width <= 0 || height <= 0)
		return;

	sWidth = width;
	sHeight = height;
	glViewport(0, 0, sWidth, sHeight);
}