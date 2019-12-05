#include "Cell.hpp"

#include "Particle.hpp"

Cell::Cell()
{

}

Cell::Cell(Cell* parent, glm::vec3 position, float width, float height, float depth)
{
  m_Parent = parent;
  m_Position = position;
  m_CellWidth = width;
  m_CellHeight = height;
  m_CellDepth = depth;
}

void Cell::AddParticle(Particle* particle)
{
  m_Particles.push_back(particle);
}

void Cell::EraseParticleAtIndex(int index)
{
  if(index < m_Particles.size())
  {
    m_Particles.erase(m_Particles.begin()+index);
  }
}

void Cell::ComputeEnergy()
{
  m_Energy = 0.0;

  for (int i = 0; i < m_Particles.size(); i++)
  {
    m_Energy += m_Particles[i]->GetEnergy();
  }
}

std::vector<Particle*> Cell::GetParticles()
{
  return m_Particles;
}

const glm::vec3 Cell::GetPosition()
{
  return m_Position;
}

const float Cell::GetCellWidth()
{
  return m_CellWidth;
}

const float Cell::GetCellHeight()
{
  return m_CellHeight;
}

const float Cell::GetCellDepth()
{
  return m_CellDepth;
}

const float Cell::GetEnergy()
{
  return m_Energy;
}

const Cell* Cell::GetParent()
{
  return m_Parent;
}

void Cell::SetParticles(std::vector<Particle*> particles)
{
  m_Particles = particles;
}

void Cell::SetPosition(glm::vec3 position)
{
  m_Position = position;
}

void Cell::SetCellWidth(float width)
{
  m_CellWidth = width;
}

void Cell::SetCellHeight(float height)
{
  m_CellHeight = height;
}

void Cell::SetCellDepth(float depth)
{
  m_CellDepth = depth;
}

void Cell::SetEnergy(float energy)
{
  m_Energy = energy;
}

void Cell::SetParent(Cell* parent)
{
  m_Parent = parent;
}