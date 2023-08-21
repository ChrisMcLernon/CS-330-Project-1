#pragma region Settings and Includes

// Includes
// -------
#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnOpengl/camera.h>

// Image Loading Utility Functions
// -------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
                            
using namespace std;   // Using Standard Namespace
using namespace glm;   // Using GLM Namespace

// Shader Program Macro
// --------------------
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed Namespace
// -----------------
namespace
{
	// Settings
	// --------
	const char* const WINDOW_TITLE = "Project 1 - Chris McLernon";
	const int WINDOW_WIDTH = 1280;
	const int WINDOW_HEIGHT = 720;

	// GLdata for Mesh
	// ---------------
	struct GLmesh
	{
		GLuint VAO;         // Vertex Array Object
		GLuint VBO[2];      // Vertex Buffer Object
		GLuint nIndices;    // Number of Indices in the Mesh
	};

	// Create Window Object
	// --------------------
	GLFWwindow* gWindow = nullptr;

	// Mesh Data
	// ---------
	GLmesh scissorsBladeMesh, floorMesh, blockMesh_1, blockMesh_2, lampMesh;

	// Texture IDs
	// ----------
	GLuint gTextureIdMetal, gTextureIdFloor, gTextureIdWood, gTextureIdYellowWood;
	vec2 uvScale(1.0f, 1.0f);

	// Shader Program
	// --------------
	GLuint gProgramID;
	GLuint gLampProgramID;

	// Camera
	// ------
	Camera gCamera(vec3(0.0f, 0.0f, 3.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	bool viewProjection = true;
	float cameraSpeed = 0.05f;

	// Frame Timing
	// ------------
	float gDeltaTime = 0.0f;   // Time Between Current and Last Frame
	float gLastFrame = 0.0f;

	// Light Settings
	// --------------
	vec3 lightColor(1.0f, 1.0f, 1.0f);
	vec3 lightColor_1(1.0f, 0.5f, 1.0f);
	vec3 lightPos(2.0f, 0.5f, -5);
	vec3 lightPos_1(2.0f, 0.5f, 4);
	vec3 lightScale(2.0f);
}

// Function Prototypes
// -------------------
bool initialize(int, char* [], GLFWwindow * *window);
void resizeWindow(GLFWwindow * window, int width, int height);
void processInput(GLFWwindow * window);
void mousePositionCallback(GLFWwindow* window, double xPos, double yPos);
void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset); 
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void createScissorMesh(GLmesh& mesh);
void createFloorMesh(GLmesh& mesh);
void createBlock1Mesh(GLmesh& mesh);
void createBlock2Mesh(GLmesh& mesh);
void createLampMesh(GLmesh& mesh);
void destroyMeshs(GLmesh& mesh, GLmesh& mesh2, GLmesh& mesh3, GLmesh& mesh4, GLmesh& mesh5);
void render();
bool createShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint & programID);
void destroyShaderProgram(GLuint programID);
bool createTexture(const char* filename, GLuint& textureId);
void destroyTexture(GLuint textureId);
void terminateApplication(GLmesh& mesh, GLmesh& mesh2, GLmesh& mesh3, GLmesh& mesh4, GLmesh& mesh5, GLuint & programID_0, GLuint& programID_1, GLuint& textureId_0, GLuint& textureId_1, GLuint& textureId_2, GLuint& textureId_3);

#pragma endregion

#pragma region Shader Source Code

// Vertex Shader Program Source Code
// ---------------------------------
const GLchar* vertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
	layout(location = 1) in vec3 normal; // VAP position 1 for normals
	layout(location = 2) in vec2 textureCoordinate;

	out vec3 vertexNormal; // For outgoing normals to fragment shader
	out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
	out vec2 vertexTextureCoordinate;

	//Uniform / Global variables for the  transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

		vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

		vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties

		vertexTextureCoordinate = textureCoordinate;
	}
);

// Fragment Shader Program Source Code
// -----------------------------------
const GLchar* fragmentShaderSource = GLSL(440,

	in vec3 vertexNormal; // For incoming normals
	in vec3 vertexFragmentPos; // For incoming fragment position
	in vec2 vertexTextureCoordinate;

	out vec4 fragmentColor; // For outgoing cube color to the GPU

	// Uniform / Global variables for object color, light color, light position, and camera/view position
	uniform vec3 objectColor;
	uniform vec3 lightColor;
	uniform vec3 lightPos;
	uniform vec3 viewPosition;
	uniform sampler2D uTexture; // Useful when working with multiple textures
	uniform vec2 uvScale;

	void main()
	{
		/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

	//Calculate Ambient lighting*/
		float ambientStrength = 0.0f; // Set ambient or global lighting strength.
		vec3 ambient = ambientStrength * lightColor; // Generate ambient light color.

		//Calculate Diffuse lighting*/
		vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit.
		vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube.
		float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light.
		vec3 diffuse = impact * lightColor; // Generate diffuse light color.

		//Calculate Specular lighting*/
		float specularIntensity = 0.2f; // Set specular light strength.
		float highlightSize = 8.0f; // Set specular highlight size.
		vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction.
		vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector.
		//Calculate specular component.
		float specularComponent = pow(max(dot(viewDir, reflectDir), 1.0f), highlightSize);
		vec3 specular = specularIntensity * specularComponent * lightColor;

		// Texture holds the color to be used for all three components
		vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

		// Calculate phong result
		vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

		fragmentColor = vec4(phong, 1.0f); // Send lighting results to GPU
	}
);

	/* Lamp Shader Source Code*/
	const GLchar* lampVertexShaderSource = GLSL(440,

		layout(location = 0) in vec3 position;

		//Uniform / Global variables for the  transform matrices
		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

		void main()
		{
			gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into coordinates
		}
	);


	/* Fragment Shader Source Code*/
	const GLchar* lampFragmentShaderSource = GLSL(440,

		out vec4 fragmentColor;

		void main()
		{
			fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
		}
	);

// Images are Loaded with Y-Axis going down, but OpenGL's Y-Axis Goes Up this Function Flips it
// --------------------------------------------------------------------------------------------
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}


#pragma endregion

