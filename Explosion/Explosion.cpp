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
#define RESOLUTION 4

int actual_power_of_two_resolution;

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

std::vector<float> particlePositions;
std::vector<float> squarePositions;
std::vector<float> squareNormals;
std::vector<float> cellPositions;
std::vector<float> cellColorAndIntensity;

Cell* scene;
OctreeNode* octreeRoot;

GLuint m_particleVboID;
GLuint m_particleUBO;

GLuint m_squarePositionVboID;
GLuint m_squareNormalVBO;
GLuint m_squareVao;

GLuint m_cellPositionVboID;
GLuint m_cellColorAndIntensityVboID;
GLuint m_cellVao;

GLuint particleProgramID;
GLuint lightingProgramID;
GLuint cellProgramID;

LightSource lightSources[PARTICLE_NUMBER];

#define TEST_OPENGL_ERROR()                                                             \
  do {		  							\
    GLenum err = glGetError(); 					                        \
    if (err != GL_NO_ERROR) std::cerr << "OpenGL ERROR! " << __LINE__ << std::endl;      \
  } while(0)

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
	glBufferData(GL_ARRAY_BUFFER, particlePositions.size() * sizeof(float), &(particlePositions[0]), GL_DYNAMIC_DRAW); TEST_OPENGL_ERROR();
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

	cellColorAndIntensity.push_back(octreeNode->GetCell()->GetParticles().size());
	cellColorAndIntensity.push_back(octreeNode->GetCell()->GetParticles().size());
	cellColorAndIntensity.push_back(octreeNode->GetCell()->GetParticles().size());
	cellColorAndIntensity.push_back(1.0f);
}

void UpdateCellVectors(OctreeNode* octreeNode, bool shouldAddCellToVector = false)
{	
	addCellToCellPositionVector(octreeNode, shouldAddCellToVector);

	if (!octreeNode->GetIsALeaf() && octreeNode->GetDepth() < 3)
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
	glCreateBuffers(1, &m_cellPositionVboID); // Generate a GPU buffer to store the positions of the vertices
	int numberOfCells = 0;
	for (int i = 0; i < actual_power_of_two_resolution + 1; i++)
	{
		numberOfCells += pow(8, i);
	}
	size_t positionBufferSize = sizeof(float) * cellPositions.size(); // Gather the size of the buffer from the CPU-side vector
	glNamedBufferStorage(m_cellPositionVboID, positionBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); // Create a data store on the GPU
	glNamedBufferSubData(m_cellPositionVboID, 0, positionBufferSize, cellPositions.data()); // Fill the data store from a CPU array
	size_t colorAndIntensityBufferSize = sizeof(float) * cellColorAndIntensity.size();
	glCreateBuffers(1, &m_cellColorAndIntensityVboID);
	glNamedBufferStorage(m_cellColorAndIntensityVboID, colorAndIntensityBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferSubData(m_cellColorAndIntensityVboID, 0, colorAndIntensityBufferSize, cellColorAndIntensity.data());

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

void initSquareVAO()
{
	glCreateBuffers(1, &m_squarePositionVboID); // Generate a GPU buffer to store the positions of the vertices
	size_t vertexBufferSize = sizeof(float) * squarePositions.size(); // Gather the size of the buffer from the CPU-side vector
	glNamedBufferStorage(m_squarePositionVboID, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); // Create a data store on the GPU
	glNamedBufferSubData(m_squarePositionVboID, 0, vertexBufferSize, squarePositions.data()); // Fill the data store from a CPU array

	glCreateBuffers(1, &m_squareNormalVBO); // Same for normal
	glNamedBufferStorage(m_squareNormalVBO, vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferSubData(m_squareNormalVBO, 0, vertexBufferSize, squareNormals.data());

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
	glGenBuffers(1, &m_particleUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, m_particleUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightSource) * PARTICLE_NUMBER + sizeof(int), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void initBuffers()
{
	initParticleVbo();

	///////////////////////////////////////

	initCellVAO();

	///////////////////////////////////////

	initSquareVAO();

	////////////////////////////////

	initParticleUBO();
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
	scene = new Cell(nullptr, glm::vec3(-CUBE_SIZE), 2.0 * CUBE_SIZE, 2.0 * CUBE_SIZE, 2.0 * CUBE_SIZE);
	scene->SetParticles(fluid->GetParticles());
}

void initShaders()
{
	// Create and compile our GLSL program from the shaders
	particleProgramID = ShaderProgram::LoadShaders("Resources/vertex.shd", "Resources/fragment.shd");
	lightingProgramID = ShaderProgram::LoadShaders("Resources/lighting_vertex.shd", "Resources/lighting_fragment.shd");
	cellProgramID = ShaderProgram::LoadShaders("Resources/cell_vertex.shd", "Resources/cell_fragment.shd");

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

void initOctree()
{
	int power_index = 0;
	float temp = RESOLUTION;

	while (temp > 1.0 && power_index < 3)
	{
		temp = temp / 2.0;
		power_index = power_index + 1;
	}

	actual_power_of_two_resolution = power_index;

	octreeRoot = OctreeNode::BuildOctree(0, power_index - 1, scene);
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
	initBuffers();
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

	octreeRoot->SetCell(scene);
	octreeRoot->UpdateParticlesInChildrenCells(); // TODO To improve

	cellColorAndIntensity.clear();
	UpdateCellVectors(octreeRoot, false);
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

void drawCubeVAO()
{
	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();

	for (int i = 0; i < particlePositions.size()/3; i++)
	{
		lightSources[i].position = glm::vec4(particlePositions[3 * i], particlePositions[3 * i + 1], particlePositions[3 * i + 2], 1.0);
		lightSources[i].color_and_intensity = glm::vec4(1.0);
	}

	glBindBuffer(GL_UNIFORM_BUFFER, m_particleUBO);
	int actualNumberOfParticles[1] = { particlePositions.size() };
	GLsizei sifeOfArray = particlePositions.size() / 3 * sizeof(LightSource);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sifeOfArray, &(lightSources[0])); TEST_OPENGL_ERROR();
	glBufferSubData(GL_UNIFORM_BUFFER, sifeOfArray, sizeof(int), &(actualNumberOfParticles)); TEST_OPENGL_ERROR();

	GLuint lightSourcesBlockIdx = glGetUniformBlockIndex(lightingProgramID, "lightSourcesBlock");
	glUniformBlockBinding(lightingProgramID, lightSourcesBlockIdx, 0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_particleUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

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
	update();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); TEST_OPENGL_ERROR();

	drawParticlesVBO();

	drawCubeVAO();

	drawCellVAO();

	glutSwapBuffers();

	glutPostRedisplay();
}

void clear()
{
	glDeleteBuffers(1, &m_particleVboID); TEST_OPENGL_ERROR();
	glDeleteBuffers(1, &m_squarePositionVboID); TEST_OPENGL_ERROR();
	glDeleteBuffers(1, &m_squareNormalVBO); TEST_OPENGL_ERROR();
	glDeleteBuffers(1, &m_squareVao); TEST_OPENGL_ERROR();

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