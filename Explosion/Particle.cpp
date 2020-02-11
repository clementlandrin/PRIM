#include "Particle.hpp"

class Cell;

void Particle::AddCell(Cell* cell)
{
    m_Cells.push_back(cell);
}

void Particle::SetPosition(glm::vec3 new_position) { m_Position = new_position; }
void Particle::SetGradient(glm::vec3 new_gradient) { m_Gradient = new_gradient; }
void Particle::SetLaplacian(glm::vec3 new_laplacian) { m_Laplacian = new_laplacian; }
void Particle::SetVGradV(glm::vec3 new_vgradv) { m_VGradV = new_vgradv; }
void Particle::SetPressureGradient(glm::vec3 new_pressure_gradient) { m_PressureGradient = new_pressure_gradient / m_Energy; }
void Particle::SetDeepestCell(Cell* cell) { m_DeepestCell = cell; }

Particle::Particle(glm::vec3 position, float energy) { m_Position = position; m_Energy = energy; }

Cell* Particle::GetDeepestCell() { return m_DeepestCell; }
const glm::vec3 Particle::GetPosition() { return m_Position; }
const glm::vec3 Particle::GetSpeed() { return m_Speed; }
const float Particle::GetEnergy() { return m_Energy; }
const std::vector<Cell*> Particle::GetCells() { return m_Cells; }
const glm::vec3 Particle::GetGradient() { return m_Gradient; }
const glm::vec3 Particle::GetLaplacian() { return m_Laplacian; }
const glm::vec3 Particle::GetVGradV() { return m_VGradV; }
const glm::vec3 Particle::GetPressureGradient() { return m_PressureGradient; }

void Particle::SetSpeed(glm::vec3 speed) { m_Speed = speed; }
void Particle::SetEnergy(float energy) { m_Energy = energy; }