// Main Function: Entry Point to OpenGL Program
// --------------------------------------------
int main(int argc, char* argv[])
{
	// Error Check: GLFW and GLEW Initialized
	// --------------------------------------
	if (!initialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Creates the Meshs
	// -----------------
	createScissorMesh(scissorsBladeMesh);
	createFloorMesh(floorMesh);
	createBlock1Mesh(blockMesh_1);
	createBlock2Mesh(blockMesh_2);

	// Create the Shader Program
	// -------------------------
	if (!createShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramID))
		return EXIT_FAILURE;
	if (!createShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramID))
		return EXIT_FAILURE;

	// Load Textures
	// -------------
	const char* texFilename = "resources/textures/metalTexture.jpg";
	if (!createTexture(texFilename, gTextureIdMetal))
	{
		cerr << "Failed to Load Texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "resources/textures/floorTexture.jpg";
	if (!createTexture(texFilename, gTextureIdFloor))
	{
		cerr << "Failed to Load Texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "resources/textures/woodTexture.jpg";
	if (!createTexture(texFilename, gTextureIdWood))
	{
		cerr << "Failed to Load Texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "resources/textures/yellowWoodTexture.jpg";
	if (!createTexture(texFilename, gTextureIdYellowWood))
	{
		cerr << "Failed to Load Texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	
	glUseProgram(gProgramID);

	glUniform1i(glGetUniformLocation(gProgramID, "uTexture"), 0);

	// Render Loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{
		// Per-Frame Timing
		// ----------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// Input
		// -----
		processInput(gWindow);

		// Renders Frame
		// -------------
		render();

		// GLFW: Poll IO Events
		// ------------------------------------
		glfwPollEvents();
	}

	// Terminates Process
	// ------------------
	terminateApplication(blockMesh_1, scissorsBladeMesh, blockMesh_2, floorMesh, lampMesh, gProgramID, gLampProgramID, gTextureIdMetal, gTextureIdFloor, gTextureIdWood, gTextureIdYellowWood);
}

// Initialize GLFW, GLEW, and Create the Window Object
// ---------------------------------------------------
bool initialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: Initialize and Configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// GLFW: Create Window
	// -------------------
	*window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);

	// Error Check: Window Creation
	// ----------------------------
	if (*window == NULL)
	{
		cerr << "Failed to create GLFW window" << endl;
		terminateApplication(blockMesh_1, blockMesh_2, scissorsBladeMesh, floorMesh, lampMesh, gProgramID, gLampProgramID, gTextureIdMetal, gTextureIdFloor, gTextureIdWood, gTextureIdYellowWood);
		return false;
	}

	// GLFW: Make Window Current Context
	// ---------------------------------
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, resizeWindow);

	// GLFW: Mouse Control Callbacks
	// -----------------------------
	glfwSetCursorPosCallback(*window, mousePositionCallback);
	glfwSetScrollCallback(*window, mouseScrollCallback);
	glfwSetMouseButtonCallback(*window, mouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLEW: Initialize
	// ----------------
	GLenum GlewInitResult = glewInit();

	// Error Check: GLEW Initialization
	// --------------------------------
	if (GLEW_OK != GlewInitResult)
	{
		cerr << glewGetErrorString(GlewInitResult) << endl;
		return false;
	}

	// Display GPU OpenGL Version
	// --------------------------
	cerr << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	// Background Color - Black
	// ------------------------
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	return true;
}

#pragma region Input Handling

// GLFW: Process All Input
// -----------------------
void processInput(GLFWwindow* window)
{
	static const float cameraSpeed = 2.5f;   // Camera Speed

	// When Escape is Pressed - Closes Window
	// When 1 is Pressed: Enables Wireframe
	// When 2 is Pressed: Disables Wireframe
	// When W is Pressed: Moves Camera Forward
	// When S is Pressed: Moves Camera Backwards
	// When A is Pressed: Moves Camera Left
	// When D is Pressed: Moves Camera Right
	// When Q is Pressed: Moves Camera Up
	// When E is Pressed: Moves Camera Down
	// ------------------------------------------
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		viewProjection = !viewProjection;
}

// GLFW: Whenever the Window Size Changes this Function Executes
// -------------------------------------------------------------
void resizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// GLFW: When the Mouse Moves, this is Called
// ------------------------------------------
void mousePositionCallback(GLFWwindow* window, double xPos, double yPos)
{
	if (gFirstMouse)
	{
		gLastX = xPos;
		gLastY = yPos;
		gFirstMouse = false;
	}

	float xOffset = xPos - gLastX;
	float yOffset = gLastY - yPos;

	gLastX = xPos;
	gLastY = yPos;

	gCamera.ProcessMouseMovement(xOffset, yOffset);
}

// GLFW: Whenever the Mouse Scroll Wheel Scrolls, this is Called
// -------------------------------------------------------------
void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset) 
{
	if (yOffset > 0 && cameraSpeed < 0.1f)
		cameraSpeed += 0.01f;
	if (yOffset < 0 && cameraSpeed > 0.01f)
		cameraSpeed -= 0.01f;
}

// GLFW: Handle Mouse Button Events
// --------------------------------
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
		case GLFW_MOUSE_BUTTON_LEFT:
		{
			if (action == GLFW_PRESS)
				cout << "Left mouse button pressed" << endl;
			else
				cout << "Left mouse button released" << endl;
		}
		break;

		case GLFW_MOUSE_BUTTON_MIDDLE:
		{
			if (action == GLFW_PRESS)
				cout << "Middle mouse button pressed" << endl;
			else
				cout << "Middle mouse button released" << endl;
		}
		break;

		case GLFW_MOUSE_BUTTON_RIGHT:
		{
			if (action == GLFW_PRESS)
				cout << "Right mouse button pressed" << endl;
			else
				cout << "Right mouse button released" << endl;
		}
		break;

		default:
			cout << "Unhandled mouse button event" << endl;
			break;
	}
}

#pragma endregion

