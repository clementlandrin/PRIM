#include "Particle.hpp"

class Cell;

void Particle::AddCell(Cell* cell)
{
    m_Cells.push_back(cell);
}

void Particle::SetPosition(glm::vec3 new_position)
{
    m_Position = new_position;
}

Particle::Particle(glm::vec3 position, float energy) { m_Position = position; m_Energy = energy; }

const glm::vec3 Particle::GetPosition() { return m_Position; }
const glm::vec3 Particle::GetSpeed() { return m_Speed; }
const float Particle::GetEnergy() { return m_Energy; }
const std::vector<Cell*> Particle::GetCells() { return m_Cells; }

void Particle::SetSpeed(glm::vec3 speed) { m_Speed = speed; }
void Particle::SetEnergy(float energy) { m_Energy = energy; }