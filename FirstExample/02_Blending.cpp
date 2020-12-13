
///////////////////////////////////////////////////////////////////////
//
// 01_JustAmbient.cpp
//
///////////////////////////////////////////////////////////////////////

using namespace std;

#include <cstdlib>
#include <ctime>
#include "vgl.h"
#include "LoadShaders.h"
#include "Light.h"
#include "Shape.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FPS 60
#define MOVESPEED 0.1f
#define TURNSPEED 0.05f
#define X_AXIS glm::vec3(1,0,0)
#define Y_AXIS glm::vec3(0,1,0)
#define Z_AXIS glm::vec3(0,0,1)
#define XY_AXIS glm::vec3(1,1,0)
#define YZ_AXIS glm::vec3(0,1,1)
#define XZ_AXIS glm::vec3(1,0,1)
#define XYZ_AXIS glm::vec3(1,1,1)


enum keyMasks {
	KEY_FORWARD =  0b00000001,		// 0x01 or 1 or 01
	KEY_BACKWARD = 0b00000010,		// 0x02 or 2 or 02
	KEY_LEFT = 0b00000100,		
	KEY_RIGHT = 0b00001000,
	KEY_UP = 0b00010000,
	KEY_DOWN = 0b00100000,
	KEY_MOUSECLICKED = 0b01000000
	// Any other keys you want to add.
};

// IDs.
GLuint vao, ibo, points_vbo, colors_vbo, uv_vbo, normals_vbo, modelID, viewID, projID;
GLuint program;

// Matrices.
glm::mat4 View, Projection;

// Our bitflags. 1 byte for up to 8 keys.
unsigned char keys = 0; // Initialized to 0 or 0b00000000.

// Camera and transform variables.
float scale = 1.0f, angle = 0.0f;
glm::vec3 position, frontVec, worldUp, upVec, rightVec; // Set by function
GLfloat pitch, yaw;
int lastX, lastY;
float rotAngle = 0.0f;


// Texture variables.
GLuint towerTx, secondTx, wallTx, planeTex, gateTx, groundTx, headTx, mazeTx;
GLint width, height, bitDepth;

// Light variables.
AmbientLight aLight(glm::vec3(1.0f, 1.0f, 1.0f),	// Ambient colour.
	1.0f);							// Ambient strength.
DirectionalLight dLight(glm::vec3(0.0f, -1.0f, 0.0f), // Direction.
	glm::vec3(1.0f, 1.0f, 1.0f),  // Diffuse colour.
	0.0f);						  // Diffuse strength.
PointLight pLight(glm::vec3(2.0f, 4.2f, -3.0f),	// Position.
	1.0f, 0.7f, 1.8f,				// Constant, Linear, Exponent.
	glm::vec3(0.0f, 0.0f, 1.0f),	// Diffuse colour.
	10.0f);						// Diffuse strength.

PointLight pLight1(glm::vec3(2.0f, 4.2f, -18.0f),	// Position.
	1.0f, 0.7f, 1.8f,				// Constant, Linear, Exponent.
	glm::vec3(1.0f, 0.0f, 0.0f),	// Diffuse colour.
	10.0f);						// Diffuse strength.
Material mat = { 0.1f, 32 }; // Alternate way to construct an object.

void timer(int);

void resetView()
{
	position = glm::vec3(5.0f, 3.0f, 10.0f);
	frontVec = glm::vec3(0.0f, 0.0f, -1.0f);
	worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	pitch = 0.0f;
	yaw = -90.0f;
	// View will now get set only in transformObject
}

