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

#define SIMULATION_FILE_NAME "simulation_2.txt"

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

#define REAL_TIME_LIGHT_MAXIMUM_NUMBER 585

#define VISCOSITY 0.1f
#define DENSITY 0.05f // Stands for inertie
#define PARTICLE_NUMBER 5000//15000
#define FLUID_PROPORTION_IN_CUBE 0.20f
#define CUBE_SIZE 0.75f
#define RESOLUTION 8
#define SIMULATION_MAX_DURATION 0.0f//20000.0f

#define NUMBER_OF_SPHERE 2

float heightOffset = 0.0f;
float lateralOffset = 0.0f;
int depthToDisplay = -1;

bool drawCellCenter = false;
bool drawParticles = true;

std::ifstream in;
std::ofstream out;

bool pause = false;

int frameNumber = 0;

bool shouldGenerateNewParticlesEachFrame = false;
bool shouldRenderLighting = true;
bool shouldRegisterSimulation = false;
bool shouldPlayRegisteredSimulation = true;
bool shouldColorLightingWithDistance = true;
bool shouldColorFire = true;

bool isRotating = false;
glm::vec2 lastRotatingCursorPosition;

static float initialTime;
static float zeroTimeOfSimulation;

int clamped_power_of_two_resolution;
int power_of_two_resolution;

