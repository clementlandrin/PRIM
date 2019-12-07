// Explosion.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
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
#define PARTICLE_NUMBER 1000
#define FLUID_DIMENSION 0.01

Fluid* fluid;

std::vector<float> vertexPositions;
std::vector<float> squarePositions;

GLuint m_vboID;
GLuint m_squareVboID;
GLuint m_squareNormalVBO;

GLuint particleProgramID;
GLuint lightingProgramID;

#define TEST_OPENGL_ERROR()                                                             \
  do {		  							\
    GLenum err = glGetError(); 					                        \
    if (err != GL_NO_ERROR) std::cerr << "OpenGL ERROR! " << __LINE__ << std::endl;      \
  } while(0)

struct LightSource
{
	glm::vec3 position;
	glm::vec3 color;
	float intensity;
};

void updatePositions()
{
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

	glGenBuffers(1, &m_squareVboID); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squareVboID); TEST_OPENGL_ERROR();
	glBufferData(GL_ARRAY_BUFFER, squarePositions.size() * sizeof(float), &(squarePositions[0]), GL_STATIC_DRAW); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
}

void initFluid()
{
	fluid = new Fluid(VISCOSITY, DENSITY);
	fluid->GenerateParticlesUniformly(PARTICLE_NUMBER, glm::vec3(0.0), FLUID_DIMENSION, FLUID_DIMENSION, FLUID_DIMENSION);
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
}

void createSquare()
{
	int vertexPerFace = 2 * 3 * 3;
	squarePositions.resize(6 * vertexPerFace);

	float squareSize = 1.0f;
	bottomFace(0 * vertexPerFace, squareSize);

	leftFace(vertexPerFace, squareSize);

	rightFace(2 * vertexPerFace, squareSize);

	topFace(3 * vertexPerFace, squareSize);

	backFace(4 * vertexPerFace, squareSize);

	frontFace(5 * vertexPerFace, squareSize);
}

void initScene()
{
	createSquare();
}

int init(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(720, 1024); TEST_OPENGL_ERROR();
	glutCreateWindow(argv[0]); TEST_OPENGL_ERROR();
	glutReshapeWindow(1024, 720); TEST_OPENGL_ERROR();
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
	initBuffer();
	return 0;
}

void update()
{
	fluid->UpdateParticlePositions(0.01);
	updatePositions();
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
	glDrawArrays(GL_POINTS, 0, vertexPositions.size()); TEST_OPENGL_ERROR();

	glDisableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();

	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();

	ShaderProgram::set("lightPosition", glm::vec3(0.0), lightingProgramID); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_squareVboID); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, squarePositions.size() * sizeof(float), &(squarePositions[0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glDrawArrays(GL_TRIANGLES, 0, squarePositions.size()); TEST_OPENGL_ERROR();

	glDisableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();

	glutSwapBuffers();

	glutPostRedisplay();
}

void clear()
{
	glDeleteBuffers(1, &m_vboID); TEST_OPENGL_ERROR();
	delete fluid;
}

int main(int argc, char **argv) {
	if (init(argc, argv) != 0)
	{
		return 1;
	}

	// Create and compile our GLSL program from the shaders
	particleProgramID = ShaderProgram::LoadShaders("Resources/vertex.shd", "Resources/fragment.shd");
	lightingProgramID = ShaderProgram::LoadShaders("Resources/lighting_vertex.shd", "Resources/lighting_fragment.shd");

	glutDisplayFunc(render);
	glutMainLoop();

	clear();
	return 0;
}