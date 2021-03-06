#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

/** Basic GLFW header */
//#include <GL/glew.h>	// Important - this header must come before glfw3 header
#include <glad/glad.h>
#include <GLFW/glfw3.h>

/** GLFW Math */
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/** GLFW Texture header */
#include <stb_image/stb_image.h> // Support several formats of image file

/** Shader Wrapper */
#include <ShaderProgram.h>

/** Camera Wrapper */
#include <EularCamera.h>

/** Model Wrapper */
#include <Model.h>
#include <Primitives.h>



class FrameBuffer {
public:

	int width, height;

	FrameBuffer(int width, int height)
		: width(width), height(height)
	{ setup(); }

	~FrameBuffer() {
		glDeleteFramebuffers(1, &fbo);
		glDeleteRenderbuffers(1, &rbo);
	}

	void Bind() { glBindFramebuffer(GL_FRAMEBUFFER, fbo); }
	void Unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

	unsigned int FBO() { return fbo; }
	unsigned int RBO() { return rbo; }
	unsigned int TID() { return tid; }

private:
	unsigned int fbo;
	unsigned int rbo;
	unsigned int tid;

	void setup();
};

void FrameBuffer :: setup() {
	#ifdef __APPLE__
	int ratio = 2;
	#else
	int ratio = 1;
	#endif
	// Framebuffer config
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	// Crete a color attachment texture
	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_2D, tid);
	// set tex dimensions to screen size (for OSX, double screen size)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ratio * width, ratio * height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tid, 0);
	// Create a render buffer obj for depth and stencil attachment
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	// Default (GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT)
	// Use (GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT) here as an alternative. But depth test will not work
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ratio * width, ratio * height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Framebuffer is not complete!\n";
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



// Global Variables
const char* APP_TITLE = "Advanced Lighting -- HDR";
const int gWindowWidth = 1280;
const int gWindowHeight = 720;
GLFWwindow* gWindow = NULL;

// Camera system
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));

// Control
bool use_torch = true;
// Lighting mode
bool use_blinn = true;
float use_gamma = 2.2f;
// Normal mapping
bool use_normal_tex = true;
float height_scale = 0.1f;
// HDR
bool use_hdr = true;
float use_exposure = 1.0f;

// Function prototypes
void processInput(GLFWwindow* window);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height);
void showFPS(GLFWwindow* window);
bool initOpenGL();

