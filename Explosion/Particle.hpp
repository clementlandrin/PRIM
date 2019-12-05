// Particle.hpp
#ifndef PARTICLE_HPP // include guard
#define PARTICLE_HPP

#include <vector>

#include "../Explosion/External/glm/glm/glm.hpp"

class Cell;

class Particle
{
public:
  Particle(glm::vec3 position, float energy);

  const glm::vec3 GetPosition();
  const glm::vec3 GetSpeed();
  const float GetEnergy();
  const std::vector<Cell*> GetCells();

  void SetPosition(glm::vec3 position);
  void SetSpeed(glm::vec3 speed);
  void SetEnergy(float energy);
  void AddCell(Cell* cell);

private:
  glm::vec3 m_Position;
  glm::vec3 m_Speed;
  float m_Energy;
  std::vector<Cell*> m_Cells;
};

#endif /* PARTICLE_HPP */
