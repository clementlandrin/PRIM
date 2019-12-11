// Explosion.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include <string> 
#pragma comment (lib, "glew32s.lib")
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glut.h>

#include "Particle.hpp"
#include "Fluid.hpp"
#include "OctreeNode.hpp"
#include "Cell.hpp"
#include "RegularGrid.h"
#include "ShaderProgram.h"

#define VISCOSITY 1.0f
#define DENSITY 1.0f
#define PARTICLE_NUMBER 512
#define FLUID_DIMENSION 0.01f
#define CUBE_SIZE 0.5f
#define RESOLUTION 8

struct LightSource
{
	glm::vec4 position;
	glm::vec4 color_and_intensity;
};

glm::mat4 model_view_matrix = glm::mat4(
	0.57735, -0.33333, 0.57735, 0.00000,
	0.00000, 0.66667, 0.57735, 0.00000,
	-0.57735, -0.33333, 0.57735, 0.00000,
	0.00000, 0.00000, -17, 1.00000);
glm::mat4 projection_matrix = glm::mat4(
	15.00000, 0.00000, 0.00000, 0.00000,
	0.00000, 15.00000, 0.00000, 0.00000,
	0.00000, 0.00000, -1.00020, -1.00000,
	0.00000, 0.00000, -10.00100, 0.00000);

Fluid* fluid;

std::vector<float> vertexPositions;
std::vector<float> squarePositions;
std::vector<float> squareNormals;

Cell* scene;
OctreeNode* octreeRoot;

GLuint m_vboID;
GLuint particleUBO;

GLuint m_squareVboID;
GLuint m_squareNormalVBO;
GLuint m_vao;

GLuint particleProgramID;
GLuint lightingProgramID;

LightSource lightSources[PARTICLE_NUMBER];

#define TEST_OPENGL_ERROR()                                                             \
  do {		  							\
    GLenum err = glGetError(); 					                        \
    if (err != GL_NO_ERROR) std::cerr << "OpenGL ERROR! " << __LINE__ << std::endl;      \
  } while(0)

void updatePositions()
{
	vertexPositions.resize(fluid->GetParticles().size() * 3);

	for (int i = 0; i < fluid->GetParticles().size(); i++)
	{
		vertexPositions[3 * i] = fluid->GetParticles()[i]->GetPosition().x;
		vertexPositions[3 * i + 1] = fluid->GetParticles()[i]->GetPosition().y;
		vertexPositions[3 * i + 2] = fluid->GetParticles()[i]->GetPosition().z;
	}
}

void initPositions()
{
	vertexPositions.resize(fluid->GetParticles().size() * 3);

	updatePositions();
}

void initBuffer()
{
	glGenBuffers(1, &m_vboID); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID); TEST_OPENGL_ERROR();
	glBufferData(GL_ARRAY_BUFFER, vertexPositions.size() * sizeof(float), &(vertexPositions[0]), GL_DYNAMIC_DRAW); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();

	///////////////////////////////////////

	glCreateBuffers(1, &m_squareVboID); // Generate a GPU buffer to store the positions of the vertices
	size_t vertexBufferSize = sizeof(float) * squarePositions.size(); // Gather the size of the buffer from the CPU-side vector
	glNamedBufferStorage(m_squareVboID, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); // Create a data store on the GPU
	glNamedBufferSubData(m_squareVboID, 0, vertexBufferSize, squarePositions.data()); // Fill the data store from a CPU array

	glCreateBuffers(1, &m_squareNormalVBO); // Same for normal
	glNamedBufferStorage(m_squareNormalVBO, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferSubData(m_squareNormalVBO, 0, vertexBufferSize, squareNormals.data());

	glCreateVertexArrays(1, &m_vao);  TEST_OPENGL_ERROR();// Create a single handle that joins together attributes (vertex positions, normals) and connectivity (triangles indices)
	glBindVertexArray(m_vao); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squareVboID); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squareNormalVBO); TEST_OPENGL_ERROR();
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();

	glBindVertexArray(0); TEST_OPENGL_ERROR(); // Desactive the VAO just created. Will be activated at rendering time.

	////////////////////////////////
	glGenBuffers(1, &particleUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, particleUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightSource) * PARTICLE_NUMBER + sizeof(int), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void generateParticles()
{
	fluid->GenerateParticlesUniformly(PARTICLE_NUMBER, glm::vec3(0.0), FLUID_DIMENSION, FLUID_DIMENSION, FLUID_DIMENSION);
}

