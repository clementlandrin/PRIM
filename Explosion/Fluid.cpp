#include "Fluid.hpp"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include "OctreeNode.hpp"

Fluid::~Fluid()
{
  for (Particle* particle : m_Particles)
  {
    delete particle;
  }
  m_Particles.clear();
}

bool Fluid::PositionIsInCube(glm::vec3 position, float cubeSize)
{
	if (abs(position.x) > cubeSize || abs(position.y) > cubeSize || abs(position.z) > cubeSize)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void Fluid::GenerateParticlesUniformly(int particleNumber, glm::vec3 origin, float width, float height, float depth, float speedFactor)
{
  srand (static_cast <unsigned> (time(0)));
  glm::vec3 position;
  glm::vec3 initialSpeed;
  for (int i = 0; i < particleNumber; i++)
  {
    float x =  2.0 * rand() / (static_cast <float> (RAND_MAX)) - 1.0;
    float y =  2.0 * rand() / (static_cast <float> (RAND_MAX)) - 1.0;
    float z =  2.0 * rand() / (static_cast <float> (RAND_MAX)) - 1.0;
	float length = rand() / (static_cast <float>(RAND_MAX));
	position = glm::vec3(x, y, z);
	position = glm::normalize(position);
	position = position * length;
	initialSpeed = glm::normalize(position);
	initialSpeed.y = 5.0f;
	initialSpeed = initialSpeed * speedFactor;

	position = position * glm::vec3(width*0.5, height*0.5, depth*0.5);
	position = position + origin;

    Particle* particle = new Particle(position, 1.0f/particleNumber);
    particle->SetSpeed(initialSpeed);

    m_Particles.push_back(particle);
  }
}

void Fluid::AddParticle(glm::vec3 position, float energy)
{
	Particle* particle = new Particle(position, energy);
	m_Particles.push_back(particle);
}

void Fluid::ClearParticles()
{
	for (int i = 0; i < m_Particles.size(); i++)
	{
		delete m_Particles[i];
	}
	m_Particles.clear();
}

void Fluid::UpdateParticlePositions(float dt, float cubeSize)
{
  for (int i = 0; i < m_Particles.size(); i++)
  {
	m_Particles[i]->SetSpeed(m_Particles[i]->GetSpeed() + dt * SpeedVariationByNavierStokes(m_Particles[i]));
	glm::vec3 newPosition = m_Particles[i]->GetPosition() + dt * m_Particles[i]->GetSpeed(); //glm::vec3(rand() / (static_cast <float> (RAND_MAX)), rand() / (static_cast <float> (RAND_MAX)), rand() / (static_cast <float> (RAND_MAX)));
	if (PositionIsInCube(newPosition, cubeSize))
	{
	  m_Particles[i]->SetPosition(newPosition);
	}
	else
	{
	  delete m_Particles[i];
	  m_Particles.erase(m_Particles.begin() + i);
	}
  }
}
glm::vec3 Fluid::SpeedVariationByNavierStokes(Particle* particle)
{
	return (m_Viscosity * particle->GetLaplacian() - particle->GetVGradV())/m_Viscosity;
}

const float Fluid::GetViscosity() { return m_Viscosity; }
const float Fluid::GetDensity() { return m_Density; }
const std::vector<Particle*> Fluid::GetParticles() { return m_Particles; }

void Fluid::SetViscosity(float viscosity) { m_Viscosity = viscosity; }
void Fluid::SetDensity(float density) { m_Density = density; }