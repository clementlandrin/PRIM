// Fluid.hpp
#ifndef FLUID_HPP // include guard
#define FLUID_HPP

#include <vector>
#include "External/glm/glm/glm.hpp"


#include "Particle.hpp"

class OctreeNode;

class Fluid
{
public:
	Fluid(float viscosity, float density) {
		m_Viscosity = viscosity;
		m_Density = density;
	};

  ~Fluid();

  void GenerateParticlesUniformly(int particleNumber, glm::vec3 origin, float width, float height, float depth);
  void UpdateParticlePositions(float dt, float cubeSize);
 
  static bool PositionIsInCube(glm::vec3 position, float cubeSize);

  const float GetViscosity();
  const float GetDensity();
  const std::vector<Particle*> GetParticles();

  void SetViscosity(float viscosity);
  void SetDensity(float density);

private:
  float m_Viscosity;
  float m_Density;
  std::vector<Particle*> m_Particles;
};

#endif /* Fluid_HPP */