void initFluid()
{
	fluid = new Fluid(VISCOSITY, DENSITY);
	generateParticles();
}

void bottomFace(int offset, float size)
{
	squarePositions[0 + offset] = 1.0f * size;
	squarePositions[1 + offset] = -1.0f * size;
	squarePositions[2 + offset] = -1.0f * size;

	squarePositions[3 + offset] = -1.0f * size;
	squarePositions[4 + offset] = -1.0f * size;
	squarePositions[5 + offset] = -1.0f * size;

	squarePositions[6 + offset] = -1.0f * size;
	squarePositions[7 + offset] = -1.0f * size;
	squarePositions[8 + offset] = 1.0f * size;

	squarePositions[9 + offset] = 1.0f * size;
	squarePositions[10 + offset] = -1.0f * size;
	squarePositions[11 + offset] = -1.0f * size;

	squarePositions[12 + offset] = -1.0f * size;
	squarePositions[13 + offset] = -1.0f * size;
	squarePositions[14 + offset] = 1.0f * size;

	squarePositions[15 + offset] = 1.0f * size;
	squarePositions[16 + offset] = -1.0f * size;
	squarePositions[17 + offset] = 1.0f * size;

	for (int i = 0; i < 6; i++)
	{
		squareNormals[3 * i + offset] = 0.0f;
		squareNormals[3 * i + offset + 1] = 1.0f;
		squareNormals[3 * i + offset + 2] = 0.0f;
	}
}

void leftFace(int offset, float size)
{
	squarePositions[0 + offset] = -1.0f * size;
	squarePositions[1 + offset] = 1.0f * size;
	squarePositions[2 + offset] = -1.0f * size;

	squarePositions[3 + offset] = -1.0f * size;
	squarePositions[4 + offset] = 1.0f * size;
	squarePositions[5 + offset] = 1.0f * size;

	squarePositions[6 + offset] = -1.0f * size;
	squarePositions[7 + offset] = -1.0f * size;
	squarePositions[8 + offset] = -1.0f * size;

	squarePositions[9 + offset] = -1.0f * size;
	squarePositions[10 + offset] = -1.0f * size;
	squarePositions[11 + offset] = -1.0f * size;

	squarePositions[12 + offset] = -1.0f * size;
	squarePositions[13 + offset] = 1.0f * size;
	squarePositions[14 + offset] = 1.0f * size;

	squarePositions[15 + offset] = -1.0f * size;
	squarePositions[16 + offset] = -1.0f * size;
	squarePositions[17 + offset] = 1.0f * size;

	for (int i = 0; i < 6; i++)
	{
		squareNormals[3 * i + offset] = 1.0f;
		squareNormals[3 * i + offset + 1] = 0.0f;
		squareNormals[3 * i + offset + 2] = 0.0f;
	}
}

void rightFace(int offset, float size)
{
	squarePositions[0 + offset] = 1.0f * size;
	squarePositions[1 + offset] = 1.0f * size;
	squarePositions[2 + offset] = -1.0f * size;

	squarePositions[3 + offset] = 1.0f * size;
	squarePositions[4 + offset] = -1.0f * size;
	squarePositions[5 + offset] = -1.0f * size;

	squarePositions[6 + offset] = 1.0f * size;
	squarePositions[7 + offset] = 1.0f * size;
	squarePositions[8 + offset] = 1.0f * size;

	squarePositions[9 + offset] = 1.0f * size;
	squarePositions[10 + offset] = 1.0f * size;
	squarePositions[11 + offset] = 1.0f * size;

	squarePositions[12 + offset] = 1.0f * size;
	squarePositions[13 + offset] = -1.0f * size;
	squarePositions[14 + offset] = -1.0f * size;

	squarePositions[15 + offset] = 1.0f * size;
	squarePositions[16 + offset] = -1.0f * size;
	squarePositions[17 + offset] = 1.0f * size;

	for (int i = 0; i < 6; i++)
	{
		squareNormals[3 * i + offset] = -1.0f;
		squareNormals[3 * i + offset + 1] = 0.0f;
		squareNormals[3 * i + offset + 2] = 0.0f;
	}
}