void computeRayTracedImage();
float blendFunction(float distance, int depth);
void createSquare();

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
	float norm() { return sqrt(x * x + y * y + z * z); }
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
	int depth;
	Sphere(double rad_, Vec p_, Vec e_, Vec c_, Refl_t refl_, int depth_) : radius(rad_), p(p_), e(e_), c(c_), refl(refl_), depth(depth_) {}

	double intersect(const Ray &r) const
	{ // returns distance, 0 if nohit
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

struct LightSource
{
	glm::vec4 position;
	glm::vec4 color_and_intensity;
	glm::vec4 depth;
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

std::vector<std::vector<float>> spherePositions;
std::vector<std::vector<float>> sphereNormals;
std::vector<std::vector<int>> sphereIndices;

std::vector<float> cellPositions;
std::vector<float> cellColorAndIntensity;
std::vector<int> cellDepth;

std::vector<float> cellPositionsForRayTracer;
std::vector<float> cellColorAndIntensityForRayTracer;
std::vector<float> cellDepthForRayTracer;

std::vector<Sphere*> spheres;
std::vector<LightSource> lightSources;
std::vector<RegularGrid*> regularGrids;
std::vector<glm::vec4> blendingRadius;

OctreeNode* octreeRoot;

GLuint m_particleVboID;
GLuint m_particleUBO;

GLuint m_squarePositionVboID;
GLuint m_squareNormalVBO;
GLuint m_squareVao;

GLuint m_spherePositionVboID[NUMBER_OF_SPHERE];
GLuint m_sphereNormalVBO[NUMBER_OF_SPHERE];
GLuint m_sphereIBO[NUMBER_OF_SPHERE];
GLuint m_sphereVao[NUMBER_OF_SPHERE];

GLuint m_cellPositionVboID;
GLuint m_cellColorAndIntensityVboID;
GLuint m_cellVao;

GLuint particleProgramID;
GLuint lightingProgramID;
GLuint cellProgramID;

float cellIntensity = 0.0f;

glm::vec3 currentSizeOfGrid = glm::vec3(2.0*FLUID_PROPORTION_IN_CUBE*CUBE_SIZE*2.0);

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
	case '0':
		std::cout << "Pressed key up : ";

		if (depthToDisplay + 1 <= clamped_power_of_two_resolution)
		{
			depthToDisplay += 1;
			std::cout << "Updating depth to display to " << depthToDisplay << std::endl;
			glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();
			ShaderProgram::set("depth_to_display", depthToDisplay, lightingProgramID); TEST_OPENGL_ERROR();
			glUseProgram(0); TEST_OPENGL_ERROR();
		}
		else
		{
			std::cout << "Nothing happens" << std::endl;
		}
		break;
	case '1':
		std::cout << "Pressed key down : ";

		if (depthToDisplay - 1 >= -1)
		{
			depthToDisplay -= 1;
			std::cout << "Updating depth to display to " << depthToDisplay << std::endl;
			glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();
			ShaderProgram::set("depth_to_display", depthToDisplay, lightingProgramID); TEST_OPENGL_ERROR();
			glUseProgram(0); TEST_OPENGL_ERROR();
		}
		else
		{
			std::cout << "Nothing happens" << std::endl;
		}
		break;
	case 'c':
		std::cout << "Pressed key c : ";

		drawCellCenter = !drawCellCenter;
		if (drawCellCenter)
		{
			std::cout << "Draw cell center" << std::endl;
		}
		else
		{
			std::cout << "Won't draw cell center" << std::endl;
		}
		break;
	case 'p':
		std::cout << "Pressed key p : ";

		drawParticles = !drawParticles;
		if (drawParticles)
		{
			std::cout << "Draw particles" << std::endl;
		}
		else
		{
			std::cout << "Won't draw particles" << std::endl;
		}
		break;
	case 'u':
		std::cout << "Bottom face of the cube moves up" << std::endl;
		
		heightOffset += 0.02f;
		createSquare();
		break;
	case 'd':
		std::cout << "Top face of the cube moves down" << std::endl;
		heightOffset -= 0.02f;
		createSquare();
		break;
	case 'l':
		std::cout << "Left face of the cube moves left" << std::endl;
		lateralOffset += 0.02f;
		createSquare();
		break;
	case 'r':
		std::cout << "Left face of the cube moves right" << std::endl;
		lateralOffset -= 0.02f;
		createSquare();
		break;
	case 'f':
		std::cout << "Toggle fire coloration : ";
		shouldColorFire = !shouldColorFire;
		if (!shouldColorFire)
		{
			std::cout << "No fire coloration" << std::endl;
		}
		else
		{
			std::cout << "Fire coloration enabled" << std::endl;
		}
		break;
	case 'g':
		std::cout << "Toggle fire coloration with distance: ";
		shouldColorLightingWithDistance = !shouldColorLightingWithDistance;
		glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();
		ShaderProgram::set("coloration_with_distance", shouldColorLightingWithDistance, lightingProgramID); TEST_OPENGL_ERROR();
		glUseProgram(0); TEST_OPENGL_ERROR();
		if (!shouldColorLightingWithDistance)
		{
			std::cout << "No fire coloration with distance" << std::endl;
		}
		else
		{
			std::cout << "Fire coloration with distance enabled" << std::endl;
		}
		break;
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

glm::vec3 computeFireColor(int depth, int numberOfParticlesInCell)
{
	float proportion = (float)numberOfParticlesInCell / (float)PARTICLE_NUMBER * pow(8.0, depth) / 2.0f; //* pow(8.0, depth) / 2.0f;
	glm::vec3 color = glm::vec3(1.0f);
	if (!shouldColorFire)
	{
		return color;
	}
	if (proportion > 6.0f / 8.0f)
	{
		color.z = glm::clamp((proportion - 6.0f / 8.0f) * 8.0f / 2.0f, 0.0f, 1.0f);
		color.x = 1.0f;
		color.y = 1.0f;
		return color;
	}
	else if (proportion > 4.0f / 8.0f)
	{
		color.y = glm::clamp((proportion - 4.0f / 8.0f) * 8.0f / 2.0f, 0.0f, 1.0f);
		color.x = 1.0f;
		color.z = 0.0f;
		return color;
	}
	else if (proportion > 2.0f / 8.0f)
	{
		color.y = 0.0f;
		color.z = 0.0f;
		color.x = glm::clamp((proportion - 2.0f / 8.0f) * 8.0f / 2.0f, 0.0f, 1.0f);
		return color;
	}
	else
	{
		return glm::vec3(0.0f);
	}
}

void addCellToCellPositionVector(OctreeNode* octreeNode, bool shouldAddToRayTracer, bool shouldAddCellToVector = false)
{
	float numberOfParticlesInCell = octreeNode->GetCell()->GetParticles().size();
	glm::vec3 color = computeFireColor(octreeNode->GetDepth(), numberOfParticlesInCell);

	if (shouldAddCellToVector)
	{
		if (octreeNode->GetDepth() <= clamped_power_of_two_resolution)
		{
			cellPositions.push_back(octreeNode->GetCell()->ComputeCenter(shouldPlayRegisteredSimulation)[0]);
			cellPositions.push_back(octreeNode->GetCell()->ComputeCenter(shouldPlayRegisteredSimulation)[1]);
			cellPositions.push_back(octreeNode->GetCell()->ComputeCenter(shouldPlayRegisteredSimulation)[2]);
		}
	}

	if (octreeNode->GetDepth() <= clamped_power_of_two_resolution)
	{
		cellColorAndIntensity.push_back(color.x);
		cellColorAndIntensity.push_back(color.y);
		cellColorAndIntensity.push_back(color.z);
		cellColorAndIntensity.push_back(glm::max((float)((float)numberOfParticlesInCell / (float)PARTICLE_NUMBER), 0.0f));

		cellDepth.push_back(octreeNode->GetDepth());
	}

	if (shouldAddToRayTracer)
	{
		cellPositionsForRayTracer.push_back(octreeNode->GetCell()->ComputeCenter(shouldPlayRegisteredSimulation)[0]);
		cellPositionsForRayTracer.push_back(octreeNode->GetCell()->ComputeCenter(shouldPlayRegisteredSimulation)[1]);
		cellPositionsForRayTracer.push_back(octreeNode->GetCell()->ComputeCenter(shouldPlayRegisteredSimulation)[2]);

		cellColorAndIntensityForRayTracer.push_back(color.x);
		cellColorAndIntensityForRayTracer.push_back(color.y);
		cellColorAndIntensityForRayTracer.push_back(color.z);
		cellColorAndIntensityForRayTracer.push_back(glm::max((float)((float)numberOfParticlesInCell / (float)PARTICLE_NUMBER), 0.0f));

		cellDepthForRayTracer.push_back(octreeNode->GetDepth());
	}
}

void UpdateCellVectors(OctreeNode* octreeNode, bool shouldAddToRayTracer, bool shouldAddCellToVector = false)
{	
	addCellToCellPositionVector(octreeNode, shouldAddToRayTracer, shouldAddCellToVector);

	if (octreeNode->GetDepth() < clamped_power_of_two_resolution || shouldAddToRayTracer)
	{
		if (!octreeNode->GetIsALeaf())
		{
			std::vector<std::vector<std::vector<OctreeNode*>>> children = octreeNode->GetChildren();
			UpdateCellVectors(children[0][0][0], shouldAddToRayTracer, shouldAddCellToVector);
			UpdateCellVectors(children[0][0][1], shouldAddToRayTracer, shouldAddCellToVector);
			UpdateCellVectors(children[0][1][0], shouldAddToRayTracer, shouldAddCellToVector);
			UpdateCellVectors(children[0][1][1], shouldAddToRayTracer, shouldAddCellToVector);
			UpdateCellVectors(children[1][0][0], shouldAddToRayTracer, shouldAddCellToVector);
			UpdateCellVectors(children[1][0][1], shouldAddToRayTracer, shouldAddCellToVector);
			UpdateCellVectors(children[1][1][0], shouldAddToRayTracer, shouldAddCellToVector);
			UpdateCellVectors(children[1][1][1], shouldAddToRayTracer, shouldAddCellToVector);
		}
	}
}

void initCellVAO()
{
	UpdateCellVectors(octreeRoot, false, true);
	glCreateBuffers(1, &m_cellPositionVboID); TEST_OPENGL_ERROR(); // Generate a GPU buffer to store the positions of the vertices
	numberOfCells = 0;
	for (int i = 0; i < clamped_power_of_two_resolution + 1; i++)
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

void initSphereVAO(int index)
{
	glCreateBuffers(1, &m_spherePositionVboID[index]); TEST_OPENGL_ERROR(); // Generate a GPU buffer to store the positions of the vertices
	size_t vertexBufferSize = sizeof(float) * spherePositions[index].size(); // Gather the size of the buffer from the CPU-side vector
	glNamedBufferStorage(m_spherePositionVboID[index], vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR(); // Create a data store on the GPU
	glNamedBufferSubData(m_spherePositionVboID[index], 0, vertexBufferSize, spherePositions[index].data()); TEST_OPENGL_ERROR(); // Fill the data store from a CPU array

	glCreateBuffers(1, &m_sphereNormalVBO[index]); TEST_OPENGL_ERROR(); // Same for normal
	glNamedBufferStorage(m_sphereNormalVBO[index], vertexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR();
	glNamedBufferSubData(m_sphereNormalVBO[index], 0, vertexBufferSize, sphereNormals[index].data()); TEST_OPENGL_ERROR();

	glCreateBuffers(1, &m_sphereIBO[index]); TEST_OPENGL_ERROR(); // Same for the index buffer, that stores the list of indices of the triangles forming the mesh
	size_t indexBufferSize = sizeof(float) * sphereIndices[index].size();
	glNamedBufferStorage(m_sphereIBO[index], indexBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT); TEST_OPENGL_ERROR();
	glNamedBufferSubData(m_sphereIBO[index], 0, indexBufferSize, sphereIndices[index].data()); TEST_OPENGL_ERROR();

	glCreateVertexArrays(1, &m_sphereVao[index]); TEST_OPENGL_ERROR(); // Create a single handle that joins together attributes (vertex positions, normals) and connectivity (triangles indices)
	glBindVertexArray(m_sphereVao[index]); TEST_OPENGL_ERROR();
	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_spherePositionVboID[index]); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();
	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_sphereNormalVBO[index]); TEST_OPENGL_ERROR();
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereIBO[index]); TEST_OPENGL_ERROR();
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
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightSource) * glm::min(cellPositions.size()/3, (unsigned int)REAL_TIME_LIGHT_MAXIMUM_NUMBER) + 3 * sizeof(glm::vec4) + sizeof(glm::vec4) * NUMBER_OF_SPHERE + sizeof(glm::vec4) * blendingRadius.size(), NULL, GL_DYNAMIC_DRAW); TEST_OPENGL_ERROR();
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

	for (int i = 0; i < NUMBER_OF_SPHERE; i++)
	{
		initSphereVAO(i);
	}

	////////////////////////////////

	initParticleUBO();
}

