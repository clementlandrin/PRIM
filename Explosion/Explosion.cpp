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

#define VISCOSITY 1.0f
#define DENSITY 1.0f
#define PARTICLE_NUMBER 1000
#define FLUID_DIMENSION 0.01

Fluid* fluid;

std::vector<float> vertexPositions;

void initPositions()
{
	vertexPositions.resize(fluid->GetParticles().size() * 3);
	
	for (int i = 0; i < fluid->GetParticles().size(); i++)
	{
		vertexPositions[3*i] = fluid->GetParticles()[i]->GetPosition().x;
		vertexPositions[3*i+1] = fluid->GetParticles()[i]->GetPosition().y;
		vertexPositions[3*i+2] = fluid->GetParticles()[i]->GetPosition().z;
	}
}

void initBuffer()
{
	GLuint m_vboID;
	glGenBuffers(1, &m_vboID);
	glBindBuffer(GL_ARRAY_BUFFER, m_vboID);

	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexPositions.size(), &(vertexPositions[0]), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initFluid()
{
	fluid = new Fluid(VISCOSITY, DENSITY);
	fluid->GenerateParticlesUniformly(PARTICLE_NUMBER, glm::vec3(0.0), FLUID_DIMENSION, FLUID_DIMENSION, FLUID_DIMENSION);
}

void init()
{
	initFluid();
	initPositions();
	initBuffer();
}

void update()
{
	fluid->UpdateParticlePositions(0.05);
}

void clear()
{
	delete fluid;
}

void display(void) {
	update();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glBegin(GL_POINTS);
	glColor4f(0.95f, 0.207, 0.031f, 1.0f);
	for (int i = 0; i < PARTICLE_NUMBER; ++i)
	{
		glVertex2f(fluid->GetParticles()[i]->GetPosition()[0], fluid->GetParticles()[i]->GetPosition()[1]);
	}
	glEnd();
	glutSwapBuffers();
	glutPostRedisplay();
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow(argv[0]);
	glutReshapeWindow(1024, 720);
	glPointSize(3.0);
	if (glewInit() != GLEW_OK)
	{
		return 1;
	}
	init();
	glutDisplayFunc(display);
	glutMainLoop();
	clear();
	return 0;
}