// Shapes. Recommend putting in a map
Cube g_cube(3);
Prism g_prism(12);
Plane g_plane;
Grid g_grid(10,3); // New UV scale parameter. Works with texture now.
Gate g_gate;
Ground g_ground;
ConeTwo g_head(12);
Cube g_maze(1);
void init(void)
{
	srand((unsigned)time(NULL));
	//Specifying the name of vertex and fragment shaders.
	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles2.vert" },
		{ GL_FRAGMENT_SHADER, "triangles2.frag" },
		{ GL_NONE, NULL }
	};

	//Loading and compiling shaders
	program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up

	modelID = glGetUniformLocation(program, "model");
	projID = glGetUniformLocation(program, "projection");
	viewID = glGetUniformLocation(program, "view");

	// Projection matrix : 45∞ Field of View, aspect ratio, display range : 0.1 unit <-> 100 units
	Projection = glm::perspective(glm::radians(45.0f), 1.0f / 1.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	// Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	resetView();

	// Image loading.
	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load("dirt.jpg", &width, &height, &bitDepth, 0);
	if (!image) cout << "Unable to load file!" << endl;

	glGenTextures(1, &planeTex);
	glBindTexture(GL_TEXTURE_2D, planeTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);
	// Prism
	unsigned char* image2 = stbi_load("stone2.jpg", &width, &height, &bitDepth, 0);
	if (!image2) cout << "Unable to load file!" << endl;

	glGenTextures(1, &wallTx);
	glBindTexture(GL_TEXTURE_2D, wallTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image2);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image2);

	unsigned char* image3 = stbi_load("stone2.jpg", &width, &height, &bitDepth, 0);
	if (!image3) cout << "Unable to load file!" << endl;

	glGenTextures(1, &towerTx);
	glBindTexture(GL_TEXTURE_2D, towerTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image3);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image3);

	//gate
	unsigned char* image4 = stbi_load("gate.png", &width, &height, &bitDepth, 0);
	if (!image3) cout << "Unable to load file!" << endl;

	glGenTextures(1, &gateTx);
	glBindTexture(GL_TEXTURE_2D, gateTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image4);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image4);

	unsigned char* image5 = stbi_load("dirt.jpg", &width, &height, &bitDepth, 0);
	if (!image5) cout << "Unable to load file!" << endl;
	//gorund
	glGenTextures(1, &groundTx);
	glBindTexture(GL_TEXTURE_2D, groundTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image5);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image5);

	//tower head
	unsigned char* image6 = stbi_load("stone2.jpg", &width, &height, &bitDepth, 0);
	if (!image6) cout << "Unable to load file!" << endl;

	glGenTextures(1, &headTx);
	glBindTexture(GL_TEXTURE_2D, headTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image6);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image6);

	//Maze blocks
	unsigned char* image7 = stbi_load("grass.jpg", &width, &height, &bitDepth, 0);
	if (!image7) cout << "Unable to load file!" << endl;

	glGenTextures(1, &mazeTx);
	glBindTexture(GL_TEXTURE_2D, mazeTx);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image7);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image7);




	glUniform1i(glGetUniformLocation(program, "texture0"), 0);



	

	// Setting ambient Light.
	glUniform3f(glGetUniformLocation(program, "aLight.ambientColour"), aLight.ambientColour.x, aLight.ambientColour.y, aLight.ambientColour.z);
	glUniform1f(glGetUniformLocation(program, "aLight.ambientStrength"), aLight.ambientStrength);
	// Setting directional light.
	glUniform3f(glGetUniformLocation(program, "dLight.base.diffuseColour"), dLight.diffuseColour.x, dLight.diffuseColour.y, dLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "dLight.base.diffuseStrength"), dLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "dLight.direction"), dLight.direction.x, dLight.direction.y, dLight.direction.z);

	// Setting point light.
	glUniform3f(glGetUniformLocation(program, "pLight.base.diffuseColour"), pLight.diffuseColour.x, pLight.diffuseColour.y, pLight.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLight.base.diffuseStrength"), pLight.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "pLight.position"), pLight.position.x, pLight.position.y, pLight.position.z);
	glUniform1f(glGetUniformLocation(program, "pLight.constant"), pLight.constant);
	glUniform1f(glGetUniformLocation(program, "pLight.linear"), pLight.linear);
	glUniform1f(glGetUniformLocation(program, "pLight.exponent"), pLight.exponent);
	//sec
	glUniform3f(glGetUniformLocation(program, "pLight1.base.diffuseColour"), pLight1.diffuseColour.x, pLight1.diffuseColour.y, pLight1.diffuseColour.z);
	glUniform1f(glGetUniformLocation(program, "pLight1.base.diffuseStrength"), pLight1.diffuseStrength);

	glUniform3f(glGetUniformLocation(program, "pLight1.position"), pLight1.position.x, pLight1.position.y, pLight1.position.z);
	glUniform1f(glGetUniformLocation(program, "pLight1.constant"), pLight1.constant);
	glUniform1f(glGetUniformLocation(program, "pLight1.linear"), pLight1.linear);
	glUniform1f(glGetUniformLocation(program, "pLight1.exponent"), pLight1.exponent);
	vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	ibo = 0;
	glGenBuffers(1, &ibo);

	points_vbo = 0;
	glGenBuffers(1, &points_vbo);

	colors_vbo = 0;
	glGenBuffers(1, &colors_vbo);

	uv_vbo = 0;
	glGenBuffers(1, &uv_vbo);

	normals_vbo = 0;
	glGenBuffers(1, &normals_vbo);

	glBindVertexArray(0); // Can optionally unbind the vertex array to avoid modification.


	glClearColor(0.0, 0.0, 0.0, 0.0);
	// Enable depth test and blend.
	glEnable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Enable smoothing.
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	// Enable face culling.
	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
	//glCullFace(GL_BACK);

	timer(0);
}

