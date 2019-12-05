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

void Fluid::GenerateParticlesUniformly(int particleNumber, glm::vec3 origin, float width, float height, float depth)
{
  srand (static_cast <unsigned> (time(0)));
  glm::vec3 position;
  glm::vec3 initialSpeed;
  for (int i = 0; i < particleNumber; i++)
  {
    float x =  (origin.x - width*0.5 + rand() / (static_cast <float> (RAND_MAX/width)));
    float y =  (origin.y - height*0.5 + rand() / (static_cast <float> (RAND_MAX/height)));
    float z =  (origin.z - depth*0.5 + rand() / (static_cast <float> (RAND_MAX/depth)));
    position = glm::vec3( x, y, z);
	initialSpeed = glm::normalize(position);

    Particle* particle = new Particle(position, 1.0f/particleNumber);
    particle->SetSpeed(initialSpeed);

    m_Particles.push_back(particle);
  }
}

void Fluid::UpdateParticlePositions(float dt)
{
  for (int i = 0; i < m_Particles.size(); i++)
  {
    m_Particles[i]->SetPosition(m_Particles[i]->GetPosition() + dt*m_Particles[i]->GetSpeed());
  }
}

const float Fluid::GetViscosity() { return m_Viscosity; }
const float Fluid::GetDensity() { return m_Density; }
const std::vector<Particle*> Fluid::GetParticles() { return m_Particles; }

void Fluid::SetViscosity(float viscosity) { m_Viscosity = viscosity; }
void Fluid::SetDensity(float density) { m_Density = density; }