void topFace(int offset, float size)
{
	squarePositions[0 + offset] = -1.0f * size;
	squarePositions[1 + offset] = 1.0f * size;
	squarePositions[2 + offset] = 1.0f * size;

	squarePositions[3 + offset] = -1.0f * size;
	squarePositions[4 + offset] = 1.0f * size;
	squarePositions[5 + offset] = -1.0f * size;

	squarePositions[6 + offset] = 1.0f * size;
	squarePositions[7 + offset] = 1.0f * size;
	squarePositions[8 + offset] = -1.0f * size;

	squarePositions[9 + offset] = -1.0f * size;
	squarePositions[10 + offset] = 1.0f * size;
	squarePositions[11 + offset] = 1.0f * size;

	squarePositions[12 + offset] = 1.0f * size;
	squarePositions[13 + offset] = 1.0f * size;
	squarePositions[14 + offset] = -1.0f * size;

	squarePositions[15 + offset] = 1.0f * size;
	squarePositions[16 + offset] = 1.0f * size;
	squarePositions[17 + offset] = 1.0f * size;

	for (int i = 0; i < 6; i++)
	{
		squareNormals[3 * i + offset] = 0.0f;
		squareNormals[3 * i + offset + 1] = -1.0f;
		squareNormals[3 * i + offset + 2] = 0.0f;
	}
}

void backFace(int offset, float size)
{
	squarePositions[0 + offset] = -1.0f * size;
	squarePositions[1 + offset] = 1.0f * size;
	squarePositions[2 + offset] = -1.0f * size;

	squarePositions[3 + offset] = -1.0f * size;
	squarePositions[4 + offset] = -1.0f * size;
	squarePositions[5 + offset] = -1.0f * size;

	squarePositions[6 + offset] = 1.0f * size;
	squarePositions[7 + offset] = -1.0f * size;
	squarePositions[8 + offset] = -1.0f * size;

	squarePositions[9 + offset] = -1.0f * size;
	squarePositions[10 + offset] = 1.0f * size;
	squarePositions[11 + offset] = -1.0f * size;

	squarePositions[12 + offset] = 1.0f * size;
	squarePositions[13 + offset] = -1.0f * size;
	squarePositions[14 + offset] = -1.0f * size;

	squarePositions[15 + offset] = 1.0f * size;
	squarePositions[16 + offset] = 1.0f * size;
	squarePositions[17 + offset] = -1.0f * size;

	for (int i = 0; i < 6; i++)
	{
		squareNormals[3 * i + offset] = 0.0f;
		squareNormals[3 * i + offset + 1] = 0.0f;
		squareNormals[3 * i + offset + 2] = 1.0f;
	}
}

void frontFace(int offset, float size)
{
	squarePositions[0 + offset] = 1.0f * size;
	squarePositions[1 + offset] = -1.0f * size;
	squarePositions[2 + offset] = 1.0f * size;

	squarePositions[3 + offset] = -1.0f * size;
	squarePositions[4 + offset] = -1.0f * size;
	squarePositions[5 + offset] = 1.0f * size;

	squarePositions[6 + offset] = 1.0f * size;
	squarePositions[7 + offset] = 1.0f * size;
	squarePositions[8 + offset] = 1.0f * size;

	squarePositions[9 + offset] = 1.0f * size;
	squarePositions[10 + offset] = 1.0f * size;
	squarePositions[11 + offset] = 1.0f * size;

	squarePositions[12 + offset] = -1.0f * size;
	squarePositions[13 + offset] = -1.0f * size;
	squarePositions[14 + offset] = 1.0f * size;

	squarePositions[15 + offset] = -1.0f * size;
	squarePositions[16 + offset] = 1.0f * size;
	squarePositions[17 + offset] = 1.0f * size;

	for (int i = 0; i < 6; i++)
	{
		squareNormals[3 * i + offset] = 0.0f;
		squareNormals[3 * i + offset + 1] = 0.0f;
		squareNormals[3 * i + offset + 2] = -1.0f;
	}
}

void createSquare()
{
	int vertexPerFace = 2 * 3 * 3;
	squarePositions.resize(6 * vertexPerFace);
	squareNormals.resize(6 * vertexPerFace);

	bottomFace(0 * vertexPerFace, CUBE_SIZE);

	leftFace(vertexPerFace, CUBE_SIZE);

	rightFace(2 * vertexPerFace, CUBE_SIZE);

	topFace(3 * vertexPerFace, CUBE_SIZE);

	backFace(4 * vertexPerFace, CUBE_SIZE);

	frontFace(5 * vertexPerFace, CUBE_SIZE);
}