//---------------------------------------------------------------------
//
// calculateView
//
void calculateView()
{
	frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec.y = sin(glm::radians(pitch));
	frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	frontVec = glm::normalize(frontVec);
	rightVec = glm::normalize(glm::cross(frontVec, worldUp));
	upVec = glm::normalize(glm::cross(rightVec, frontVec));

	View = glm::lookAt(
		position, // Camera position
		position + frontVec, // Look target
		upVec); // Up vector
	glUniform3f(glGetUniformLocation(program, "eyePosition"), position.x, position.y, position.z);
}

//---------------------------------------------------------------------
//
// transformModel
//
void transformObject(glm::vec3 scale, glm::vec3 rotationAxis, float rotationAngle, glm::vec3 translation) {
	glm::mat4 Model;
	Model = glm::mat4(1.0f);
	Model = glm::translate(Model, translation);
	Model = glm::rotate(Model, glm::radians(rotationAngle), rotationAxis);
	Model = glm::scale(Model, scale);
	
	calculateView();
	glUniformMatrix4fv(modelID, 1, GL_FALSE, &Model[0][0]);
	glUniformMatrix4fv(viewID, 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(projID, 1, GL_FALSE, &Projection[0][0]);
}

//---------------------------------------------------------------------
//
// display
//
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vao);
	

	glBindTexture(GL_TEXTURE_2D, planeTex);
	g_plane.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(0.0f, 0.0f, 0.0f));
	glDrawElements(GL_TRIANGLES, g_plane.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glUniform3f(glGetUniformLocation(program, "pLight.position"), pLight.position.x, pLight.position.y, pLight.position.z);
	glUniform3f(glGetUniformLocation(program, "pLight1.position"), pLight1.position.x, pLight1.position.y, pLight1.position.z);

	//first wall
	for (int x = 7; x <= 35; x++) {
		if (x % 2 == 0) {
			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 3.5f, 1.0f), X_AXIS, 0.0f, glm::vec3(x, 0.0f, -4.0f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
			glBindTexture(GL_TEXTURE_2D, wallTx);
		}
		else
			glBindTexture(GL_TEXTURE_2D, wallTx);
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(x, 0.0f, -4.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, wallTx);
	}
	//second wall
	for (int x = 6; x <= 35; x++) {
		if (x % 2 == 0) {
			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 3.5f, 1.0f), X_AXIS, 0.0f, glm::vec3(x, 0.0f, -27.0f));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
			glBindTexture(GL_TEXTURE_2D, wallTx);
		}
		else
			glBindTexture(GL_TEXTURE_2D, wallTx);
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(x, 0.0f, -27.0f));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, wallTx);
	}
	//third wall
	for (int x = -5; x >= -26; x--) {
		if (x % 2 == 0) {
			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 3.5f, 1.0f), X_AXIS, 0.0f, glm::vec3(6.0f, 0.0f, x));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
			glBindTexture(GL_TEXTURE_2D, wallTx);
		}
		else
			glBindTexture(GL_TEXTURE_2D, wallTx);
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(6.0f, 0.0f, x));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, wallTx);
	}
	//forth wall:
	for (int x = -5; x >= -13; x--) {
		if (x % 2 != 0) {
			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 3.5f, 1.0f), X_AXIS, 0.0f, glm::vec3(35.0f, 0.0f, x));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
			glBindTexture(GL_TEXTURE_2D, wallTx);
		}
		else
			glBindTexture(GL_TEXTURE_2D, wallTx);
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(35.0f, 0.0f, x));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, wallTx);
	}
	for (int x = -18; x >= -26; x--) {
		if (x % 2 == 0) {
			glBindTexture(GL_TEXTURE_2D, wallTx);
			g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
			transformObject(glm::vec3(1.0f, 3.5f, 1.0f), X_AXIS, 0.0f, glm::vec3(35.0f, 0.0f, x));
			glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
			glBindTexture(GL_TEXTURE_2D, wallTx);
		}
		else
			glBindTexture(GL_TEXTURE_2D, wallTx);
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 3.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(35.0f, 0.0f, x));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, wallTx);
	}

	//tower1
	glBindTexture(GL_TEXTURE_2D, towerTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 5.5f, 1.5f), X_AXIS, 0.0f, glm::vec3(5.85f, 0.0f, -4.2f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);


	glBindTexture(GL_TEXTURE_2D, headTx);
	g_head.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.5f, 2.0f), X_AXIS, 0.0f, glm::vec3(5.6f, 5.5f, -4.5f));
	glDrawElements(GL_TRIANGLES, g_head.NumIndices(), GL_UNSIGNED_SHORT, 0);


	//tower2
	glBindTexture(GL_TEXTURE_2D, towerTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 5.5f, 1.5f), X_AXIS, 0.0f, glm::vec3(5.85f, 0.0f, -27.2f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);


	glBindTexture(GL_TEXTURE_2D, headTx);
	g_head.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.5f, 2.0f), X_AXIS, 0.0f, glm::vec3(5.6f, 5.5f, -27.5f));
	glDrawElements(GL_TRIANGLES, g_head.NumIndices(), GL_UNSIGNED_SHORT, 0);


	//tower2
	glBindTexture(GL_TEXTURE_2D, towerTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 5.5f, 1.5f), X_AXIS, 0.0f, glm::vec3(34.85f, 0.0f, -4.2f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);

	glBindTexture(GL_TEXTURE_2D, headTx);
	g_head.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.5f, 2.0f), X_AXIS, 0.0f, glm::vec3(34.6f, 5.5f, -4.5f));
	glDrawElements(GL_TRIANGLES, g_head.NumIndices(), GL_UNSIGNED_SHORT, 0);


	//tower2

	glBindTexture(GL_TEXTURE_2D, towerTx);
	g_prism.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.5f, 5.5f, 1.5f), X_AXIS, 0.0f, glm::vec3(34.85f, 0.0f, -27.2f));
	glDrawElements(GL_TRIANGLES, g_prism.NumIndices(), GL_UNSIGNED_SHORT, 0);



	glBindTexture(GL_TEXTURE_2D, headTx);
	g_head.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(2.0f, 1.5f, 2.0f), X_AXIS, 0.0f, glm::vec3(34.6f, 5.5f, -27.5f));
	glDrawElements(GL_TRIANGLES, g_head.NumIndices(), GL_UNSIGNED_SHORT, 0);


	//ground




	glBindTexture(GL_TEXTURE_2D, groundTx);
	g_ground.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(6.0f, 1.0f, -4.0f));
	glDrawElements(GL_TRIANGLES, g_ground.NumIndices(), GL_UNSIGNED_SHORT, 0);









	//============================================================MAZE==============================================================   mazeTx
	if(1==1){
	//13-18
	for (int x = -12; x >= -17; x--) {
		glBindTexture(GL_TEXTURE_2D, mazeTx);
		g_maze.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(7.0f, 1.0f, x));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);

	}
	for (int x = -4; x >= -25; x--) {
		if (x < -12 && x > -17) {

		}
		else {
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(8.0f, 1.0f, x));
			glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}
	for (int x = -4; x >= -25; x--) {
		if (x < -12 && x > -17) {

		}
		else {
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(10.0f, 1.0f, x));
			glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}

	}
	for (int x = 10; x <= 12; x++) {

		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -12.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int x = 10; x <= 12; x++) {

		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -17.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);


	}
	for (int x = -6; x >= -22; x--) {
		if (x < -12 && x > -17) {

		}
		else {
		
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(12.0f, 1.0f, x));
			glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}
	for (int x = 12; x <= 34; x++) {

	
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -6.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);


	}
	for (int x = 12; x <= 34; x++) {


		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -23.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);


	}

	for (int x = -9; x >= -20; x--) {
		if (x <= -14 && x >= -15) {

		}
		else {
		
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(15.0f, 1.0f, x));
			glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}

	}

	for (int x = 16; x <= 19; x++) {

		
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -9.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int x = 16; x <= 19; x++) {

	
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -20.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}

	for (int x = -9; x >= -20; x--) {

		
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(19.0f, 1.0f, x));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);

	}
	for (int x = -6; x >= -22; x--) {
		if (x < -13 && x > -16) {

		}
		else {
	
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(21.0f, 1.0f, x));
			glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}
	for (int x = 22; x <= 23; x++) {

	
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -16.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int x = 22; x <= 23; x++) {

	
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -13.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}

	for (int x = -9; x >= -20; x--) {
		if (x <= -12 && x >= -17) {

		}
		else {
		
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(23.0f, 1.0f, x));
			glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}

	for (int x = 23; x <= 25; x++) {

	
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -9.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}

	for (int x = 23; x <= 25; x++) {

	
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -20.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}


	for (int x = -9; x >= -20; x--) {
		if (x <= -13 && x >= -16) {

		}
		else {
	
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(25.0f, 1.0f, x));
			glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}

	for (int x = 25; x <= 31; x++) {

		
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -17.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int x = 25; x <= 31; x++) {


		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(x, 1.0f, -12.0f));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
	}
	for (int x = -12; x >= -17; x--) {

	
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(31.0f, 1.0f, x));
		glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);

	}

	for (int x = -7; x >= -23; x--) {
		if (x <= -13 && x >= -16) {

		}
		else {
		
			transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, -90.0f, glm::vec3(34.0f, 1.0f, x));
			glDrawElements(GL_TRIANGLES, g_maze.NumIndices(), GL_UNSIGNED_SHORT, 0);
		}
	}

}

	//================================================================================================================================




	//Gate
	glBindTexture(GL_TEXTURE_2D, gateTx);
	g_gate.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
	transformObject(glm::vec3(4.4f, 2.0f, 1.0f), Y_AXIS, 90.0f, glm::vec3(36.0f, 1.0f, -12.8f));
	glDrawElements(GL_TRIANGLES, g_gate.NumIndices(), GL_UNSIGNED_SHORT, 0);




	//stairs
	for (int x = -14; x >= -17; x--) {
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 0.75f, 1.0f), X_AXIS, 0.0f, glm::vec3(37.0f, 0.0f, x));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, wallTx);

	}
	for (int x = -14; x >= -17; x--) {
	
		
		transformObject(glm::vec3(1.0f, 1.0f, 1.0f), X_AXIS, 0.0f, glm::vec3(36.0f, 0.0f, x));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);


	}
	for (int x = -14; x >= -17; x--) {
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 0.5f, 1.0f), X_AXIS, 0.0f, glm::vec3(38.0f, 0.0f, x));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, wallTx);

	}
	for (int x = -14; x >= -17; x--) {
		glBindTexture(GL_TEXTURE_2D, wallTx);
		g_cube.BufferShape(&ibo, &points_vbo, &colors_vbo, &uv_vbo, &normals_vbo, program);
		transformObject(glm::vec3(1.0f, 0.25f, 1.0f), X_AXIS, 0.0f, glm::vec3(39.0f, 0.0f, x));
		glDrawElements(GL_TRIANGLES, g_cube.NumIndices(), GL_UNSIGNED_SHORT, 0);
		glBindTexture(GL_TEXTURE_2D, wallTx);

	}















	glBindVertexArray(0); // Done writing.
	glutSwapBuffers(); // Now for a potentially smoother render.
}

