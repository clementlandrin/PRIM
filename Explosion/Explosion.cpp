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
#define PARTICLE_NUMBER 100
#define FLUID_DIMENSION 0.01

Fluid* fluid;

std::vector<float> vertexPositions;

GLuint m_vboID;
GLuint programID;

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
	glGenBuffers(1, &m_vboID);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID);
	glBufferData(GL_ARRAY_BUFFER, vertexPositions.size() * sizeof(float), &(vertexPositions[0]), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initFluid()
{
	fluid = new Fluid(VISCOSITY, DENSITY);
	fluid->GenerateParticlesUniformly(PARTICLE_NUMBER, glm::vec3(0.0), FLUID_DIMENSION, FLUID_DIMENSION, FLUID_DIMENSION);
}

int init(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(720, 1024);
	glutCreateWindow(argv[0]);
	glutReshapeWindow(1024, 720);
	glPointSize(3.0);
	if (glewInit() != GLEW_OK)
	{
		return 1;
	}
	initFluid();
	initPositions();
	initBuffer();
	return 0;
}

void update()
{
	fluid->UpdateParticlePositions(0.05);
	updatePositions();
}

void render()
{
	update();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertexPositions.size() * sizeof(float), &(vertexPositions[0]));
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glDrawArrays(GL_POINTS, 0, vertexPositions.size());

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glutSwapBuffers();

	glutPostRedisplay();
}

void clear()
{
	glDeleteBuffers(1, &m_vboID);
	delete fluid;
}

int main(int argc, char **argv) {
	if (init(argc, argv) != 0)
	{
		return 1;
	}

	// Create and compile our GLSL program from the shaders
	programID = ShaderProgram::LoadShaders("Resources/vertex.shd", "Resources/fragment.shd");
	
	glutDisplayFunc(render);
	glutMainLoop();

	clear();
	return 0;
}