//-----------------------------------------------------------------------------
// Main Application Entry Point
//-----------------------------------------------------------------------------
int main() {

	if (!initOpenGL()){
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return -1;
	}

	FrameBuffer frameBuffer(gWindowWidth, gWindowHeight);

	// Model loader
	//Model objectSponzaModel("Resources/sponza/sponza.obj");
	//Model objectCountryhouseModel("Resources/CountryHouse/house.obj");
	//Model objectWarehouseModel("Resources/warehouse/warehouse.obj");
	//Model objectFarmhouseModel("Resources/farmhouse/farmhouse.obj");
	//Model objectIndustrialFansModel("Resources/IndustrialFans/IndustrialFans.obj");
	//Model objectNanosuit("Resources/nanosuit/nanosuit.obj");
	//Model objectCyborg("Resources/cyborg/cyborg.obj");

	Quad objectQuad;

	Cube objectCube;
	objectCube.AddTexture("Resources/default/wood.png", TEX_DIFFUSE,  true);
	objectCube.AddTexture("Resources/default/wood.png", TEX_SPECULAR, true);
	objectCube.AddTexture("Resources/default/toy_box_normal.png", TEX_NORMAL);
	objectCube.AddTexture("Resources/default/toy_box_disp.png", TEX_HEIGHT);

	// Shader loader
	Shader objectShader("shaders/hdrLighting.vert", "shaders/hdrLighting.frag");
	Shader hdrShader("shaders/hdr.vert", "shaders/hdr.frag");



	// Light global
	glm::vec3 pointLightPos[] = {
		glm::vec3( 0.0f,  0.0f, 49.5f),
		glm::vec3(-1.4f, -1.9f,  9.0f),
		glm::vec3( 0.0f, -1.8f,  4.0f),
		glm::vec3( 0.8f, -1.7f,  6.0f)
	};
	glm::vec3 pointLightColors[] = {
		glm::vec3(200.0f),
		glm::vec3(0.1f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.2f),
		glm::vec3(0.0f, 0.1f, 0.0f)
	};
	glm::vec3 directionalLightDirection(1.0f, -1.0f, 0.0f);

	// Object shader config
	objectShader.use();
	// Light config
	// Directional light
	objectShader.setUniform("uDirectionalLight.direction", directionalLightDirection);
	objectShader.setUniform("uDirectionalLight.ambient",  0.0f, 0.0f, 0.0f);
	objectShader.setUniform("uDirectionalLight.diffuse",  0.0f, 0.0f, 0.0f);
	objectShader.setUniform("uDirectionalLight.specular", 0.0f, 0.0f, 0.0f);
	// Point light
	for (int i=0; i<4; i++) {
		objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].position").c_str(),  pointLightPos[i]);
		objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].ambient").c_str(),   0.0f, 0.0f, 0.0f);
		objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].diffuse").c_str(),   pointLightColors[i]);
		objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].specular").c_str(),  pointLightColors[i]);
		objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].constant").c_str(),  1.0f);
		objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].linear").c_str(),    0.09f);
		objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].quadratic").c_str(), 0.032f);
	}
	// Spot light
	objectShader.setUniform("uSpotLight.innerCutOff", glm::cos(glm::radians(12.5f)));
	objectShader.setUniform("uSpotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
	objectShader.setUniform("uSpotLight.ambient",  0.0f, 0.0f, 0.0f);
	objectShader.setUniform("uSpotLight.diffuse",  1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.specular", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.constant", 1.0f);
	objectShader.setUniform("uSpotLight.linear", 0.09f);
	objectShader.setUniform("uSpotLight.quadratic", 0.032f);



	// Camera global
	float aspect = (float)gWindowWidth / (float)gWindowHeight;



	// Rendering loop
	while (!glfwWindowShouldClose(gWindow)) {

		// Display FPS on title
		showFPS(gWindow);

		// Key input
		processInput(gWindow);

		std::string information;
		information = " HDR : " + std::to_string(use_exposure) + "\t\t\r";
		write(0, information.c_str(), information.size());



		// Camera transformations
		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.fov), aspect, 0.1f, 100.0f);
		//glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);



		// 1. Render scene into floating point framebuffer
		// -----------------------------------------------

		frameBuffer.Bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		objectShader.use();

		objectShader.setUniform("uEnableBlinn", use_blinn);
		objectShader.setUniform("uEnableTorch", use_torch);
		objectShader.setUniform("uEnableNormal", use_normal_tex);
		objectShader.setUniform("uGamma", use_gamma);
		objectShader.setUniform("uHeightScale", height_scale);
		objectShader.setUniform("uReverseNormal", true);

		objectShader.setUniform("uView", view);
		objectShader.setUniform("uProjection", projection);
		objectShader.setUniform("uCameraPos", camera.position);

		objectShader.setUniform("uSpotLight.position", camera.position);
		objectShader.setUniform("uSpotLight.direction", camera.front);

		glm::mat4 modelMatrix;

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 25.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(5.0f, 5.0f, 55.0f));
		objectShader.use();
		objectShader.setUniform("uModel", modelMatrix);
		objectCube.Draw(objectShader);

		frameBuffer.Unbind();

		// 2. Render floating point color buffer to 2D quad and
		// tonemap HDR colors to default framebuffer color range
		// -----------------------------------------------------

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		hdrShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.TID());
		hdrShader.setUniform("uHDRBuffer", 0);
		hdrShader.setUniform("uHDR", use_hdr);
		hdrShader.setUniform("uExposure", use_exposure);
		objectQuad.Draw(hdrShader);



		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwPollEvents();
		glfwSwapBuffers(gWindow);
	}
	
	glfwTerminate();

	return 0;
}