void parseKeys()
{
	if (keys & KEY_FORWARD)
		position += frontVec * MOVESPEED;
	else if (keys & KEY_BACKWARD)
		position -= frontVec * MOVESPEED;
	if (keys & KEY_LEFT)
		position -= rightVec * MOVESPEED;
	else if (keys & KEY_RIGHT)
		position += rightVec * MOVESPEED;
	if (keys & KEY_UP)
		position.y += MOVESPEED;
	else if (keys & KEY_DOWN)
		position.y -= MOVESPEED;
}

void timer(int) { // essentially our update()
	parseKeys();
	glutPostRedisplay();
	glutTimerFunc(1000/FPS, timer, 0); // 60 FPS or 16.67ms.
}

//---------------------------------------------------------------------
//
// keyDown
//
void keyDown(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD; break;
	case 's':
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD; break;
	case 'a':
		if (!(keys & KEY_LEFT))
			keys |= KEY_LEFT; break;
	case 'd':
		if (!(keys & KEY_RIGHT))
			keys |= KEY_RIGHT; break;
	case 'r':
		if (!(keys & KEY_UP))
			keys |= KEY_UP; break;
	case 'f':
		if (!(keys & KEY_DOWN))
			keys |= KEY_DOWN; break;
	}
}

void keyDownSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		if (!(keys & KEY_FORWARD))
			keys |= KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		if (!(keys & KEY_BACKWARD))
			keys |= KEY_BACKWARD;
	}
}