void generateParticles(int particleNumber)
{
	float fluidInitialSpace = FLUID_PROPORTION_IN_CUBE * CUBE_SIZE;
	fluid->GenerateParticlesUniformly(particleNumber, glm::vec3(0.0f, 0.0f, 0.0f), fluidInitialSpace, fluidInitialSpace, fluidInitialSpace, 0.1f);
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
	squarePositions[1 + offset] = heightOffset * size;
	squarePositions[2 + offset] = -1.0f * size;

	squarePositions[3 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[4 + offset] = heightOffset * size;
	squarePositions[5 + offset] = -1.0f * size;

	squarePositions[6 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[7 + offset] = heightOffset * size;
	squarePositions[8 + offset] = 1.0f * size;

	squarePositions[9 + offset] = 1.0f * size;
	squarePositions[10 + offset] = heightOffset * size;
	squarePositions[11 + offset] = -1.0f * size;

	squarePositions[12 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[13 + offset] = heightOffset * size;
	squarePositions[14 + offset] = 1.0f * size;

	squarePositions[15 + offset] = 1.0f * size;
	squarePositions[16 + offset] = heightOffset * size;
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
	squarePositions[0 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[1 + offset] = 1.0f * size;
	squarePositions[2 + offset] = -1.0f * size;

	squarePositions[3 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[4 + offset] = 1.0f * size;
	squarePositions[5 + offset] = 1.0f * size;

	squarePositions[6 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[7 + offset] = heightOffset * size;
	squarePositions[8 + offset] = -1.0f * size;

	squarePositions[9 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[10 + offset] = heightOffset * size;
	squarePositions[11 + offset] = -1.0f * size;

	squarePositions[12 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[13 + offset] = 1.0f * size;
	squarePositions[14 + offset] = 1.0f * size;

	squarePositions[15 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[16 + offset] = heightOffset * size;
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
	squarePositions[4 + offset] = heightOffset * size;
	squarePositions[5 + offset] = -1.0f * size;

	squarePositions[6 + offset] = 1.0f * size;
	squarePositions[7 + offset] = 1.0f * size;
	squarePositions[8 + offset] = 1.0f * size;

	squarePositions[9 + offset] = 1.0f * size;
	squarePositions[10 + offset] = 1.0f * size;
	squarePositions[11 + offset] = 1.0f * size;

	squarePositions[12 + offset] = 1.0f * size;
	squarePositions[13 + offset] = heightOffset * size;
	squarePositions[14 + offset] = -1.0f * size;

	squarePositions[15 + offset] = 1.0f * size;
	squarePositions[16 + offset] = heightOffset * size;
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
	squarePositions[0 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[1 + offset] = 1.0f * size;
	squarePositions[2 + offset] = 1.0f * size;

	squarePositions[3 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[4 + offset] = 1.0f * size;
	squarePositions[5 + offset] = -1.0f * size;

	squarePositions[6 + offset] = 1.0f * size;
	squarePositions[7 + offset] = 1.0f * size;
	squarePositions[8 + offset] = -1.0f * size;

	squarePositions[9 + offset] = -1.0f * size + lateralOffset * size;
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
	squarePositions[0 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[1 + offset] = 1.0f * size;
	squarePositions[2 + offset] = -1.0f * size;

	squarePositions[3 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[4 + offset] = heightOffset * size;
	squarePositions[5 + offset] = -1.0f * size;

	squarePositions[6 + offset] = 1.0f * size;
	squarePositions[7 + offset] = heightOffset * size;
	squarePositions[8 + offset] = -1.0f * size;

	squarePositions[9 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[10 + offset] = 1.0f * size;
	squarePositions[11 + offset] = -1.0f * size;

	squarePositions[12 + offset] = 1.0f * size;
	squarePositions[13 + offset] = heightOffset * size;
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
	squarePositions[1 + offset] = heightOffset * size;
	squarePositions[2 + offset] = 1.0f * size;

	squarePositions[3 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[4 + offset] = heightOffset * size;
	squarePositions[5 + offset] = 1.0f * size;

	squarePositions[6 + offset] = 1.0f * size;
	squarePositions[7 + offset] = 1.0f * size;
	squarePositions[8 + offset] = 1.0f * size;

	squarePositions[9 + offset] = 1.0f * size;
	squarePositions[10 + offset] = 1.0f * size;
	squarePositions[11 + offset] = 1.0f * size;

	squarePositions[12 + offset] = -1.0f * size + lateralOffset * size;
	squarePositions[13 + offset] = heightOffset * size;
	squarePositions[14 + offset] = 1.0f * size;

	squarePositions[15 + offset] = -1.0f * size + lateralOffset * size;
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
	squarePositions.clear();
	squareNormals.clear();
	squarePositions.resize(6 * vertexPerFace);
	squareNormals.resize(6 * vertexPerFace);

	bottomFace(0 * vertexPerFace, CUBE_SIZE);

	leftFace(vertexPerFace, CUBE_SIZE);

	rightFace(2 * vertexPerFace, CUBE_SIZE);

	topFace(3 * vertexPerFace, CUBE_SIZE);

	backFace(4 * vertexPerFace, CUBE_SIZE);

	frontFace(5 * vertexPerFace, CUBE_SIZE);
}

void createSphere(glm::vec3 origin, float radius, int resolution, int index)
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
			spherePositions[index].push_back(x + origin.x);
			spherePositions[index].push_back(y + origin.y);
			spherePositions[index].push_back(z + origin.z);

			// normalized vertex normal (nx, ny, nz)
			nx = x * lengthInv;
			ny = y * lengthInv;
			nz = z * lengthInv;
			sphereNormals[index].push_back(nx);
			sphereNormals[index].push_back(ny);
			sphereNormals[index].push_back(nz);
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
				sphereIndices[index].push_back(k1);
				sphereIndices[index].push_back(k2);
				sphereIndices[index].push_back(k1 + 1);
			}

			// k1+1 => k2 => k2+1
			if (i != (resolution - 1))
			{
				sphereIndices[index].push_back(k1 + 1);
				sphereIndices[index].push_back(k2);
				sphereIndices[index].push_back(k2 + 1);
			}
		}
	}
}

void translateSphere(glm::vec3 translation, int index)
{
	for (int i = 0; i < spherePositions[index].size() / 3; ++i)
	{
		spherePositions[index][3 * i] += translation.x;
		spherePositions[index][3 * i + 1] += translation.y;
		spherePositions[index][3 * i + 2] += translation.z;
	}
}

void initScene()
{
	createSquare();

	int sphere_resolution = 100;

	spheres.push_back(new Sphere(0.1, Vec(-0.5, 0.0, 0.0), Vec(1.0, 1.0, 1.0), Vec(1.0, 0.0, 0.0), Refl_t::DIFFUSE, -1));
	spheres.push_back(new Sphere(0.2, Vec(-0.1, -0.4, 0.0), Vec(1.0, 1.0, 1.0), Vec(0.0, 0.0, 1.0), Refl_t::DIFFUSE, -1));

	spherePositions.resize(NUMBER_OF_SPHERE);
	sphereNormals.resize(NUMBER_OF_SPHERE);
	sphereIndices.resize(NUMBER_OF_SPHERE);

	for (int i = 0; i < NUMBER_OF_SPHERE; i++)
	{
		createSphere(glm::vec3(spheres[i]->p.x, spheres[i]->p.y, spheres[i]->p.z), spheres[i]->radius, sphere_resolution, i);
	}

	power_of_two_resolution = 0;
	clamped_power_of_two_resolution = 0;
	float temp = (float)RESOLUTION;

	while (temp > 1.0)
	{
		temp = temp / 2.0;
		if (clamped_power_of_two_resolution < 3)
		{
			clamped_power_of_two_resolution++;
		}
		power_of_two_resolution++;
	}

	std::cout << clamped_power_of_two_resolution << std::endl;
	std::cout << power_of_two_resolution << std::endl;
	for (int i = 0; i < power_of_two_resolution + 1; i++)
	{
		regularGrids.push_back(new RegularGrid(pow(2, i), currentSizeOfGrid));
	}
	regularGrids[0]->GetCells()[0][0][0]->SetParticles(fluid->GetParticles());
	regularGrids[0]->GetCells()[0][0][0]->UpdateSpeed();

	blendingRadius.resize(clamped_power_of_two_resolution + 1);

	for (int i = 0; i < clamped_power_of_two_resolution + 1; i++)
	{
		blendingRadius[i] = glm::vec4(1 - (float)(i+1)/(float)(clamped_power_of_two_resolution+2), 0.0f, 0.0f, 0.0f);
		std::cout << "Blending radius " << i << " : " << blendingRadius[i].x << std::endl;
	}
	
	blendingRadius[0] = glm::vec4(1.2f, 0.0f, 0.0f, 0.0f);
	if (blendingRadius.size() > 1)
	{
		blendingRadius[1] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
		if (blendingRadius.size() > 2)
		{
			blendingRadius[2] = glm::vec4(0.8f, 0.0f, 0.0f, 0.0f);
			if (blendingRadius.size() > 3)
			{
				blendingRadius[3] = glm::vec4(0.6f, 0.0f, 0.0f, 0.0f);
			}
		}
	}
}

void initShaders()
{
	// Create and compile our GLSL program from the shaders
	particleProgramID = ShaderProgram::LoadShaders("Resources/vertex.shd", "Resources/fragment.shd");
	lightingProgramID = ShaderProgram::LoadShaders("Resources/lighting_vertex.shd", "Resources/lighting_fragment.shd");
	cellProgramID = ShaderProgram::LoadShaders("Resources/cell_vertex.shd", "Resources/cell_fragment.shd");

	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("depth_to_display", depthToDisplay, lightingProgramID); TEST_OPENGL_ERROR();
	ShaderProgram::set("coloration_with_distance", shouldColorLightingWithDistance, lightingProgramID); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();

	updateUniformMatrixOfShaders();
}

void initOctree()
{
	int position_of_root_in_grid[3] = { 0, 0, 0 };
	octreeRoot = OctreeNode::BuildOctree(0, power_of_two_resolution - 1, regularGrids[0]->GetCells()[0][0][0], position_of_root_in_grid, regularGrids);
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

void skipFrameInFile(std::ifstream &in)
{
	frameNumber++;
	int numberOfParticles;
	glm::vec3 position;

	in.read((char *)&numberOfParticles, sizeof(int));
	if (numberOfParticles == -1)
	{
		in.close();
		in.open(SIMULATION_FILE_NAME, std::ios_base::binary);
	}
	else if (numberOfParticles <= 0)
	{
		std::cout << "Read " << numberOfParticles << " particles from file" << std::endl;
	}
	else
	{
		in.seekg(numberOfParticles * sizeof(float) * 3, std::ios::cur);
	}
}

bool readFrameInFile(std::ifstream &in)
{
	frameNumber++;
	int numberOfParticles;
	glm::vec3 position;
	
	in.read((char *)&numberOfParticles, sizeof(int));
	if (numberOfParticles == -1)
	{
		in.close();
		in.open(SIMULATION_FILE_NAME, std::ios_base::binary);
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
	cellIntensity = 0.0f;
	frameNumber += 1;

	glm::vec3 sphereNewPosition = glm::vec3(0.5f * cos(currentTime/100000.0f), 0.0f, 0.5f * sin(currentTime/100000.0f));
	translateSphere(sphereNewPosition - glm::vec3(spheres[0]->p.x, spheres[0]->p.y, spheres[0]->p.z), 0);
	spheres[0]->p = Vec(sphereNewPosition.x, sphereNewPosition.y, sphereNewPosition.z);


	float dt = currentTime - initialTime;
	initialTime = currentTime;

	float timeFactor;
	if (realTimeSimulation)
	{
		timeFactor = dt * 0.0005f;
	}
	else
	{
		timeFactor = 0.0005f;
	}

	if (shouldPlayRegisteredSimulation)
	{
		clearParticles();
		currentSizeOfGrid = glm::vec3(CUBE_SIZE*2.0);

		if (in.good())
		{
			/*while (frameNumber % 1 != 0)
			{
				skipFrameInFile(in);
			}*/
			if (!readFrameInFile(in))
			{
				in.close();
				in.open(SIMULATION_FILE_NAME, std::ios_base::binary);
			}
		}

		regularGrids[0]->GetCells()[0][0][0]->SetParticles(fluid->GetParticles());

		octreeRoot->SetCell(regularGrids[0]->GetCells()[0][0][0]);
		octreeRoot->UpdateParticlesInChildrenCells();

		cellColorAndIntensity.clear();
		cellDepth.clear();
		cellPositions.clear();
		UpdateCellVectors(octreeRoot, false, true);
		
		for (int i = 0; i < regularGrids.size(); i++)
		{
			regularGrids[i]->ResizeGrid(currentSizeOfGrid);
		}
	}
	else
	{
		glm::vec3 maxSpeed = regularGrids[regularGrids.size() - 1]->UpdateSpeedOfCells();
		currentSizeOfGrid[0] = glm::min(currentSizeOfGrid[0] + maxSpeed[0] * timeFactor * 2.0f, 2.0f * CUBE_SIZE);
		currentSizeOfGrid[1] = glm::min(currentSizeOfGrid[1] + maxSpeed[1] * timeFactor * 2.0f, 2.0f * CUBE_SIZE);
		currentSizeOfGrid[2] = glm::min(currentSizeOfGrid[2] + maxSpeed[2] * timeFactor * 2.0f, 2.0f * CUBE_SIZE);

		fluid->UpdateParticlePositions(timeFactor * CUBE_SIZE, CUBE_SIZE);

		if (shouldGenerateNewParticlesEachFrame && fluid->GetParticles().size() + PARTICLE_NUMBER * 0.01 < 10 * PARTICLE_NUMBER)
		{
			generateParticles(PARTICLE_NUMBER * 0.01);
		}

		if (shouldRegisterSimulation)
		{
			if (!out.is_open())
			{
				out = std::ofstream(SIMULATION_FILE_NAME, std::ios::out | std::ios::binary);
			}
			if (out.good())
			{
				writeFrameInFile(out);
			}
		}

		regularGrids[regularGrids.size() - 1]->UpdateGradientAndVGradVOfCells();
		regularGrids[regularGrids.size() - 1]->UpdateLaplacianOfCells();
		regularGrids[regularGrids.size() - 1]->UpdateSpeedVariationNavierStokes(fluid);
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
			in = std::ifstream(SIMULATION_FILE_NAME, std::ios_base::binary);
			frameNumber = 0;
		}
	}

	updatePositions();

	if (shouldRegisterSimulation)
	{
		for (int i = 0; i < regularGrids.size(); i++)
		{
			regularGrids[i]->ResizeGrid(currentSizeOfGrid);
		}
		regularGrids[0]->GetCells()[0][0][0]->SetParticles(fluid->GetParticles());

		octreeRoot->SetCell(regularGrids[0]->GetCells()[0][0][0]);
		octreeRoot->UpdateParticlesInChildrenCells();

		cellColorAndIntensity.clear();
		cellDepth.clear();
		cellPositions.clear();
		UpdateCellVectors(octreeRoot, false, true);
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

void updateUBO()
{
	if (shouldRenderLighting)
	{
		lightSources.resize(cellPositions.size() / 3);

		for (int i = 0; i < cellPositions.size() / 3; i++)
		{
			lightSources[i].position = glm::vec4(cellPositions[3 * i], cellPositions[3 * i + 1], cellPositions[3 * i + 2], 1.0);
			lightSources[i].color_and_intensity = glm::vec4(cellColorAndIntensity[4 * i], cellColorAndIntensity[4 * i + 1], cellColorAndIntensity[4 * i + 2], cellColorAndIntensity[4 * i + 3]);
			lightSources[i].depth = glm::vec4((float)cellDepth[i], 0.0f, 0.0f, 0.0f);
		}
	}
	else
	{
		lightSources.resize(1);
		lightSources[0].position = glm::vec4(0.0f);
		lightSources[0].color_and_intensity = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		lightSources[0].depth = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	std::vector<glm::vec4> spherePositionAndRadius;
	for (int i = 0; i < NUMBER_OF_SPHERE; i++)
	{
		spherePositionAndRadius.push_back(glm::vec4(spheres[i]->p.x, spheres[i]->p.y, spheres[i]->p.z, spheres[i]->radius));
	}
	GLsizei sizeOfSphereArray = spherePositionAndRadius.size() * sizeof(glm::vec4);
	glm::vec4 actualNumberOfSpheres[1] = { glm::vec4(NUMBER_OF_SPHERE, 0.0f, 0.0f, 0.0f) };
	glm::vec4 numberOfRadius[1] = { glm::vec4(blendingRadius.size(), 0.0f, 0.0f, 0.0f) };

	glBindBuffer(GL_UNIFORM_BUFFER, m_particleUBO); TEST_OPENGL_ERROR();
	glm::vec4 actualNumberOfCells[1] = { glm::vec4(lightSources.size() - 1, 0.0f, 0.0f, 0.0f) };
	GLsizei sizeOfArray = glm::min(lightSources.size() - 1, (unsigned int)REAL_TIME_LIGHT_MAXIMUM_NUMBER) * sizeof(LightSource);
	if (!shouldRenderLighting)
	{
		GLsizei sizeOfArray = glm::min(lightSources.size(), (unsigned int)REAL_TIME_LIGHT_MAXIMUM_NUMBER) * sizeof(LightSource);
	}
	GLsizei sizeBlendingRadiusArray = blendingRadius.size() * sizeof(glm::vec4);
	if (!shouldRenderLighting)
	{
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeOfArray, &(lightSources[0])); TEST_OPENGL_ERROR();
	}
	else
	{
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeOfArray, &(lightSources[1])); TEST_OPENGL_ERROR();
	}
	glBufferSubData(GL_UNIFORM_BUFFER, sizeOfArray, sizeof(glm::vec4), &(actualNumberOfCells)); TEST_OPENGL_ERROR();
	glBufferSubData(GL_UNIFORM_BUFFER, sizeOfArray + sizeof(glm::vec4), sizeOfSphereArray, &(spherePositionAndRadius[0])); TEST_OPENGL_ERROR();
	glBufferSubData(GL_UNIFORM_BUFFER, sizeOfArray + sizeof(glm::vec4) + sizeOfSphereArray, sizeof(glm::vec4), &(actualNumberOfSpheres)); TEST_OPENGL_ERROR();
	glBufferSubData(GL_UNIFORM_BUFFER, sizeOfArray + 2 * sizeof(glm::vec4) + sizeOfSphereArray, sizeBlendingRadiusArray, &(blendingRadius[0])); TEST_OPENGL_ERROR();
	glBufferSubData(GL_UNIFORM_BUFFER, sizeOfArray + 2 * sizeof(glm::vec4) + sizeOfSphereArray + sizeBlendingRadiusArray, sizeof(glm::vec4), &(numberOfRadius)); TEST_OPENGL_ERROR();

	GLuint lightSourcesBlockIdx = glGetUniformBlockIndex(lightingProgramID, "lightSourcesBlock"); TEST_OPENGL_ERROR();
	glUniformBlockBinding(lightingProgramID, lightSourcesBlockIdx, 0); TEST_OPENGL_ERROR();
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_particleUBO); TEST_OPENGL_ERROR();
	glBindBuffer(GL_UNIFORM_BUFFER, 0); TEST_OPENGL_ERROR();
}

void drawSphereVAO(int index)
{
	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();

	ShaderProgram::set("albedo",glm::vec4(spheres[index]->c.x, spheres[index]->c.y, spheres[index]->c.z, 1.0f), lightingProgramID); TEST_OPENGL_ERROR();

	updateUBO();

	glBindVertexArray(m_sphereVao[index]); TEST_OPENGL_ERROR(); // Activate the VAO storing geometry data
	glEnableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_spherePositionVboID[index]); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, spherePositions[index].size() * sizeof(float), &(spherePositions[index][0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glEnableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, m_sphereNormalVBO[index]); TEST_OPENGL_ERROR();
	glBufferSubData(GL_ARRAY_BUFFER, 0, sphereNormals[index].size() * sizeof(float), &(sphereNormals[index][0])); TEST_OPENGL_ERROR();
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); TEST_OPENGL_ERROR();

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei> (sphereIndices[index].size()), GL_UNSIGNED_INT, 0); // Call for rendering: stream the current GPU geometry through the current GPU program

	glDisableVertexAttribArray(0); TEST_OPENGL_ERROR();
	glDisableVertexAttribArray(1); TEST_OPENGL_ERROR();
	glBindBuffer(GL_ARRAY_BUFFER, 0); TEST_OPENGL_ERROR();
	glUseProgram(0); TEST_OPENGL_ERROR();
}

void drawCubeVAO()
{
	glUseProgram(lightingProgramID); TEST_OPENGL_ERROR();

	ShaderProgram::set("albedo", glm::vec4(1.0), lightingProgramID); TEST_OPENGL_ERROR();

	updateUBO();

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

	if (drawParticles)
	{
		drawParticlesVBO();
	}

	drawCubeVAO();

	for (int i = 0; i < NUMBER_OF_SPHERE; i++)
	{
		drawSphereVAO(i);
	}


	if (drawCellCenter)
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
	for (int i = 0; i < NUMBER_OF_SPHERE; i++)
	{
		glDeleteBuffers(1, &m_spherePositionVboID[i]); TEST_OPENGL_ERROR();
	}
	glDeleteBuffers(1, &m_squareNormalVBO); TEST_OPENGL_ERROR();
	glDeleteBuffers(1, &m_squareVao); TEST_OPENGL_ERROR();

	for (int i = 0; i < regularGrids.size(); i++)
	{
		delete regularGrids[i];
	}

	for (int i = 0; i < spheres.size(); i++)
	{
		delete spheres[i];
	}

	delete octreeRoot;
	delete fluid;
}

// Scene :
//std::vector<Sphere> spheres;
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

float modifyRadiance(float &radiance, float intensity)
{
	if (intensity > 0.25f && intensity < 0.5f)
	{
		return radiance * 4.0f * (0.5f - intensity);
	}
	else
	{
		return radiance + 2.0f * (intensity - 0.5f);
	}
}

inline bool intersectScene(const Ray &r, double &t, int &id, bool intersectWithEmissive = false, int lightID = -1)
{
	double d, inf = t = 1e20;
	for (int i = 0; i < spheres.size(); ++i)
	{
		if (intersectWithEmissive)
		{
			if (spheres[i]->refl == EMMISSIVE)
			{
				if (lightID != -1 && i == lightID)
				{
					if (blendFunction(Vec(r.o - spheres[i]->p).norm(), spheres[i]->depth) > 0.0)
					{
						if ((d = spheres[i]->intersect(r)) && d < t)
						{
							t = d;
							id = i;
						}
					}
				}
				else if (lightID == -1)
				{
					if (blendFunction(Vec(r.o - spheres[i]->p).norm(), spheres[i]->depth) > 0.0)
					{
						if ((d = spheres[i]->intersect(r)) && d < t)
						{
							t = d;
							id = i;
						}
					}
				}
			}
			else
			{
				if ((d = spheres[i]->intersect(r)) && d < t)
				{
					t = d;
					id = i;
				}
			}
		}
		else if (spheres[i]->refl != EMMISSIVE)
		{
			if ((d = spheres[i]->intersect(r)) && d < t)
			{
				t = d;
				id = i;
			}
		}
		else
		{
			/*if ((d = spheres[i]->intersect(r)) && d < t)
			{
				modifyRadiance(&rad, spheres[i]->e.norm());
			}*/
		}
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
	if (!intersectScene(r, t, id, depth != 0))
		return Vec(); // if miss, return black

	const Sphere &obj = *spheres[id]; // the hit object

	Vec x = r.o + r.d * t,           // the hit position
		n = (x - obj.p).normalize(), // the normal of the object at the hit
		f = obj.c;                   // the color of the object at the hit

	if (n.dot(r.d) > 0.0)
		n = -1.0 * n;

	if (++depth > 1)
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
			const Sphere &light = *spheres[lights[lightIt]];
			const Vec Li = light.e;
			if (Li.x != 0.0 && Li.y != 0.0 && Li.z != 0.0)
			{
				// TODO
				const Vec dirToLight = (light.randomSample() - x).normalize();
				const Ray &toLight = Ray(x + 0.0001 * dirToLight, dirToLight);
				int idTemp;
				double tTemp;
				if (intersectScene(toLight, tTemp, idTemp, true, lights[lightIt]))
				{
					if (idTemp == lights[lightIt])
					{

						rad = rad + Li.mult(obj.c) * brdf(toLight.d, n, -1.0 * r.d) * (n.dot(toLight.d)) * blendFunction(Vec(toLight.o - light.p).norm(), light.depth);
					}
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

float blendFunction(float distance, int depth)
{
	if (depth == blendingRadius.size() - 1 && distance <= blendingRadius[depth].x)
	{
		return 1.0;
	}
	else if (depth == 0 && distance >= blendingRadius[depth].x)
	{
		return 1.0;
	}

	if (distance > blendingRadius[depth].x)
	{
		return clamp((distance - blendingRadius[depth - 1].x) / (blendingRadius[depth].x - blendingRadius[depth - 1].x), 0.0, 1.0);
	}
	else
	{
		return clamp((blendingRadius[depth + 1].x - distance) / (blendingRadius[depth + 1].x - blendingRadius[depth].x), 0.0, 1.0);
	}
	return 0.0;
}

void computeRayTracedImage()
{
	cellColorAndIntensityForRayTracer.clear();
	cellPositionsForRayTracer.clear();
	cellPositions.clear();
	cellDepth.clear();
	cellDepthForRayTracer.clear();
	cellColorAndIntensity.clear();
	UpdateCellVectors(octreeRoot, true, true);

	//int w = 1024, h = 768, samps = 1; // # samples
	int w = 512, h = 384, samps = 1; // # samples

	Ray cam(Vec(-0.12, -0.0, 4.0), Vec(0, 0, -1).normalize());   // camera center and direction
	Vec cx = Vec(w * .5 / h), cy = (cx.cross(cam.d)).normalize() * .5, *pixelsColor = new Vec[w * h];

	// setup scene:
	spheres.push_back(new Sphere(1e5, Vec(-1e5 + CUBE_SIZE, 0.0, 0.0), Vec(0, 0, 0), Vec(.75, .75, .75), DIFFUSE, -1));   //Left
	spheres.push_back(new Sphere(1e5, Vec(1e5 - CUBE_SIZE, 0.0, 0.0), Vec(0, 0, 0), Vec(.75, .75, .75), DIFFUSE, -1)); //Rght
	spheres.push_back(new Sphere(1e5, Vec(0, 0, 1e5 - CUBE_SIZE), Vec(0, 0, 0), Vec(.75, .75, .75), DIFFUSE, -1));         //Back
	//spheres.push_back(new Sphere(1e5, Vec(0, 0, -1e5 + CUBE_SIZE), Vec(0, 0, 0), Vec(0, 0, 0), DIFFUSE, -1));        //Front
	spheres.push_back(new Sphere(1e5, Vec(0, 1e5 - CUBE_SIZE, 0), Vec(0, 0, 0), Vec(.75, .75, .75), DIFFUSE, -1));         //Bottom
	spheres.push_back(new Sphere(1e5, Vec(0, -1e5 + CUBE_SIZE, 0), Vec(0, 0, 0), Vec(.75, .75, .75), DIFFUSE, -1));         //Top
	/*spheres.push_back(Sphere(16.5, Vec(27, 16.5, 47), Vec(0, 0, 0), Vec(1, 1, 1) * .999, MIRROR, -1));         //Mirr
	spheres.push_back(Sphere(16.5, Vec(73, 16.5, 78), Vec(0, 0, 0), Vec(1, 1, 1) * .999, GLASS, -1));         //Change to Glass*/

	int spheresSize = spheres.size();
	if (spheres.size() > NUMBER_OF_SPHERE + 5)
	{
		for (int i = NUMBER_OF_SPHERE + 5; i < spheresSize; i++)
		{
			delete spheres[i];
		}
	}
	spheres.erase(spheres.begin() + NUMBER_OF_SPHERE + 5, spheres.end());
	lights.clear();

	for (int i = 0; i < lightSources.size(); i++)
	{
		spheres.push_back(new Sphere(CUBE_SIZE / pow(2, lightSources[i].depth.x),
			Vec(lightSources[i].position.x, lightSources[i].position.y, lightSources[i].position.z),
			1.0 / cellPositionsForRayTracer.size() * Vec(lightSources[i].color_and_intensity.x, lightSources[i].color_and_intensity.y, lightSources[i].color_and_intensity.z),
			Vec(lightSources[i].color_and_intensity.x, lightSources[i].color_and_intensity.y, lightSources[i].color_and_intensity.z), EMMISSIVE, lightSources[i].depth.x));
		lights.push_back(NUMBER_OF_SPHERE + 5 + i);
	}

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
				r = r + radiance(Ray(cam.o + d, d.normalize()), 0) * (1. / samps);
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
		{
			fprintf(f, "%d %d %d ", (int)(pixelsColor[i].x * 255), (int)(pixelsColor[i].y * 255), (int)(pixelsColor[i].z * 255));
		}
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
		std::ofstream ofs(SIMULATION_FILE_NAME, std::ios::out | std::ios::trunc); // clear contents
		ofs.close();
	}

	in = std::ifstream(SIMULATION_FILE_NAME, std::ios_base::binary);

	initialTime = glutGet(GLUT_ELAPSED_TIME);
	zeroTimeOfSimulation = initialTime;

	glutDisplayFunc(render);
	glutMainLoop();

	clear();

	return 0;
}