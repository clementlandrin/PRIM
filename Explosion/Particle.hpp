// Particle.hpp
#ifndef PARTICLE_HPP // include guard
#define PARTICLE_HPP

#include <vector>

#include "External/glm/glm/glm.hpp"

class Cell;

class Particle
{
public:
  Particle(glm::vec3 position, float energy);

  const glm::vec3 GetPosition();
  const glm::vec3 GetSpeed();
  const float GetEnergy();
  const std::vector<Cell*> GetCells();
  const glm::vec3 GetGradient();
  const glm::vec3 GetLaplacian();
  const glm::vec3 GetVGradV();
  const glm::vec3 GetPressureGradient();

  void SetPosition(glm::vec3 position);
  void SetSpeed(glm::vec3 speed);
  void SetGradient(glm::vec3 new_gradient);
  void SetLaplacian(glm::vec3 new_laplacian);
  void SetVGradV(glm::vec3 new_vgradv);
  void SetPressureGradient(glm::vec3 new_pressure_gradient);
  void SetEnergy(float energy);
  void AddCell(Cell* cell);
private:
  glm::vec3 m_Position;
  glm::vec3 m_Speed;
  float m_Energy;
  std::vector<Cell*> m_Cells;
  glm::vec3 m_Gradient = glm::vec3(0.0);
  glm::vec3 m_Laplacian = glm::vec3(0.0);
  glm::vec3 m_VGradV = glm::vec3(0.0);
  glm::vec3 m_PressureGradient = glm::vec3(0.0);
};

#endif /* PARTICLE_HPP */