void keyUp(unsigned char key, int x, int y) // x and y is mouse location upon key press.
{
	switch (key)
	{
	case 'w':
		keys &= ~KEY_FORWARD; break;
	case 's':
		keys &= ~KEY_BACKWARD; break;
	case 'a':
		keys &= ~KEY_LEFT; break;
	case 'd':
		keys &= ~KEY_RIGHT; break;
	case 'r':
		keys &= ~KEY_UP; break;
	case 'f':
		keys &= ~KEY_DOWN; break;
	case ' ':
		resetView();
	}
}

void keyUpSpec(int key, int x, int y) // x and y is mouse location upon key press.
{
	if (key == GLUT_KEY_UP)
	{
		keys &= ~KEY_FORWARD;
	}
	else if (key == GLUT_KEY_DOWN)
	{
		keys &= ~KEY_BACKWARD;
	}
}

void mouseMove(int x, int y)
{
	if (keys & KEY_MOUSECLICKED)
	{
		pitch += (GLfloat)((y - lastY) * TURNSPEED);
		yaw -= (GLfloat)((x - lastX) * TURNSPEED);
		lastY = y;
		lastX = x;
	}
}

void mouseClick(int btn, int state, int x, int y)
{
	if (state == 0)
	{
		lastX = x;
		lastY = y;
		keys |= KEY_MOUSECLICKED; // Flip flag to true
		glutSetCursor(GLUT_CURSOR_NONE);
		//cout << "Mouse clicked." << endl;
	}
	else
	{
		keys &= ~KEY_MOUSECLICKED; // Reset flag to false
		glutSetCursor(GLUT_CURSOR_INHERIT);
		//cout << "Mouse released." << endl;
	}
}

void clean()
{
	cout << "Cleaning up!" << endl;
	

}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutSetOption(GLUT_MULTISAMPLE, 8);
	glutInitWindowSize(1024, 1024);
	glutCreateWindow("GAME2012 - Week 7");

	glewInit();	//Initializes the glew and prepares the drawing pipeline.
	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyDown);
	glutSpecialFunc(keyDownSpec);
	glutKeyboardUpFunc(keyUp); // New function for third example.
	glutSpecialUpFunc(keyUpSpec);

	glutMouseFunc(mouseClick);
	glutMotionFunc(mouseMove); // Requires click to register.
	
	atexit(clean); // This GLUT function calls specified function before terminating program. Useful!

	glutMainLoop();

	return 0;
}
