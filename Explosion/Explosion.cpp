// Explosion.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include <string> 
#pragma comment (lib, "glew32s.lib")
#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glut.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>


#include "Particle.hpp"
#include "Fluid.hpp"
#include "OctreeNode.hpp"
#include "Cell.hpp"
#include "RegularGrid.h"
#include "ShaderProgram.h"

#define GLUT_SPACE 32
#define GLUT_ESC 27
#define GLUT_ENTER 13

#define M_PI 3.14159

#define VISCOSITY 10000.0f
#define DENSITY 1.0f
#define PARTICLE_NUMBER 5000
#define FLUID_PROPORTION_IN_CUBE 0.05f
#define CUBE_SIZE 0.75f
#define RESOLUTION 4
#define SIMULATION_MAX_DURATION 0.0f//20000.0f
std::ifstream in;
std::ofstream out;

bool pause = false;

int frameNumber = 0;

bool shouldGenerateNewParticlesEachFrame = false;
bool shouldRenderLighting = true;
bool shouldRegisterSimulation = false;
bool shouldPlayRegisteredSimulation = true;

bool isRotating = false;
glm::vec2 lastRotatingCursorPosition;

static float initialTime;
static float zeroTimeOfSimulation;

int actual_power_of_two_resolution;

void computeRayTracedImage();

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

int numberOfCells;

std::vector<float> particlePositions;
std::vector<float> squarePositions;
std::vector<float> squareNormals;
std::vector<float> spherePositions;
std::vector<float> sphereNormals;
std::vector<int> sphereIndices;
std::vector<float> cellPositions;
std::vector<float> cellColorAndIntensity;
std::vector<LightSource> lightSources;
std::vector<RegularGrid*> regularGrids;

OctreeNode* octreeRoot;

GLuint m_particleVboID;
GLuint m_particleUBO;

GLuint m_squarePositionVboID;
GLuint m_squareNormalVBO;
GLuint m_squareVao;

GLuint m_spherePositionVboID;
GLuint m_sphereNormalVBO;
GLuint m_sphereIBO;
GLuint m_sphereVao;

GLuint m_cellPositionVboID;
GLuint m_cellColorAndIntensityVboID;
GLuint m_cellVao;

GLuint particleProgramID;
GLuint lightingProgramID;
GLuint cellProgramID;

glm::vec3 currentSizeOfGrid = glm::vec3(FLUID_PROPORTION_IN_CUBE*CUBE_SIZE*2.0);

#define TEST_OPENGL_ERROR()                                                             \
  do {		  							\
    GLenum err = glGetError(); 					                        \
    if (err != GL_NO_ERROR) std::cerr << "OpenGL ERROR! " << __LINE__ << std::endl;      \
  } while(0)