//-----------------------------------------------------------------------------
// Initialize GLFW and OpenGL
//-----------------------------------------------------------------------------
bool initOpenGL() {

	// Intialize GLFW 
	// GLFW is configured.  Must be called before calling any GLFW functions
	if (!glfwInit()) {
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// forward compatible with newer versions of OpenGL as they become available
	// but not backward compatible (it will not run on devices that do not support OpenGL 3.3
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Create an OpenGL 3.3 core, forward compatible context window
	gWindow = glfwCreateWindow(gWindowWidth, gWindowHeight, APP_TITLE, NULL, NULL);
	if (gWindow == NULL) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	// Make the window's context the current one
	glfwMakeContextCurrent(gWindow);

	// Set the required callback functions
	//glfwSetKeyCallback(gWindow, glfw_onKey);
	glfwSetCursorPosCallback(gWindow, mouseCallback);
	glfwSetScrollCallback(gWindow, scrollCallback);
	glfwSetFramebufferSizeCallback(gWindow, glfw_onFramebufferSize);

	// Initialize GLAD: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

	// Define the viewport dimensions
	//glViewport(0, 0, gWindowWidth, gWindowHeight);

	// Configure global OpenGL state	
	glEnable(GL_DEPTH_TEST);

	// Blending functionality
	glEnable(GL_BLEND);
	// Tell OpenGL how to calc colors of blended fragments (pixels)
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Hide the cursor and capture it
	glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return true;
}

//-----------------------------------------------------------------------------
// Is called whenever a key is pressed/released via GLFW
//-----------------------------------------------------------------------------
void processInput(GLFWwindow* window) {

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.processAccerlate(true);
	else
		camera.processAccerlate(false);

	static float deltaTime = 0.0f;
	static float lastFrame = 0.0f;
	float currentFrame = (float) glfwGetTime(); // Per-frame time
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.processKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.processKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.processKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.processKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.processKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camera.processKeyboard(DOWN, deltaTime);
	
	static bool gWireframe = false;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
		gWireframe = !gWireframe;
		if (gWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		use_torch = !use_torch;
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		use_blinn = !use_blinn;
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		use_normal_tex = !use_normal_tex;
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
		use_hdr = !use_hdr;
	if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
		use_gamma = use_gamma >= 4.0f ? 4.0f : use_gamma + 0.01f;
	if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
		use_gamma = use_gamma <= 1.0f ? 1.0f : use_gamma - 0.01f;
	if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS)
		height_scale = height_scale >= 1.0f ? 1.0f : height_scale + 0.0005f;
	if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS)
		height_scale = height_scale <= 0.0f ? 0.0f : height_scale - 0.0005f;
	if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
		use_exposure = use_exposure >= 5.0f ? 5.0f : use_exposure + 0.01f;
	if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
		use_exposure = use_exposure <= 0.0f ? 0.0f : use_exposure - 0.01f;
}

//-----------------------------------------------------------------------------
// Is called whenever mouse movement is detected via GLFW
//-----------------------------------------------------------------------------
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	static bool firstMouse = true;
	static float lastX = gWindowWidth / 2;
	static float lastY = gWindowHeight / 2;

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coord range from buttom to top
	lastX = xpos;
	lastY = ypos;

	camera.processMouse(xoffset, yoffset);
}

//-----------------------------------------------------------------------------
// Is called whenever scroller is detected via GLFW
//-----------------------------------------------------------------------------
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	camera.processScroll(yoffset);
}

//-----------------------------------------------------------------------------
// Is called when the window is resized
//-----------------------------------------------------------------------------
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

//-----------------------------------------------------------------------------
// Code computes the average frames per second, and also the average time it takes
// to render one frame.  These stats are appended to the window caption bar.
//-----------------------------------------------------------------------------
void showFPS(GLFWwindow* window)
{
	static double previousSeconds = 0.0;
	static int frameCount = 0;
	double elapsedSeconds;
	double currentSeconds = glfwGetTime(); // returns number of seconds since GLFW started, as double float

	elapsedSeconds = currentSeconds - previousSeconds;

	// Limit text updates to 4 times per second
	if (elapsedSeconds > 0.25)
	{
		previousSeconds = currentSeconds;
		double fps = (double)frameCount / elapsedSeconds;
		double msPerFrame = 1000.0 / fps;

		// The C++ way of setting the window title
		std::ostringstream outs;
		outs.precision(3);	// decimal places
		outs << std::fixed
			<< APP_TITLE << "    "
			<< "FPS: " << fps << "    "
			<< "Frame Time: " << msPerFrame << " (ms)";
		glfwSetWindowTitle(window, outs.str().c_str());

		// Reset for next average.
		frameCount = 0;
	}

	frameCount++;
}