// Renders each Frame
// ------------------
void render()
{
	// Enable Z-Depth
	// --------------
	glEnable(GL_DEPTH_TEST);

	// Clear the Frame and Z-Buffers
	// -----------------------------
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set Shader being Used
	// ---------------------
	glUseProgram(gProgramID);

	// Transforms the Camera
	// ---------------------
	mat4 view = gCamera.GetViewMatrix();

	// Creates a Perspective Projection: 4 Parameters (FOV, Aspect Ratio, Near PLane, Far Plane)
	// -----------------------------------------------------------------------------------------
	mat4 projection;
	if (viewProjection) {
		projection = perspective(radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else {
		float scale = 120;
		projection = ortho((800.0f / scale), -(800.0f / scale), -(600.0f / scale), (600.0f / scale), -2.5f, 6.5f);
	}

	GLint viewLoc = glGetUniformLocation(gProgramID, "view");
	GLint projLoc = glGetUniformLocation(gProgramID, "projection");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));

	// Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
	GLint objectColorLoc = glGetUniformLocation(gProgramID, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(gProgramID, "lightColor");
	GLint keyLightColorLoc = glGetUniformLocation(gProgramID, "keyLightColor");
	GLint lightPositionLoc = glGetUniformLocation(gProgramID, "lightPos");
	GLint viewPositionLoc = glGetUniformLocation(gProgramID, "viewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(lightPositionLoc, lightPos.x, lightPos.y, lightPos.z);
	const glm::vec3 cameraPosition = gCamera.Position;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	GLint UVScaleLoc = glGetUniformLocation(gProgramID, "uvScale");
	glUniform2fv(UVScaleLoc, 1, value_ptr(uvScale));

#pragma region Scissors Rendering

	// Left Blade
	// Bind Textures to Corresponding Texture Units
	// --------------------------------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdMetal);

	mat4 scale = glm::scale(vec3(1.2f, 1.2f, 1.2f));
	mat4 yRotation = rotate(1.28f, vec3(1.0f, 0.0f, 0.0f));
	mat4 xRotation = rotate(0.0f, vec3(0.0f, 1.0f, 0.0f));
	mat4 zRotation = rotate(-0.4f, vec3(0.0f, 0.0f, 1.0f));
	mat4 translation = glm::translate(vec3(2.2f, -0.582f, -1.4f));

	// Transformation are Applied Right-To-Left
	// ----------------------------------------
	mat4 model = translation * (xRotation * yRotation * zRotation) * scale;

	// Retrieves and Passes Transform Matriceces to the Shader Program
	// ---------------------------------------------------------------
	GLint modelLoc = glGetUniformLocation(gProgramID, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

	// Activate VBO's winthin mesh's VAO
	// ---------------------------------
	glBindVertexArray(scissorsBladeMesh.VAO);

	glDrawElements(GL_TRIANGLES, scissorsBladeMesh.nIndices, GL_UNSIGNED_SHORT, NULL);

	// Right Blade 
	// Bind Textures to Corresponding Texture Units
	// --------------------------------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdMetal);

	scale = glm::scale(vec3(-1.2f, 1.2f, 1.2f));
	yRotation = rotate(1.18f, vec3(1.0f, 0.0f, 0.0f));
	xRotation = rotate(0.0f, vec3(0.0f, 1.0f, 0.0f));
	zRotation = rotate(-0.8f, vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(vec3(1.9f, -0.5f, -1.2f));

	// Transformation are Applied Right-To-Left
	// ----------------------------------------
	model = translation * (xRotation * yRotation * zRotation) * scale;

	// Retrieves and Passes Transform Matriceces to the Shader Program
	// ---------------------------------------------------------------
	modelLoc = glGetUniformLocation(gProgramID, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

	// Activate VBO's winthin mesh's VAO
	// ---------------------------------
	glBindVertexArray(scissorsBladeMesh.VAO);

	// Draw Triangle
	// -------------
	glDrawElements(GL_TRIANGLES, scissorsBladeMesh.nIndices, GL_UNSIGNED_SHORT, NULL);

#pragma endregion

#pragma region Floor Rendering

	// Bind Textures to Corresponding Texture Units
	// --------------------------------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdFloor);
	
	scale = glm::scale(vec3(12.0f, 12.0f, 12.0f));
	yRotation = rotate(0.0f, vec3(1.0f, 0.0f, 0.0f));
	xRotation = rotate(0.0f, vec3(0.0f, 1.0f, 0.0f));
	zRotation = rotate(0.0f, vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(vec3(0.0f, -1.0f, 0.0f));

	// Transformation are Applied Right-To-Left
	// ----------------------------------------
	model = translation * (xRotation * yRotation * zRotation) * scale;

	// Retrieves and Passes Transform Matriceces to the Shader Program
	// ---------------------------------------------------------------
	modelLoc = glGetUniformLocation(gProgramID, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

	// Activate VBO's winthin mesh's VAO
	// ---------------------------------
	glBindVertexArray(floorMesh.VAO);

	// Bind Textures to Corresponding Texture Units
	// --------------------------------------------

	glDrawElements(GL_TRIANGLES, floorMesh.nIndices, GL_UNSIGNED_SHORT, NULL);

#pragma endregion

#pragma region Block_1 Rendering

	// Bind Textures to Corresponding Texture Units
	// --------------------------------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdWood);

	scale = glm::scale(vec3(2.f, 2.0f, 2.0f));
	yRotation = rotate(0.0f, vec3(1.0f, 0.0f, 0.0f));
	xRotation = rotate(0.3f, vec3(0.0f, 1.0f, 0.0f));
	zRotation = rotate(0.0f, vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(vec3(0.8f, -0.9999f, -0.7f));

	// Transformation are Applied Right-To-Left
	// ----------------------------------------
	model = translation * (xRotation * yRotation * zRotation) * scale;

	// Retrieves and Passes Transform Matriceces to the Shader Program
	// ---------------------------------------------------------------
	modelLoc = glGetUniformLocation(gProgramID, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

	// Activate VBO's winthin mesh's VAO
	// ---------------------------------
	glBindVertexArray(blockMesh_1.VAO);

	// Bind Textures to Corresponding Texture Units
	// --------------------------------------------

	glDrawElements(GL_TRIANGLES, blockMesh_1.nIndices, GL_UNSIGNED_SHORT, NULL);

#pragma endregion

#pragma region Block_2 Rendering

	// Bind Textures to Corresponding Texture Units
	// --------------------------------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdYellowWood);
	scale = glm::scale(vec3(2.0f, 2.0f, 2.0f));
	yRotation = rotate(0.0f, vec3(1.0f, 0.0f, 0.0f));
	xRotation = rotate(0.3f, vec3(0.0f, 1.0f, 0.0f));
	zRotation = rotate(1.565f, vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(vec3(2.6f, -0.999f, -0.2f));

	// Transformation are Applied Right-To-Left
	// ----------------------------------------
	model = translation * (xRotation * yRotation * zRotation) * scale;

	// Retrieves and Passes Transform Matriceces to the Shader Program
	// ---------------------------------------------------------------
	modelLoc = glGetUniformLocation(gProgramID, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

	// Activate VBO's winthin mesh's VAO
	// ---------------------------------
	glBindVertexArray(blockMesh_2.VAO);

	// Bind Textures to Corresponding Texture Units
	// --------------------------------------------

	glDrawElements(GL_TRIANGLES, blockMesh_2.nIndices, GL_UNSIGNED_SHORT, NULL);

#pragma endregion

#pragma region Light Binding / Generation

	// LAMP: draw lamps
	//----------------
	glUseProgram(gLampProgramID);

	yRotation = rotate(-0.25f, vec3(1.0f, 0.0f, 0.0f));
	xRotation = rotate(0.0f, vec3(0.0f, 1.0f, 0.0f));
	zRotation = rotate(0.0f, vec3(0.0f, 0.0f, 1.0f));

	//Transform the smaller cube used as a visual que for the light source
	model = translate(lightPos) * (xRotation * yRotation * zRotation) * glm::scale(lightScale);

	// Reference matrix uniforms from the Lamp Shader program
	modelLoc = glGetUniformLocation(gLampProgramID, "model");
	viewLoc = glGetUniformLocation(gLampProgramID, "view");
	projLoc = glGetUniformLocation(gLampProgramID, "projection");

	// Pass matrix data to the Lamp Shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, value_ptr(projection));
	glBindVertexArray(blockMesh_1.VAO);
	glDrawElements(GL_TRIANGLES, blockMesh_1.nIndices, GL_UNSIGNED_SHORT, NULL);

#pragma endregion

	// Deactivate the Vertex Array Object
	// ----------------------------------
	glBindVertexArray(0);

	// GLFW: Swap Buffers and Poll IO Events
	// -------------------------------------
	glfwSwapBuffers(gWindow);
}

#pragma region Meshs and Shaders

void createScissorMesh(GLmesh& mesh)
{
	// Specifies NDC for Triangle Vertices and Color
	// ---------------------------------------------
	GLfloat scissorVerts[] =
	{
		// Index 0
		// -------
		-0.2f, 0.9f, 0.0f,         // (X,Y,Z)
		0.0f,  1.0f,  0.0f,
		0, 0.9f,

		// Index 1
		// -------
	   -0.05f, 0.05f, 0.0f,         // (X,Y,Z)
	   0.0f,  1.0f,  0.0f,
		0, 0,

		// Index 2
		// -------
		-0.25f, 0.01f, 0.0f,         // (X,Y,Z)
		0.0f,  1.0f,  0.0f,
		0, 0,

		// Index 3
		// -------
	    -0.2f, -0.03f, 0.0f,          // (X,Y,Z)
		1.0f,  1.0f,  0.0f,
		0, 0,

		// Index 4
		// -------
		-0.15f, 0.01f, 0.0f,         // (X,Y,Z)
		1.0f,  1.0f,  0.0f,
		0, 0,

		// Index 5
		// -------
		-0.05f, -0.1f, 0.0f,         // (X,Y,Z)
		0.0f,  1.0f,  0.0f,
		1.0f, 0.0f,

		// Index 6
		// -------
		-0.1f, -0.1f, 0.0f,         // (X,Y,Z)
		0.0f,  1.0f,  0.0f,
		0.0f, 0.0f,

		// Index 7
		// -------
		-0.15f, -0.04f, 0.0f,         // (X,Y,Z)
		0.0f,  1.0f,  0.0f,
		0.0f, 0.0f,

		// Index 8
		// -------
		-0.12f, -0.2f, 0.0f,         // (X,Y,Z)
		0.0f,  1.0f,  0.0f,
		0.0f, 0.0f,

		// Index 9
		// -------
		-0.05f, -0.2f, 0.0f,         // (X,Y,Z)
		0.0f,  1.0f,  0.0f,
		1.0f, 0.0f,

		// Index 10
		// --------
		-0.15f, -0.04f, 0.0f,         // (X,Y,Z)
		0.0f,  1.0f,  0.0f,
		0.0f, 0.0f,

		// Index 11
		// --------
		-0.09f, -0.23f, 0.0f,         // (X,Y,Z)
		0.0f,  1.0f,  0.0f,
		0.0f, 1.0f,

		// Index 12
		// -------
		-0.2f, 0.9f, -0.02f,         // (X,Y,Z)
		0.0f,  -1.0f,  0.0f,
		1.0f, 0.0f,

		// Index 13
		// -------
		-0.05f, 0.05f, -0.02f,         // (X,Y,Z)
		0.0f,  -1.0f,  0.0f,
		1.0f, 0.0f,

		// Index 14
		// -------
		-0.25f, 0.01f, -0.02f,         // (X,Y,Z)
		0.0f,  -1.0f,  0.0f,
		0.0f, 0.0f,

		// Index 15
		// -------
		-0.2f, -0.03f, -0.02f,          // (X,Y,Z)
		0.0f,  -1.0f,  0.0f,
		0.0f, 0.0f, 

		// Index 16
		// -------
		-0.15f, 0.01f, -0.02f,         // (X,Y,Z)
		0.0f, -1.0f, 0.0f,
		0.0f, 0.0f,

		// Index 17
		// -------
		-0.05f, -0.1f, -0.02f,         // (X,Y,Z)
		   0.0f, -1.0f, 0.0f,
		0.0f, 0.0f,

		// Index 18
		// -------
		-0.1f, -0.1f, -0.02f,         // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		1.0f, 0.0f,

		// Index 19
		// -------
		-0.15f, -0.04f, -0.02f,         // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.0f,

		// Index 20
		// -------
		-0.12f, -0.2f, -0.02f,         // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.0f,

		// Index 21
		// -------
		-0.05f, -0.2f, -0.02f,         // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		1.0f, 0.0f,

		// Index 22
		// --------
		-0.15f, -0.04f, -0.02f,         // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		1.0f, 0.0f,

		// Index 23
		// --------
		-0.09f, -0.23f, -0.02f,         // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 1.0f,

		// Index 24
		// --------
	    -0.18f, 0.8f, -0.02f,         // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		1.0f, 1.0f,

		// Index 25
		// --------
		0.04f, -0.26f, 0.02f,         // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		1.0f, 0.0f,

		// Index 26
		// --------
		-0.12f, -0.5f, 0.02f,         // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 1.0f,

		// Index 27
		// --------
		-0.045f, -0.40f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 28
		// --------
		0.0f, -0.37f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 29
		// --------
		0.1f, -0.32f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 30
		// --------
		0.08f, -0.4f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 31
		// --------
		0.17f, -0.45f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 32
		// --------
		0.15f, -0.6f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 33
		// --------
		0.2f, -0.7f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 34
		// --------
		0.14f, -0.9f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 35
		// --------
		0.2f, -0.97f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 36
		// --------
		0.07f, -1.1f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 37
		// --------
		0.13f, -1.16f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 38
		// --------
		0.04f, -1.15f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 39
		// --------
		0.04f, -1.24f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 40
		// --------
		-0.08f, -1.3f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 41
		// --------
		-0.08f, -1.2f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 42
		// --------
		-0.13f, -1.25f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 43
		// --------
		-0.082f, -1.1f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 44
		// --------
		-0.16f, -0.97f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 45
		// --------
		-0.072f, -0.9f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 46
		// --------
		-0.16f, -0.83f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 47
		// --------
		-0.06f, -0.76f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 48
		// --------
		-0.055f, -0.6f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 49
		// --------
		-0.04f, -1.2f, 0.02f,      // (X,Y,Z)
			0.0f, 1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 50
		// --------
		0.04f, -0.26f, -0.04f,         // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		1.0f, 0.0f,

		// Index 51
		// --------
		-0.12f, -0.5f, -0.04f,         // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 1.0f,

		// Index 52
		// --------
		-0.045f, -0.40f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 53
		// --------
		0.0f, -0.37f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 54
		// --------
		0.1f, -0.32f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 55
		// --------
		0.08f, -0.4f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 56
		// --------
		0.17f, -0.45f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 57
		// --------
		0.15f, -0.6f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 58
		// --------
		0.2f, -0.7f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 59
		// --------
		0.14f, -0.9f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 60
		// --------
		0.2f, -0.97f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 61
		// --------
		0.07f, -1.1f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 62
		// --------
		0.13f, -1.16f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 63
		// --------
		0.04f, -1.15f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 64
		// --------
		0.04f, -1.24f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 65
		// --------
		-0.08f, -1.3f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 66
		// --------
		-0.08f, -1.2f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 67
		// --------
		-0.13f, -1.25f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 68
		// --------
		-0.082f, -1.1f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 69
		// --------
		-0.16f, -0.97f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 70
		// --------
		-0.072f, -0.9f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 71
		// --------
		-0.16f, -0.83f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 72
		// --------
		-0.06f, -0.76f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 73
		// --------
		-0.055f, -0.6f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,

		// Index 74
		// --------
		-0.04f, -1.2f, -0.04f,      // (X,Y,Z)
			0.0f, -1.0f, 0.0f,
		0.0f, 0.3f,
};

	// Create VBO: for Indices
	// -----------------------
	GLushort scissorIndices[] =
	{
		// Blade - Front
		// -------------
		0, 1, 2,
		1, 2, 3,
		1, 4, 5,
		5, 9, 6,
		9, 6, 8,
		3, 7, 4,
		7, 4, 10,
		10, 5, 6,
		4, 10, 5,
		9, 8, 11,

		// Blade - Back
		// ------------
		12, 13, 14,
		13, 14, 15,
		13, 16, 17,
		17, 21, 18,
		21, 18, 20,
		15, 19, 16,
		19, 16, 22,
		22, 17, 18,
		16, 22, 17,
		21, 20, 23,

		// Blade - Connections
		// -------------------
		0, 12, 2,
		12, 2, 14,
		2, 3, 15,
		14, 2, 15,
		15, 19, 7,
		3, 7, 15,
		7, 10, 22,
		7, 19, 22,
		6, 10, 22,
		6, 18, 22,
		6, 8, 20,
		6, 18, 20,
		8, 20, 11,
		11, 23, 20,
		1, 5, 13,
		13, 17, 5,
		5, 9, 17,
		9, 17, 21,
		9, 11, 23,
		9, 21, 23,
		0, 24, 13,
		12, 24, 13,
		0, 13, 1,
		0, 12, 24,

		// Handle - Front
		// --------------
		11, 9, 25,
		11, 25, 27,
		11, 27, 26,
		27, 25, 28,
		28, 25, 29,
		28, 29, 30,
		30, 31, 29,
		30, 31, 32,
		31, 32, 33,
		32, 33, 34,
		33, 34, 35,
		34, 35, 36,
		35, 36, 37,
		36, 37, 38,
		37, 38, 39,
		38, 39, 41,
		41, 40, 39,
		40, 41, 42,
		41, 42, 43,
		42, 43, 44,
		43, 44, 45,
		44, 45, 46,
		45, 46, 47,
		46, 47, 48,
		46, 48, 26,
		48, 26, 27,
		38, 49, 41,
		43, 41, 49,

		// Handle - Back
		// -------------
		23, 21, 50,
		23, 50, 52,
		23, 52, 51,
		52, 50, 53,
		53, 50, 54,
		53, 54, 55,
		55, 56, 54,
		55, 56, 57,
		56, 57, 58,
		57, 58, 59,
		58, 59, 60,
		59, 60, 61,
		60, 61, 62,
		61, 62, 63,
		62, 63, 64,
		63, 64, 66,
		66, 65, 64,
		65, 66, 67,
		66, 67, 68,
		67, 68, 69,
		68, 69, 70,
		69, 70, 71,
		70, 71, 72,
		71, 72, 73,
		71, 73, 51,
		73, 51, 52,
		63, 74, 66,
		68, 66, 74,

		// Handle - Connections
		// --------------------
		9, 25, 50,
		21, 9, 50,
		25, 29, 50,
		29, 50, 54,
		29, 31, 54,
		31, 54, 56,
		31, 33, 56,
		33, 56, 58,
		33, 35, 58,
		35, 58, 60,
		35, 37, 60,
		37, 60, 62,
		37, 39, 62,
		39, 62, 64,
		39, 40, 64,
		40, 64, 65,
		40, 42, 65,
		42, 65, 67,
		42, 44, 67,
		44, 67, 69,
		44, 46, 69,
		46, 69, 71,
		46, 26, 71,
		26, 71, 51,
		26, 11, 51,
		11, 51, 23,
		27, 28, 52,
		28, 52, 53,
		28, 30, 53,
		30, 53, 55,
		30, 32, 55,
		32, 55, 57,
		32, 34, 57,
		34, 57, 59,
		34, 36, 59,
		36, 59, 61,
		36, 38, 61,
		38, 61, 63,
		38, 49, 63,
		49, 63, 74,
		49, 43, 74,
		43, 74, 68,
		43, 45, 68,
		45, 68, 70,
		45, 47, 70,
		47, 70, 72,
		47, 48, 72,
		48, 72, 73,
		48, 27, 73,
		27, 73, 52

	};

	// Creates the Vertex Attribute Pointer fot the Screen Coordinates
	// ---------------------------------------------------------------
	const GLuint floatsPerVertex = 3;   // Number of Coordinates per Vertex
	const GLuint floatsPerUV = 2;    // (X, Y)
	const GLuint floatsPerNormal = 3;

	// Generates VAO's and VBO's and Activate
	// those Buffers, Then send vertices to GPU
	// ----------------------------------------
	glGenVertexArrays(1, &mesh.VAO);
	glBindVertexArray(mesh.VAO);

	// Creates 2 Buffers (VBO): First One is Vertex Data; Second One for Indices
	// -------------------------------------------------------------------
	glGenBuffers(2, mesh.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[0]);   // Activates Buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(scissorVerts), scissorVerts, GL_STATIC_DRAW);   // Sends Vertex or Coordinate Data to the GPU

	mesh.nIndices = sizeof(scissorIndices) / sizeof(scissorIndices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.VBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(scissorIndices), scissorIndices, GL_STATIC_DRAW);

	// Strides between vertex coordinates is 6 (x, y, r, g, b, a). A tightly packed stride is 0.
	// -----------------------------------------------------------------------------------------
	GLint stride = sizeof(float) * (static_cast<int64_t>(floatsPerVertex) + floatsPerNormal + floatsPerUV);   // The number of floats before each

	// Creates Vertex Attribute Pointer
	// --------------------------------
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(2);
}

void createFloorMesh(GLmesh& mesh)
{
	// Specifies NDC for Triangle Vertices and Color
	// ---------------------------------------------
	GLfloat floorVerts[] =
	{
		// Index 0
		// -------
		-1.0f, 0.0f, -1.0f,
		0.0f, 1.0f, 0.0f,
		0, 0,

		// Index 1
		// -------
		1.0f, 0.0f, -1.0f,
		0.0f, 1.0f, 0.0f,
		1, 0,

		// Index 2
		// -------
		1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		1, 1,

		// Index 3
		// -------
		-1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		0, 1


	};

	// Create VBO: for Indices
	// -----------------------
	GLushort floorIndices[] =
	{
		0, 1, 2,
		2, 3, 0
	};

	// Creates the Vertex Attribute Pointer fot the Screen Coordinates
	// ---------------------------------------------------------------
	const GLuint floatsPerVertex = 3;   // Number of Coordinates per Vertex
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;    // (X, Y)

	// Generates VAO's and VBO's and Activate
	// those Buffers, Then send vertices to GPU
	// ----------------------------------------
	glGenVertexArrays(1, &mesh.VAO);
	glBindVertexArray(mesh.VAO);

	// Creates 2 Buffers (VBO): First One is Vertex Data; Second One for Indices
	// -------------------------------------------------------------------
	glGenBuffers(2, mesh.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[0]);   // Activates Buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(floorVerts), floorVerts, GL_STATIC_DRAW);   // Sends Vertex or Coordinate Data to the GPU

	mesh.nIndices = sizeof(floorIndices) / sizeof(floorIndices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.VBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);


	// Strides between vertex coordinates is 6 (x, y, r, g, b, a). A tightly packed stride is 0.
	// -----------------------------------------------------------------------------------------
	GLint stride = sizeof(float) * (static_cast<int64_t>(floatsPerVertex) + floatsPerNormal + floatsPerUV);   // The number of floats before each

	// Creates Vertex Attribute Pointer
	// --------------------------------
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(2);
}

void createBlock1Mesh(GLmesh& mesh)
{
	// Specifies NDC for Triangle Vertices and Color
	// ---------------------------------------------
	GLfloat block_1Verts[] =
	{
		// Index 0
		// -------
		0, 0, 0,
		0.0f,  0.0f, -1.0f,
		0, 0,

		// Index 1
		// -------
		0, 0.25f, 0,
		0.0f,  0.0f, -1.0f,
		0.25f, 0,

		// Index 2
		// -------
		1, 0, 0,
		0.0f,  0.0f, -1.0f,
		0.25f, 1,

		// Index 3
		// -------
		1, 0.25f, 0,
		0.0f,  0.0f, -1.0f,
		0, 1,

		// Index 4
		// -------
		0, 0, 0.5f,
		0.0f,  0.0f,  1.0f,
		0, 0,

		// Index 5
		// -------
		0, 0.25f, 0.5f,
		0.0f,  0.0f,  1.0f,
		0.25f, 0,

		// Index 6
		// -------
		1, 0, 0.5f,
		0.0f,  0.0f,  1.0f,
		0.25f, 1,

		// Index 7
		// -------
		1, 0.25f, 0.5f,
		0.0f,  0.0f,  1.0f,
		0, 1,

		//////////////////

		// Index 8
		// -------
		0, 0, 0,
		-1.0f,  0.0f,  0.0f,
		0, 0,

		// Index 9
		// -------
		0, 0.25f, 0,
		-1.0f,  0.0f,  0.0f,
		0, 1,

		// Index 10
		// -------
		1, 0, 0,
		1.0f,  0.0f,  0.0f,
		0, 0,

		// Index 11
		// -------
		1, 0.25f, 0,
		1.0f,  0.0f,  0.0f,
		1, 0,

		// Index 12
		// -------
		0, 0, 0.5f,
		-1.0f,  0.0f,  0.0f,
		1, 0,

		// Index 13
		// -------
		0, 0.25f, 0.5f,
		-1.0f,  0.0f,  0.0f,
		1, 1,

		// Index 14
		// -------
		1, 0, 0.5f,
		1.0f,  0.0f,  0.0f,
		0, 1,

		// Index 15
		// -------
		1, 0.25f, 0.5f,
		1.0f,  0.0f,  0.0f,
		1, 1,

		///////////////////////

		// Index 16
		// -------
		0, 0, 0,
		0.0f, -1.0f, 0.0f,
		0, 1,

		// Index 17
		// -------
		0, 0.25f, 0,
		0.0f, 1.0f, 0.0f,
		0, 0,

		// Index 18
		// -------
		1, 0, 0,
		0.0f, -1.0f, 0.0f,
		1, 1,

		// Index 19
		// -------
		1, 0.25f, 0,
		0.0f, 1.0f, 0.0f,
		0, 1,

		// Index 20
		// -------
		0, 0, 0.5f,
		0.0f, -1.0f, 0.0f,
		1, 0,

		// Index 21
		// -------
		0, 0.25f, 0.5f,
		0.0f, 1.0f, 0.0f,
		1, 1,

		// Index 22
		// -------
		1, 0, 0.5f,
		0.0f, -1.0f, 0.0f,
		0, 0,

		// Index 23
		// -------
		1, 0.25f, 0.5f,
		0.0f, 1.0f, 0.0f,
		1, 0,

	};

	// Create VBO: for Indices
	// -----------------------
	GLushort block_1Indices[] =
	{
		// Side 1 - left
		0, 1, 2,
		1, 2, 3,

		// Side 2 - right
		4, 5, 6,
		5, 6, 7,

		// Side 3 - front -1.0f,  0.0f,  0.0f,
		8, 9, 12,
		9, 12, 13,

		// Side 4 - back 0.0f,  0.0f,  -1.0f,
		10, 11, 14,
		11, 14, 15,

		// Side 5 - bottom 0.0f, -1.0f, 0.0f,
		16, 18, 20,
		18, 20, 22,

		// Side 6 - top 0.0f, 1.0f, 0.0f,
		17, 19, 21,
		19, 21, 23

	};

	// Creates the Vertex Attribute Pointer fot the Screen Coordinates
	// ---------------------------------------------------------------
	const GLuint floatsPerVertex = 3;   // Number of Coordinates per Vertex
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;    // (X, Y)

	// Generates VAO's and VBO's and Activate
	// those Buffers, Then send vertices to GPU
	// ----------------------------------------
	glGenVertexArrays(1, &mesh.VAO);
	glBindVertexArray(mesh.VAO);

	// Creates 2 Buffers (VBO): First One is Vertex Data; Second One for Indices
	// -------------------------------------------------------------------
	glGenBuffers(2, mesh.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[0]);   // Activates Buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(block_1Verts), block_1Verts, GL_STATIC_DRAW);   // Sends Vertex or Coordinate Data to the GPU

	mesh.nIndices = sizeof(block_1Indices) / sizeof(block_1Indices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.VBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(block_1Indices), block_1Indices, GL_STATIC_DRAW);


	// Strides between vertex coordinates is 6 (x, y, r, g, b, a). A tightly packed stride is 0.
	// -----------------------------------------------------------------------------------------
	GLint stride = sizeof(float) * (static_cast<int64_t>(floatsPerVertex) + floatsPerNormal + floatsPerUV);   // The number of floats before each

	// Creates Vertex Attribute Pointer
	// --------------------------------
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(2);
}

void createBlock2Mesh(GLmesh& mesh)
{
	// Specifies NDC for Triangle Vertices and Color
	// ---------------------------------------------
	GLfloat block_2Verts[] =
	{
		// Index 0
		// -------
		0, 0, 0,
		0.0f,  0.0f, -1.0f,
		0, 0,

		// Index 1
		// -------
		0, 0.25f, 0,
		0.0f,  0.0f, -1.0f,
		0, 1,

		// Index 2
		// -------
		1, 0, 0,
		0.0f,  0.0f, -1.0f,
		1, 0,

		// Index 3
		// -------
		1, 0.25f, 0,
		0.0f,  0.0f, -1.0f,
		1, 1,

		// Index 4
		// -------
		0, 0, 0.25f,
		0.0f,  0.0f, 1.0f,
		0, 0,

		// Index 5
		// -------
		0, 0.25f, 0.25f,
		0.0f,  0.0f, 1.0f,
		0, 1,

		// Index 6
		// -------
		1, 0, 0.25f,
		0.0f,  0.0f, 1.0f,
		1, 0,

		// Index 7
		// -------
		1, 0.25f, 0.25f,
		0.0f,  0.0f, 1.0f,
		1, 1,

		//////////////////

		// Index 8
		// -------
		0, 0, 0,
		0.0f,  -1.0f,  0.0f,
		0, 0,

		// Index 9
		// -------
		0, 0.25f, 0,
		0.0f,  -1.0f,  0.0f,
		0, 1,

		// Index 10
		// -------
		1, 0, 0,
		0.0f,  1.0f,  0.0f,
		0, 0,

		// Index 11
		// -------
		1, 0.25f, 0,
		0.0f,  1.0f,  0.0f,
		0, 1,

		// Index 12
		// -------
		0, 0, 0.25f,
		0.0f,  -1.0f,  0.0f,
		0, 0,

		// Index 13
		// -------
		0, 0.25f, 0.25f,
		0.0f,  -1.0f,  0.0f,
		0, 1,

		// Index 14
		// -------
		1, 0, 0.25f,
		0.0f,  1.0f,  0.0f,
		1, 0,

		// Index 15
		// -------
		1, 0.25f, 0.25f,
		0.0f,  1.0f,  0.0f,
		1, 1,

		///////////////////////

		// Index 16
		// -------
		0, 0, 0,
		1.0f, 0.0f, 0.0f,
		0.25f, 0,

		// Index 17
		// -------
		0, 0.25f, 0,
		-1.0f, 0.0f, 0.0f,
		0, 0,

		// Index 18
		// -------
		1, 0, 0,
		1.0f, 0.0f, 0.0f,
		0, 1,

		// Index 19
		// -------
		1, 0.25f, 0,
		-1.0f, 0.0f, 0.0f,
		0.25f, 1,

		// Index 20
		// -------
		0, 0, 0.25f,
		1.0f, 0.0f, 0.0f,
		1, 1,

		// Index 21
		// -------
		0, 0.25f, 0.25f,
		-1.0f, 0.0f, 0.0f,
		0.25f, 0,

		// Index 22
		// -------
		1, 0, 0.25f,
		1.0f, 0.0f, 0.0f,
		1, 0,

		// Index 23
		// -------
		1, 0.25f, 0.25f,
		-1.0f, 0.0f, 0.0f,
		1, 1


	};

	// Create VBO: for Indices
	// -----------------------
	GLushort block_2Indices[] =
	{
		// Side 1 - left
		0, 1, 2,
		1, 2, 3,

		// Side 2 - right
		4, 5, 6,
		5, 6, 7,

		// Side 3 - bottom
		8, 9, 12,
		9, 12, 13,

		// Side 4 - top
		10, 11, 14,
		11, 14, 15,

		// Side 5 - front
		16, 18, 20,
		18, 20, 22,

		// Side 6 - back
		17, 19, 21,
		19, 21, 23
	};

	// Creates the Vertex Attribute Pointer fot the Screen Coordinates
	// ---------------------------------------------------------------
	const GLuint floatsPerVertex = 3;   // Number of Coordinates per Vertex
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;    // (X, Y)

	// Generates VAO's and VBO's and Activate
	// those Buffers, Then send vertices to GPU
	// ----------------------------------------
	glGenVertexArrays(1, &mesh.VAO);
	glBindVertexArray(mesh.VAO);

	// Creates 2 Buffers (VBO): First One is Vertex Data; Second One for Indices
	// -------------------------------------------------------------------
	glGenBuffers(2, mesh.VBO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[0]);   // Activates Buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(block_2Verts), block_2Verts, GL_STATIC_DRAW);   // Sends Vertex or Coordinate Data to the GPU

	mesh.nIndices = sizeof(block_2Indices) / sizeof(block_2Indices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.VBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(block_2Indices), block_2Indices, GL_STATIC_DRAW);


	// Strides between vertex coordinates is 6 (x, y, r, g, b, a). A tightly packed stride is 0.
	// -----------------------------------------------------------------------------------------
	GLint stride = sizeof(float) * (static_cast<int64_t>(floatsPerVertex) + floatsPerNormal + floatsPerUV);   // The number of floats before each

	// Creates Vertex Attribute Pointer
	// --------------------------------
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(2);
}

// Creates Shaders
// ---------------
bool createShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programID)
{
	// Compilation and Linkage Error Reporting
	// ---------------------------------------
	int success = 0;
	char infoLog[512];

	// Create Shader Program Object
	// ----------------------------
	programID = glCreateProgram();

	// Create Shader Objects
	// ---------------------
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Attach Shader Object to Shader Source
	// -------------------------------------
	glShaderSource(vertexShaderID, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderID, 1, &fragShaderSource, NULL);

	// Compile Vertex Shader
	// ---------------------
	glCompileShader(vertexShaderID);

	// Error Check: Vertex Shader Compilation
	// --------------------------------------
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderID, 512, NULL, infoLog);
		cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;

		return false;
	}

	// Compile Fragment Shader
	// -----------------------
	glCompileShader(fragmentShaderID);

	// Error Check: Fragment Shader Compilation
	// ----------------------------------------
	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderID, sizeof(infoLog), NULL, infoLog);
		cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;

		return false;
	}

	// Attach the Compiled Shaders to the Shader Program Object
	// --------------------------------------------------------
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	glLinkProgram(programID);

	// Error Check: Shader Program Link
	// --------------------------------
	glGetProgramiv(programID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programID, sizeof(infoLog), NULL, infoLog);
		cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;

		return false;
	}

	glUseProgram(programID);   // Uses Shader Program

	return true;
}

// Create Textures
// ---------------
bool createTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// Set Texture Wrapping Params
		// ---------------------------
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Set Texture Filtering Params
		// ----------------------------
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		}
		else if (channels == 4)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		}
		else
		{
			cerr << "Not Implemented to Handle Image With " << channels << " Channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);   // Unbind the Texture

		return true;
	}

	return false;   // Error Loading Image 
}

#pragma endregion

#pragma region Terminate Functions

// Destroy VAO's and VBO's to Release Mesh Data
// --------------------------------------------
void destroyMeshs(GLmesh& mesh, GLmesh& mesh2, GLmesh& mesh3, GLmesh& mesh4, GLmesh& mesh5)
{
	glDeleteVertexArrays(1, &mesh.VAO);
	glDeleteBuffers(2, mesh.VBO);
	glDeleteVertexArrays(1, &mesh2.VAO);
	glDeleteBuffers(2, mesh2.VBO);
	glDeleteVertexArrays(1, &mesh3.VAO);
	glDeleteBuffers(2, mesh3.VBO);
	glDeleteVertexArrays(1, &mesh4.VAO);
	glDeleteBuffers(2, mesh4.VBO);
}

// Destroy Shader Program
// ----------------------
void destroyShaderProgram(GLuint programID)
{
	glDeleteProgram(programID);
}

// Destroy Textures
// ----------------
void destroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}

// Terminates Application
// ----------------------
void terminateApplication(GLmesh& mesh, GLmesh& mesh2, GLmesh& mesh3, GLmesh& mesh4, GLmesh& mesh5, GLuint& programID_0, GLuint& programID_1, GLuint& textureId_0, GLuint& textureId_1, GLuint& textureId_2, GLuint& textureId_3)
{
	destroyMeshs(mesh, mesh2, mesh3, mesh4, mesh5);
	destroyTexture(textureId_0);
	destroyTexture(textureId_1);
	destroyTexture(textureId_2);
	destroyTexture(textureId_3);
	destroyShaderProgram(programID_0);
	destroyShaderProgram(programID_1);
	exit(EXIT_SUCCESS);
}

#pragma endregion