void updateUniformMatrixOfShaders()
{
	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("model_view_matrix", model_view_matrix, lightingProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("projection_matrix", projection_matrix, lightingProgramID); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();

	glUseProgram(particleProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("model_view_matrix", model_view_matrix, particleProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("projection_matrix", projection_matrix, particleProgramID); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();

	glUseProgram(cellProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("model_view_matrix", model_view_matrix, cellProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("projection_matrix", projection_matrix, cellProgramID); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();
}

void mouseButtonCallback(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		isRotating = true;
		lastRotatingCursorPosition = glm::vec2(x, y);
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		isRotating = false;
	}
}

void mousePositionCallback(int x, int y)
{
	if (isRotating)
	{
		model_view_matrix = glm::rotate(model_view_matrix, glm::radians((float)(lastRotatingCursorPosition.x - x)), glm::vec3(0.0, 1.0, 0.0));
		updateUniformMatrixOfShaders();
		lastRotatingCursorPosition = glm::vec2(x, y);
	}
}

void keyboardCallback(unsigned char key, int x, int y)
{
	switch (key)
	{
	case GLUT_SPACE:
		pause = !pause;
		if (pause)
		{
			std::cout << "Pause simulation" << std::endl;
		}
		else
		{
			std::cout << "Resume simulation" << std::endl;
		}
		break;
	case GLUT_ESC:
		exit(0);
		break;
	case GLUT_ENTER:
		if (pause)
		{
			computeRayTracedImage();
			break;
		}
	}
}

void updatePositions()
{
	particlePositions.resize(fluid->GetParticles().size() * 3);

	for (int i = 0; i < fluid->GetParticles().size(); i++)
	{
		particlePositions[3 * i] = fluid->GetParticles()[i]->GetPosition().x;
		particlePositions[3 * i + 1] = fluid->GetParticles()[i]->GetPosition().y;
		particlePositions[3 * i + 2] = fluid->GetParticles()[i]->GetPosition().z;
	}
}

void initPositions()
{
	particlePositions.resize(fluid->GetParticles().size() * 3);

	updatePositions();
}

void initParticleVbo()
{
	glGenBuffers(1, &m_particleVboID); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_particleVboID); TEST_OPENGL_ERROR();
	if (shouldGenerateNewParticlesEachFrame)
	{
		glBufferData(GL_ARRAY_BUFFER, particlePositions.size() * sizeof(float) * 10, &(particlePositions[0]), GL_DYNAMIC_DRAW); TEST_OPENGL_ERROR();
	}
	else
	{
		glBufferData(GL_ARRAY_BUFFER, particlePositions.size() * sizeof(float), &(particlePositions[0]), GL_DYNAMIC_DRAW); TEST_OPENGL_ERROR();
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
}

void addCellToCellPositionVector(OctreeNode* octreeNode, bool shouldAddCellToVector = false)
{
	if (shouldAddCellToVector)
	{
		cellPositions.push_back(octreeNode->GetCell()->ComputeCenter()[0]);
		cellPositions.push_back(octreeNode->GetCell()->ComputeCenter()[1]);
		cellPositions.push_back(octreeNode->GetCell()->ComputeCenter()[2]);
	}

	int indexOfCell = cellPositions.size();
	int indexToPowerOfTwo = 1;
	int powerOfTwo = 0;
	while(indexOfCell > indexToPowerOfTwo)
	{
		powerOfTwo += 1;
		indexToPowerOfTwo = indexToPowerOfTwo + pow(8, powerOfTwo);
	}
	int depth = powerOfTwo + 1;

	float numberOfParticlesInCell = octreeNode->GetCell()->GetParticles().size();
	cellColorAndIntensity.push_back(1.0f);
	cellColorAndIntensity.push_back(1.0f);
	cellColorAndIntensity.push_back(1.0f);
	cellColorAndIntensity.push_back(numberOfParticlesInCell / depth / PARTICLE_NUMBER);
}

void UpdateCellVectors(OctreeNode* octreeNode, bool shouldAddCellToVector = false)
{	
	addCellToCellPositionVector(octreeNode, shouldAddCellToVector);

	if (!octreeNode->GetIsALeaf())
	{
		std::vector<std::vector<std::vector<OctreeNode*>>> children = octreeNode->GetChildren();
		UpdateCellVectors(children[0][0][0], shouldAddCellToVector);
		UpdateCellVectors(children[0][0][1], shouldAddCellToVector);
		UpdateCellVectors(children[0][1][0], shouldAddCellToVector);
		UpdateCellVectors(children[0][1][1], shouldAddCellToVector);
		UpdateCellVectors(children[1][0][0], shouldAddCellToVector);
		UpdateCellVectors(children[1][0][1], shouldAddCellToVector);
		UpdateCellVectors(children[1][1][0], shouldAddCellToVector);
		UpdateCellVectors(children[1][1][1], shouldAddCellToVector);
	}
}

void initCellVAO()
{
	UpdateCellVectors(octreeRoot, true);
	glCreateBuffers(1, &m_cellPositionVboID); TEST_OPENGL_ERROR(); // Generate a GPU buffer to store the positions of the vertices
	numberOfCells = 0;
	for (int i = 0; i < actual_power_of_two_resolution + 1; i++)
	{
		numberOfCells += pow(8, i);
	}
	size_t positionBufferSize = sizeof(float) * numberOfCells * 3; // Gather the size of the buffer from the CPU-side vector
	glNamedBufferStorage(m_cellPositionVboID, positionBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR(); // Create a data store on the GPU
	glNamedBufferSubData(m_cellPositionVboID, 0, positionBufferSize, cellPositions.data()); TEST_OPENGL_ERROR(); // Fill the data store from a CPU array
	size_t colorAndIntensityBufferSize = sizeof(float) * numberOfCells * 4;
	glCreateBuffers(1, &m_cellColorAndIntensityVboID); TEST_OPENGL_ERROR();
	glNamedBufferStorage(m_cellColorAndIntensityVboID, colorAndIntensityBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR();
	glNamedBufferSubData(m_cellColorAndIntensityVboID, 0, colorAndIntensityBufferSize, cellColorAndIntensity.data()); TEST_OPENGL_ERROR();

	glCreateVertexArrays(1, &m_cellVao);  TEST_OPENGL_ERROR();// Create a single handle that joins together attributes (vertex positions, normals) and connectivity (triangles indices)
	glBindVertexArray(m_cellVao); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_cellPositionVboID); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_cellColorAndIntensityVboID); TEST_OPENGL_ERROR();
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();

	glBindVertexArray(0); TEST_OPENGL_ERROR(); // Desactive the VAO just created. Will be activated at rendering time.
}

void initSphereVAO()
{
	glCreateBuffers(1, &m_spherePositionVboID); TEST_OPENGL_ERROR(); // Generate a GPU buffer to store the positions of the vertices
	size_t vertexBufferSize = sizeof(float) * spherePositions.size(); // Gather the size of the buffer from the CPU-side vector
	glNamedBufferStorage(m_spherePositionVboID, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR(); // Create a data store on the GPU
	glNamedBufferSubData(m_spherePositionVboID, 0, vertexBufferSize, spherePositions.data()); TEST_OPENGL_ERROR(); // Fill the data store from a CPU array

	glCreateBuffers(1, &m_sphereNormalVBO); TEST_OPENGL_ERROR(); // Same for normal
	glNamedBufferStorage(m_sphereNormalVBO, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR();
	glNamedBufferSubData(m_sphereNormalVBO, 0, vertexBufferSize, sphereNormals.data()); TEST_OPENGL_ERROR();

	glCreateBuffers(1, &m_sphereIBO); TEST_OPENGL_ERROR(); // Same for the index buffer, that stores the list of indices of the triangles forming the mesh
	size_t indexBufferSize = sizeof(float) * sphereIndices.size();
	glNamedBufferStorage(m_sphereIBO, indexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR();
	glNamedBufferSubData(m_sphereIBO, 0, indexBufferSize, sphereIndices.data()); TEST_OPENGL_ERROR();

	glCreateVertexArrays(1, &m_sphereVao); TEST_OPENGL_ERROR(); // Create a single handle that joins together attributes (vertex positions, normals) and connectivity (triangles indices)
	glBindVertexArray(m_sphereVao); TEST_OPENGL_ERROR();
	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_spherePositionVboID); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();
	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_sphereNormalVBO); TEST_OPENGL_ERROR();
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereIBO); TEST_OPENGL_ERROR();
	glBindVertexArray(0); TEST_OPENGL_ERROR();// Desactive the VAO just created. Will be activated at rendering time.
}

void initSquareVAO()
{
	glCreateBuffers(1, &m_squarePositionVboID); TEST_OPENGL_ERROR(); // Generate a GPU buffer to store the positions of the vertices
	size_t vertexBufferSize = sizeof(float) * squarePositions.size(); TEST_OPENGL_ERROR(); // Gather the size of the buffer from the CPU-side vector
	glNamedBufferStorage(m_squarePositionVboID, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR(); // Create a data store on the GPU
	glNamedBufferSubData(m_squarePositionVboID, 0, vertexBufferSize, squarePositions.data()); TEST_OPENGL_ERROR(); // Fill the data store from a CPU array

	glCreateBuffers(1, &m_squareNormalVBO); TEST_OPENGL_ERROR(); // Same for normal
	glNamedBufferStorage(m_squareNormalVBO, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR();
	glNamedBufferSubData(m_squareNormalVBO, 0, vertexBufferSize, squareNormals.data()); TEST_OPENGL_ERROR();

	glCreateVertexArrays(1, &m_squareVao);  TEST_OPENGL_ERROR();// Create a single handle that joins together attributes (vertex positions, normals) and connectivity (triangles indices)
	glBindVertexArray(m_squareVao); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squarePositionVboID); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squareNormalVBO); TEST_OPENGL_ERROR();
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();

	glBindVertexArray(0); TEST_OPENGL_ERROR(); // Desactive the VAO just created. Will be activated at rendering time.
}

void initParticleUBO()
{
	glGenBuffers(1, &m_particleUBO); TEST_OPENGL_ERROR();
	glBindBuffer(GL_UNIFORM_BUFFER, m_particleUBO); TEST_OPENGL_ERROR();
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightSource) * cellPositions.size()/3 + sizeof(int), NULL, GL_DYNAMIC_DRAW); TEST_OPENGL_ERROR();
	glBindBuffer(GL_UNIFORM_BUFFER, 0); TEST_OPENGL_ERROR();
}

void initBuffers()
{
	initParticleVbo();

	///////////////////////////////////////

	initCellVAO();

	///////////////////////////////////////

	initSquareVAO();

	////////////////////////////////

	initSphereVAO();

	////////////////////////////////

	initParticleUBO();
}

void generateParticles(int particleNumber)
{
	float fluidInitialSpace = FLUID_PROPORTION_IN_CUBE * CUBE_SIZE;
	fluid->GenerateParticlesUniformly(particleNumber, glm::vec3(0.0, 0.0, 0.0), fluidInitialSpace, fluidInitialSpace, fluidInitialSpace, 0.1f);
}

void clearParticles()
{
	fluid->ClearParticles();
}

void initFluid()
{
	fluid = new Fluid(VISCOSITY, DENSITY);
	generateParticles(PARTICLE_NUMBER);
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

void createSphere(glm::vec3 origin, float radius, int resolution)
{
	float x, y, z, xy;                              // vertex position
	float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
	float s, t;                                     // vertex texCoord

	float sectorStep = 2 * M_PI / resolution;
	float stackStep = M_PI / resolution;
	float sectorAngle, stackAngle;

	for (int i = 0; i <= resolution; ++i)
	{
		stackAngle = M_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
		xy = radius * cosf(stackAngle);             // r * cos(u)
		z = radius * sinf(stackAngle);              // r * sin(u)

		// add (sectorCount+1) vertices per stack
		// the first and last vertices have same position and normal, but different tex coords
		for (int j = 0; j <= resolution; ++j)
		{
			sectorAngle = j * sectorStep;           // starting from 0 to 2pi

			// vertex position (x, y, z)
			x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
			y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
			spherePositions.push_back(x + origin.x);
			spherePositions.push_back(y + origin.y);
			spherePositions.push_back(z + origin.z);

			// normalized vertex normal (nx, ny, nz)
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;
			sphereNormals.push_back(nx);
			sphereNormals.push_back(ny);
			sphereNormals.push_back(nz);
		}
	}

	// generate CCW index list of sphere triangles
	int k1, k2;
	for (int i = 0; i < resolution; ++i)
	{
		k1 = i * (resolution + 1);     // beginning of current stack
		k2 = k1 + resolution + 1;      // beginning of next stack

		for (int j = 0; j < resolution; ++j, ++k1, ++k2)
		{
			// 2 triangles per sector excluding first and last stacks
			// k1 => k2 => k1+1
			if (i != 0)
			{
				sphereIndices.push_back(k1);
				sphereIndices.push_back(k2);
				sphereIndices.push_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (resolution - 1))
			{
				sphereIndices.push_back(k1 + 1);
				sphereIndices.push_back(k2);
				sphereIndices.push_back(k2 + 1);
			}
		}
	}
}

void initScene()
{
	createSquare();

	createSphere(glm::vec3(-0.5f, 0.0f, -0.5f), 0.1f, 10);

	int power_index = 0;
	float temp = (float)RESOLUTION;

	while (temp > 1.0 && power_index < 5)
	{
		temp = temp / 2.0;
		power_index = power_index + 1;
	}

	actual_power_of_two_resolution = power_index;
	for (int i = 0; i < actual_power_of_two_resolution + 1; i++)
	{
		regularGrids.push_back(new RegularGrid(pow(2, i), currentSizeOfGrid));
	}
	regularGrids[0]->GetCells()[0][0][0]->SetParticles(fluid->GetParticles());
}

void initShaders()
{
	// Create and compile our GLSL program from the shaders
	particleProgramID = ShaderProgram::LoadShaders("Resources/vertex.shd", "Resources/fragment.shd");
	lightingProgramID = ShaderProgram::LoadShaders("Resources/lighting_vertex.shd", "Resources/lighting_fragment.shd");
	cellProgramID = ShaderProgram::LoadShaders("Resources/cell_vertex.shd", "Resources/cell_fragment.shd");

	updateUniformMatrixOfShaders();
}

void initOctree()
{
	int position_of_root_in_grid[3] = { 0, 0, 0 };
	octreeRoot = OctreeNode::BuildOctree(0, actual_power_of_two_resolution - 1, regularGrids[0]->GetCells()[0][0][0], position_of_root_in_grid, regularGrids);
}

int init(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow(argv[0]); TEST_OPENGL_ERROR();
	glutReshapeWindow(1024, 1024); TEST_OPENGL_ERROR();
	glutMouseFunc(&mouseButtonCallback);
	glutMotionFunc(&mousePositionCallback);
	glutKeyboardFunc(&keyboardCallback);
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
	initBuffers();
	initShaders();
	return 0;
}

void writeInt(std::ofstream& file, int val) {
	file.write(reinterpret_cast<char *>(&val), sizeof(int));
}

void writeFloat(std::ofstream& file, float val) {
	file.write(reinterpret_cast<char *>(&val), sizeof(float));
}

void writeFrameInFile(std::ofstream &out)
{
	writeInt(out, fluid->GetParticles().size());
	for (int i = 0; i < fluid->GetParticles().size(); i++)
	{
		writeFloat(out, fluid->GetParticles()[i]->GetPosition().x);
		writeFloat(out, fluid->GetParticles()[i]->GetPosition().y);
		writeFloat(out, fluid->GetParticles()[i]->GetPosition().z);
	}
}

bool readFrameInFile(std::ifstream &in)
{
	int numberOfParticles;
	glm::vec3 position;
	
	in.read((char *)&numberOfParticles, sizeof(int));
	if (numberOfParticles == -1)
	{
		in.close();
		in.open("simulation.txt", std::ios_base::binary);
		return readFrameInFile(in);
	}
	else if (numberOfParticles <= 0)
	{
		std::cout << "Read " << numberOfParticles << " particles from file" << std::endl;
		return false;
	}
	else
	{
		fluid->ClearParticles();
		for (int i = 0; i < numberOfParticles; i++)
		{
			in.read((char *)&(position.x), sizeof(float));
			in.read((char *)&(position.y), sizeof(float));
			in.read((char *)&(position.z), sizeof(float));
			fluid->AddParticle(position, 1.0f / numberOfParticles);
		}
	}
	return true;
}

void update(float currentTime, bool realTimeSimulation)
{
	frameNumber += 1;

	float dt = currentTime - initialTime;
	initialTime = currentTime;

	float timeFactor;
	if (realTimeSimulation)
	{
		timeFactor = dt * 0.0005f;
	}
	else
	{
		timeFactor = 0.003f;
	}

	if (shouldPlayRegisteredSimulation)
	{
		clearParticles();
		currentSizeOfGrid = glm::vec3(CUBE_SIZE*2.0);

		if (in.good())
		{
			if (!readFrameInFile(in))
			{
				in.close();
				while (1)
				{
					//std::cout << "Cannot read file" << std::endl;
				}
			}
		}
	}
	else
	{
		glm::vec3 maxSpeed = regularGrids[regularGrids.size() - 1]->UpdateSpeedOfCells();
		currentSizeOfGrid[0] = glm::min(currentSizeOfGrid[0] + maxSpeed[0] * timeFactor, 2.0f * CUBE_SIZE);
		currentSizeOfGrid[1] = glm::min(currentSizeOfGrid[1] + maxSpeed[1] * timeFactor, 2.0f * CUBE_SIZE);
		currentSizeOfGrid[2] = glm::min(currentSizeOfGrid[2] + maxSpeed[2] * timeFactor, 2.0f * CUBE_SIZE);

		fluid->UpdateParticlePositions(timeFactor * CUBE_SIZE, CUBE_SIZE);
		if (shouldGenerateNewParticlesEachFrame && fluid->GetParticles().size() + PARTICLE_NUMBER * 0.01 < 10 * PARTICLE_NUMBER)
		{
			generateParticles(PARTICLE_NUMBER * 0.01);
		}

		if (shouldRegisterSimulation)
		{
			if (!out.is_open())
			{
				out = std::ofstream("simulation.txt", std::ios::out | std::ios::binary);
			}
			if (out.good())
			{
				writeFrameInFile(out);
			}
		}

		regularGrids[regularGrids.size() - 1]->UpdateGradientAndVGradVOfCells();
		regularGrids[regularGrids.size() - 1]->UpdateLaplacianOfCells();
		regularGrids[regularGrids.size() - 1]->PushNavierStokesParametersToParticles();

		if (fluid->GetParticles().size() < PARTICLE_NUMBER * 0.1 || (SIMULATION_MAX_DURATION != 0.0f && currentTime - zeroTimeOfSimulation > SIMULATION_MAX_DURATION))
		{
			std::cout << "Simulation duration : " << (glutGet(GLUT_ELAPSED_TIME) - zeroTimeOfSimulation)/1000.0f << " seconds" << std::endl;
			if (in.good())
			{
				in.close();
			}
			if (out.good())
			{
				writeInt(out, -1);
				out.close();
			}
			regularGrids[regularGrids.size() - 1]->ResetNavierStokesParametersOfCells();
			clearParticles();
			currentSizeOfGrid = glm::vec3(CUBE_SIZE*2.0);
			generateParticles(PARTICLE_NUMBER);
			// Register only the first simulation for now.
			shouldRegisterSimulation = false;
			shouldPlayRegisteredSimulation = true;
			in = std::ifstream("simulation.txt", std::ios_base::binary);
			frameNumber = 0;
		}
	}

	updatePositions();

	if (shouldRegisterSimulation)
	{
		regularGrids[0]->GetCells()[0][0][0]->SetParticles(fluid->GetParticles());

		octreeRoot->SetCell(regularGrids[0]->GetCells()[0][0][0]);
		octreeRoot->UpdateParticlesInChildrenCells();

		cellColorAndIntensity.clear();
		cellPositions.clear();
		UpdateCellVectors(octreeRoot, true);

		for (int i = 0; i < regularGrids.size(); i++)
		{
			regularGrids[i]->ResizeGrid(currentSizeOfGrid);
		}
	}

}

void drawParticlesVBO()
{
	glUseProgram(particleProgramID); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_particleVboID); TEST_OPENGL_ERROR();

	glBufferSubData(GL_ARRAY_BUFFER, 0, particlePositions.size() * sizeof(float), &(particlePositions[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();
	glDrawArrays(GL_POINTS, 0, particlePositions.size() / 3); TEST_OPENGL_ERROR();

	glDisableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();
}

void drawSphereVAO()
{
	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();

	if (shouldRenderLighting)
	{
		lightSources.resize(cellPositions.size() / 3);

		for (int i = 0; i < cellPositions.size() / 3; i++)
		{
			lightSources[i].position = glm::vec4(cellPositions[3 * i], cellPositions[3 * i + 1], cellPositions[3 * i + 2], 1.0);
			lightSources[i].color_and_intensity = glm::vec4(cellColorAndIntensity[4 * i], cellColorAndIntensity[4 * i + 1], cellColorAndIntensity[4 * i + 2], cellColorAndIntensity[4 * i + 3]);
		}
	}
	else
	{
		lightSources.resize(1);
		lightSources[0].position = glm::vec4(0.0f);
		lightSources[0].color_and_intensity = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	glBindBuffer(GL_UNIFORM_BUFFER, m_particleUBO); TEST_OPENGL_ERROR();
	int actualNumberOfCells[1] = { lightSources.size() };
	GLsizei sifeOfArray = lightSources.size() * sizeof(LightSource);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sifeOfArray, &(lightSources[0])); TEST_OPENGL_ERROR();
	glBufferSubData(GL_UNIFORM_BUFFER, sifeOfArray, sizeof(int), &(actualNumberOfCells)); TEST_OPENGL_ERROR();

	GLuint lightSourcesBlockIdx = glGetUniformBlockIndex(lightingProgramID, "lightSourcesBlock"); TEST_OPENGL_ERROR();
	glUniformBlockBinding(lightingProgramID, lightSourcesBlockIdx, 0); TEST_OPENGL_ERROR();
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_particleUBO); TEST_OPENGL_ERROR();
	glBindBuffer(GL_UNIFORM_BUFFER, 0); TEST_OPENGL_ERROR();

	glBindVertexArray(m_sphereVao); TEST_OPENGL_ERROR(); // Activate the VAO storing geometry data
	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_spherePositionVboID); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, spherePositions.size() * sizeof(float), &(spherePositions[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_sphereNormalVBO); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, sphereNormals.size() * sizeof(float), &(sphereNormals[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei> (sphereIndices.size()), GL_UNSIGNED_INT, 0); // Call for rendering: stream the current GPU geometry through the current GPU program

	glDisableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glDisableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();
}

void drawCubeVAO()
{
	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();

	if (shouldRenderLighting)
	{
		lightSources.resize(cellPositions.size() / 3);

		for (int i = 0; i < cellPositions.size() / 3; i++)
		{
			lightSources[i].position = glm::vec4(cellPositions[3 * i], cellPositions[3 * i + 1], cellPositions[3 * i + 2], 1.0);
			lightSources[i].color_and_intensity = glm::vec4(cellColorAndIntensity[4 * i], cellColorAndIntensity[4 * i + 1], cellColorAndIntensity[4 * i + 2], cellColorAndIntensity[4 * i + 3]);
		}
	}
	else
	{
		lightSources.resize(1);
		lightSources[0].position = glm::vec4(0.0f);
		lightSources[0].color_and_intensity = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	glBindBuffer(GL_UNIFORM_BUFFER, m_particleUBO); TEST_OPENGL_ERROR();
	int actualNumberOfCells[1] = { lightSources.size() };
	GLsizei sifeOfArray = lightSources.size() * sizeof(LightSource);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sifeOfArray, &(lightSources[0])); TEST_OPENGL_ERROR();
	glBufferSubData(GL_UNIFORM_BUFFER, sifeOfArray, sizeof(int), &(actualNumberOfCells)); TEST_OPENGL_ERROR();

	GLuint lightSourcesBlockIdx = glGetUniformBlockIndex(lightingProgramID, "lightSourcesBlock"); TEST_OPENGL_ERROR();
	glUniformBlockBinding(lightingProgramID, lightSourcesBlockIdx, 0); TEST_OPENGL_ERROR();
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_particleUBO); TEST_OPENGL_ERROR();
	glBindBuffer(GL_UNIFORM_BUFFER, 0); TEST_OPENGL_ERROR();

	glBindVertexArray(m_squareVao); TEST_OPENGL_ERROR(); // Activate the VAO storing geometry data
	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squarePositionVboID); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, squarePositions.size() * sizeof(float), &(squarePositions[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squareNormalVBO); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, squareNormals.size() * sizeof(float), &(squareNormals[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glDrawArrays(GL_TRIANGLES, 0, squarePositions.size() / 3); TEST_OPENGL_ERROR();

	glDisableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glDisableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();
}

void drawCellVAO()
{
	glUseProgram(cellProgramID); TEST_OPENGL_ERROR();

	glBindVertexArray(m_cellVao); TEST_OPENGL_ERROR();
	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_cellPositionVboID); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, cellPositions.size() * sizeof(float), &(cellPositions[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_cellColorAndIntensityVboID); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, cellColorAndIntensity.size() * sizeof(float), &(cellColorAndIntensity[0])); TEST_OPENGL_ERROR();
	GLintptr offSet = cellPositions.size() * sizeof(float) * 3;
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glDrawArrays(GL_POINTS, 0, cellPositions.size() / 3); TEST_OPENGL_ERROR();

	glDisableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glDisableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();
}

void render()
{
	if (!pause)
	{
		update(glutGet(GLUT_ELAPSED_TIME), !shouldRegisterSimulation);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); TEST_OPENGL_ERROR();

	drawParticlesVBO();

	drawCubeVAO();

	drawSphereVAO();

	if (shouldRegisterSimulation && shouldRenderLighting)
	{
		drawCellVAO();
	}

	glutSwapBuffers();

	glutPostRedisplay();
}

void clear()
{
	glDeleteBuffers(1, &m_particleVboID); TEST_OPENGL_ERROR();
	glDeleteBuffers(1, &m_squarePositionVboID); TEST_OPENGL_ERROR();
	glDeleteBuffers(1, &m_spherePositionVboID); TEST_OPENGL_ERROR();
	glDeleteBuffers(1, &m_squareNormalVBO); TEST_OPENGL_ERROR();
	glDeleteBuffers(1, &m_squareVao); TEST_OPENGL_ERROR();

	for (int i = 0; i < regularGrids.size(); i++)
	{
		delete regularGrids[i];
	}
	delete octreeRoot;
	delete fluid;
}

struct Vec
{
	double x, y, z;
	Vec(double x_ = 0, double y_ = 0, double z_ = 0)
	{
		x = x_;
		y = y_;
		z = z_;
	}
	Vec operator+(const Vec &b) const { return Vec(x + b.x, y + b.y, z + b.z); }
	Vec operator-(const Vec &b) const { return Vec(x - b.x, y - b.y, z - b.z); }
	Vec operator*(double b) const { return Vec(x * b, y * b, z * b); }
	Vec mult(const Vec &b) const { return Vec(x * b.x, y * b.y, z * b.z); }
	Vec &normalize() { return *this = *this * (1 / sqrt(x * x + y * y + z * z)); }
	double dot(const Vec &b) const { return x * b.x + y * b.y + z * b.z; }
	Vec cross(Vec &b) { return Vec(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); }
};
Vec operator*(double b, Vec const &o) { return Vec(o.x * b, o.y * b, o.z * b); }

void generateRandomPointOnSphere(double &theta, double &phi)
{
	double x = (double)(rand()) / RAND_MAX;
	double y = (double)(rand()) / RAND_MAX;
	theta = x * 2.0 * M_PI;
	phi = acos(std::min<double>(1.0, std::max<double>(-1.0, 2.0 * y - 1.0)));
}
Vec randomSampleOnSphere()
{
	double theta, phi;
	generateRandomPointOnSphere(theta, phi);
	return Vec(cos(theta) * cos(phi), sin(theta) * cos(phi), sin(phi));
}
Vec randomSampleOnHemisphere(Vec const &upDirection)
{
	Vec r = randomSampleOnSphere();
	if (r.dot(upDirection) > 0.0)
		return r;
	return -1.0 * r;
}

struct Ray
{
	Vec o, d;
	Ray(Vec o_, Vec d_) : o(o_), d(d_)
	{
		d.normalize();
	}
};

enum Refl_t
{
	DIFFUSE,
	MIRROR,
	GLASS,
	EMMISSIVE
}; // material types, used in radiance()

struct Sphere
{
	double radius; // radius
	Vec p, e, c;   // position, emission, color
	Refl_t refl;   // reflection type (DIFFuse, SPECular, REFRactive)
	Sphere(double rad_, Vec p_, Vec e_, Vec c_, Refl_t refl_) : radius(rad_), p(p_), e(e_), c(c_), refl(refl_) {}

	double intersect(const Ray &r) const
	{ // returns distance, 0 if nohit
		// TODO
		Vec oc = r.o - p;
		double sa = 1.0;
		double sb = 2.0 * oc.dot(r.d);
		double sc = oc.dot(oc) - radius * radius;

		double delta = sb * sb - 4.0 * sa * sc;
		if (delta < 0.0)
			return 0.0; // no solution

		double deltaSqrt = sqrt(delta);
		double lambda1 = (-sb - deltaSqrt) / (2.0 * sa);
		double lambda2 = (-sb + deltaSqrt) / (2.0 * sa);
		if (lambda1 < lambda2 && lambda1 >= 0.0)
			return lambda1;
		if (lambda2 >= 0.0)
			return lambda2;
		return 0.0;
	}

	Vec randomSample() const
	{
		return p + radius * randomSampleOnSphere();
	}
};

// Scene :
std::vector<Sphere> spheres;
std::vector<unsigned int> lights;
// lights is the whole set of emissive objects

inline double clamp(double x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
inline double clamp(double x, double min, double max)
{
	if (x < min)
		x = min;
	else if (x > max)
		x = max;
	return x;
}
inline bool intersectScene(const Ray &r, double &t, int &id)
{
	double d, inf = t = 1e20;
	for (int i = 0; i < spheres.size(); ++i)
		if ((d = spheres[i].intersect(r)) && d < t)
		{
			t = d;
			id = i;
		}
	return t < inf;
}

double brdf(const Vec &wi, const Vec &n, const Vec &wo)
{
	const double Kd = 1.0;
	const double Ks = 0.0;
	const double s = 10.0; // brightness

	Vec r = 2 * n * (wi.dot(n)) - wi;
	double fs = Ks * pow(r.dot(wo), s);

	double brdf = Kd + fs;
	return brdf;
}

Vec refract(const Vec &I, const Vec &N, const float &ior)
{
	float cosi = clamp(-1, 1, I.dot(N));
	float etai = 1, etat = ior;
	Vec n = N;
	if (cosi < 0)
		cosi = -cosi;
	else
	{
		std::swap(etai, etat);
		n = -1.0 * N;
	}
	float eta = etai / etat;
	float k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
}

Vec radiance(const Ray &r, int depth)
{
	double t;   // distance to intersection
	int id = 0; // id of intersected object
	if (!intersectScene(r, t, id))
		return Vec(); // if miss, return black

	const Sphere &obj = spheres[id]; // the hit object

	Vec x = r.o + r.d * t,           // the hit position
		n = (x - obj.p).normalize(), // the normal of the object at the hit
		f = obj.c;                   // the color of the object at the hit

	if (n.dot(r.d) > 0.0)
		n = -1.0 * n;

	if (++depth > 5)
		return Vec(); // we limit the number of rebounds in the scene

	if (obj.refl == EMMISSIVE)
	{ // we hit a light
		return obj.e;
	}
	if (obj.refl == DIFFUSE)
	{ // Ideal DIFFUSE reflection
		// We shoot rays towards all lights:
		Vec rad;
		for (unsigned int lightIt = 0; lightIt < lights.size(); ++lightIt)
		{
			const Sphere &light = spheres[lights[lightIt]];

			// TODO
			const Vec dirToLight = (light.randomSample() - x).normalize();
			const Ray &toLight = Ray(x + 0.0001 * dirToLight, dirToLight);
			int idTemp;
			double tTemp;
			if (intersectScene(toLight, tTemp, idTemp))
			{
				if (idTemp == lights[lightIt])
				{
					const Vec Li = light.e;
					rad = rad + Li.mult(obj.c) * brdf(toLight.d, n, -1.0 * r.d) * (n.dot(toLight.d));
				}
			}
		}

		// TODO : add secondary rays:
		const uint32_t NB_SECONDARY_RAY = 0;
		for (uint32_t i = 0; i < NB_SECONDARY_RAY; i++)
		{
			const Vec newDir = randomSampleOnHemisphere(n).normalize();
			const Ray &newRay = Ray(x + 0.0001 * newDir, newDir);
			rad = rad + radiance(newRay, depth) * brdf(newRay.d, n, -1.0 * r.d) * (n.dot(newRay.d)); // FIXME : Have to be normalized
		}

		return rad;
	}
	else if (obj.refl == MIRROR)
	{ // Ideal SPECULAR reflection
		// TODO
		Vec dirToReflected = r.d - 2 * n * (r.d.dot(n));
		const Ray &toReflected = Ray(x + 0.0001 * dirToReflected, dirToReflected);

		return radiance(toReflected, depth).mult(obj.c);
	}
	else if (obj.refl == GLASS)
	{ // Ideal SPECULAR refraction
		// TODO

		Vec dirToRefracted = refract(r.d, n, 1.500);
		const Ray &toReflected = Ray(x + 0.0001 * dirToRefracted, dirToRefracted);

		return radiance(toReflected, depth).mult(obj.c);
		return Vec();
	}

	return Vec();
}


void computeRayTracedImage()
{
	int w = 1024, h = 768, samps = 1; // # samples
	Ray cam(Vec(50, 52, 295.6), Vec(0, -0.042612, -1).normalize());   // camera center and direction
	Vec cx = Vec(w * .5135 / h), cy = (cx.cross(cam.d)).normalize() * .5135, *pixelsColor = new Vec[w * h];

	// setup scene:
	spheres.push_back(Sphere(1e5, Vec(1e5 + 1, 40.8, 81.6), Vec(0, 0, 0), Vec(.75, .25, .25), DIFFUSE));   //Left
	spheres.push_back(Sphere(1e5, Vec(-1e5 + 99, 40.8, 81.6), Vec(0, 0, 0), Vec(.25, .25, .75), DIFFUSE)); //Rght
	spheres.push_back(Sphere(1e5, Vec(50, 40.8, 1e5), Vec(0, 0, 0), Vec(.75, .75, .75), DIFFUSE));         //Back
	spheres.push_back(Sphere(1e5, Vec(50, 40.8, -1e5 + 170), Vec(0, 0, 0), Vec(0, 0, 0), DIFFUSE));        //Front
	spheres.push_back(Sphere(1e5, Vec(50, 1e5, 81.6), Vec(0, 0, 0), Vec(.75, .75, .75), DIFFUSE));         //Bottom
	spheres.push_back(Sphere(1e5, Vec(50, -1e5 + 81.6, 81.6), Vec(0, 0, 0), Vec(.75, .75, .75), DIFFUSE)); //Top
	spheres.push_back(Sphere(16.5, Vec(27, 16.5, 47), Vec(0, 0, 0), Vec(1, 1, 1) * .999, MIRROR));         //Mirr
	spheres.push_back(Sphere(16.5, Vec(73, 16.5, 78), Vec(0, 0, 0), Vec(1, 1, 1) * .999, GLASS));         //Change to Glass
	spheres.push_back(Sphere(5, Vec(50, 70, 50), Vec(1, 1, 1), Vec(0, 0, 0), EMMISSIVE));                  //Light
	lights.push_back(8);

	// ray trace:
	for (int y = 0; y < h; y++)
	{ // Loop over image rows
		fprintf(stderr, "\rRendering (%d spp) %5.2f%%", samps * 4, 100. * y / (h - 1));
		for (unsigned short x = 0; x < w; x++)
		{ // Loop cols
			Vec r(0, 0, 0);
			for (unsigned int sampleIt = 0; sampleIt < samps; ++sampleIt)
			{
				double dx = ((double)(rand()) / RAND_MAX);
				double dy = ((double)(rand()) / RAND_MAX);
				Vec d = cx * ((x + dx) / w - .5) +
					cy * ((y + dy) / h - .5) + cam.d;
				r = r + radiance(Ray(cam.o + d * 140, d.normalize()), 0) * (1. / samps);
			}

			pixelsColor[x + (h - 1 - y) * w] = pixelsColor[x + (h - 1 - y) * w] + Vec(clamp(r.x), clamp(r.y), clamp(r.z));
		} // Camera rays are pushed ^^^^^ forward to start in interior
	}

	// save image:
	FILE *f;
	errno_t err = fopen_s(&f, "image.ppm", "w"); // Write image to PPM file.
	if (err == 0)
	{
		fprintf(f, "P3\n%d %d\n%d\n", w, h, 255);
		for (int i = 0; i < w * h; i++)
			fprintf(f, "%d %d %d ", (int)(pixelsColor[i].x * 255), (int)(pixelsColor[i].y * 255), (int)(pixelsColor[i].z * 255));
		fclose(f);
	}
}

int main(int argc, char **argv) {
	if (init(argc, argv) != 0)
	{
		return 1;
	}


	if (shouldRegisterSimulation)
	{
		std::ofstream ofs("simulation.txt", std::ios::out | std::ios::trunc); // clear contents
		ofs.close();
	}

	in = std::ifstream("simulation.txt", std::ios_base::binary);

	initialTime = glutGet(GLUT_ELAPSED_TIME);
	zeroTimeOfSimulation = initialTime;

	glutDisplayFunc(render);
	glutMainLoop();

	clear();

	return 0;
}