void initScene()
{
	createSquare();
	scene = new Cell(nullptr, glm::vec3(-CUBE_SIZE / 2.0), CUBE_SIZE, CUBE_SIZE, CUBE_SIZE);
	scene->SetParticles(fluid->GetParticles());
}

void initShaders()
{
	// Create and compile our GLSL program from the shaders
	particleProgramID = ShaderProgram::LoadShaders("Resources/vertex.shd", "Resources/fragment.shd");
	lightingProgramID = ShaderProgram::LoadShaders("Resources/lighting_vertex.shd", "Resources/lighting_fragment.shd");

	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("model_view_matrix", model_view_matrix, lightingProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("projection_matrix", projection_matrix, lightingProgramID); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();
}

void initOctree()
{
	int power_index = 0;
	float temp = RESOLUTION;

	while (temp > 1.0 && power_index < 3)
	{
		temp = temp / 2.0;
		power_index = power_index + 1;
	}

	octreeRoot = new OctreeNode();
	octreeRoot->BuildOctree(0, power_index - 1, scene);
}

int init(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow(argv[0]); TEST_OPENGL_ERROR();
	glutReshapeWindow(1024, 1024); TEST_OPENGL_ERROR();
	glPointSize(3.0);
	if (glewInit() != GLEW_OK)
	{
		return 1;
	}
	glEnable(GL_DEPTH_TEST); TEST_OPENGL_ERROR();
	glCullFace(GL_BACK); TEST_OPENGL_ERROR();
	glEnable(GL_CULL_FACE); TEST_OPENGL_ERROR();
	initFluid();
	initPositions();
	initScene();
	initOctree();
	initBuffer();
	initShaders();
	return 0;
}

void update()
{
	fluid->UpdateParticlePositions(0.01 * CUBE_SIZE, CUBE_SIZE);

	if (fluid->GetParticles().size() == 0)
	{
		generateParticles();
	}

	updatePositions();

	scene->SetParticles(fluid->GetParticles());
	delete octreeRoot;
	initOctree();
}

void render()
{
	update();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); TEST_OPENGL_ERROR();

	glUseProgram(particleProgramID); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexPositions.size() * sizeof(float), &(vertexPositions[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();
	glDrawArrays(GL_POINTS, 0, vertexPositions.size()/3); TEST_OPENGL_ERROR();

	glDisableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();

	////////////////////////////////////////////////////////////////

	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();

	for (int i = 0; i < vertexPositions.size()/3; i++)
	{
		lightSources[i].position = glm::vec4(vertexPositions[3 * i], vertexPositions[3 * i + 1], vertexPositions[3 * i + 2], 1.0);
		lightSources[i].color_and_intensity = glm::vec4(1.0);
	}

	glBindBuffer(GL_UNIFORM_BUFFER, particleUBO);
	int actualNumberOfParticles[1] = { vertexPositions.size() };
	GLsizei sifeOfArray = vertexPositions.size() / 3 * sizeof(LightSource);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sifeOfArray, &(lightSources[0])); TEST_OPENGL_ERROR();
	glBufferSubData(GL_UNIFORM_BUFFER, sifeOfArray, sizeof(int), &(actualNumberOfParticles)); TEST_OPENGL_ERROR();

	GLuint lightSourcesBlockIdx = glGetUniformBlockIndex(lightingProgramID, "lightSourcesBlock");
	glUniformBlockBinding(lightingProgramID, lightSourcesBlockIdx, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, particleUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindVertexArray(m_vao); TEST_OPENGL_ERROR(); // Activate the VAO storing geometry data
	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squareVboID); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, squarePositions.size() * sizeof(float), &(squarePositions[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squareNormalVBO); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, squareNormals.size() * sizeof(float), &(squareNormals[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * 0, (void*)0); TEST_OPENGL_ERROR();

	glDrawArrays(GL_TRIANGLES, 0, squarePositions.size()/3); TEST_OPENGL_ERROR();

	glDisableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glDisableVertexAttribArray(1); TEST_OPENGL_ERROR();

	glutSwapBuffers();

	glutPostRedisplay();
}

void clear()
{
	glDeleteBuffers(1, &m_vboID); TEST_OPENGL_ERROR();
	delete scene;
	delete octreeRoot;
	delete fluid;
}

int main(int argc, char **argv) {
	if (init(argc, argv) != 0)
	{
		return 1;
	}

	glutDisplayFunc(render);
	glutMainLoop();

	clear();
	return 0;
}