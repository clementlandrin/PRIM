#include "Cell.hpp"

#include "Particle.hpp"
#include "RegularGrid.h"
#include <iostream>

Cell::Cell(Cell* parent, glm::vec3 position, float width, float height, float depth, int indexInRegularGrid[3], RegularGrid* regularGrid)
{
  m_Parent = parent;

  m_Position = position;
  m_CellWidth = width;
  m_CellHeight = height;
  m_CellDepth = depth;

  m_IndexInRegularGrid[0] = indexInRegularGrid[0];
  m_IndexInRegularGrid[1] = indexInRegularGrid[1];
  m_IndexInRegularGrid[2] = indexInRegularGrid[2];
  m_RegularGrid = regularGrid;
}

Cell::~Cell()
{
	if (!m_Parent)
	{
		std::cout << "toto 1" << std::endl;
	}
}

void Cell::AddParticle(Particle* particle)
{
  m_Particles.push_back(particle);
}

void Cell::ClearParticles()
{
  m_Particles.clear();
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

void Cell::UpdateSpeed()
{
	m_Speed = glm::vec3(0.0);
	for (int i = 0; i < m_Particles.size(); i++)
	{
		m_Speed += m_Particles[i]->GetSpeed();
	}
	if (m_Particles.size() != 0)
	{
		m_Speed /= m_Particles.size();
		for (int i = 0; i < m_Particles.size(); i++)
		{
			m_Particles[i]->SetSpeed(m_Speed);
		}
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

const glm::vec3 Cell::ComputeCenter()
{
	return glm::vec3(m_Position[0] + m_CellWidth / 2.0, m_Position[1] + m_CellHeight / 2.0, m_Position[2] + m_CellDepth / 2.0);
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

Cell * Cell::GetOnTopCell()
{
	if (m_IndexInRegularGrid[1] < m_RegularGrid->GetResolution() - 1)
	{
		return m_RegularGrid->GetCells()[m_IndexInRegularGrid[0]][m_IndexInRegularGrid[1] + 1][m_IndexInRegularGrid[2]];
	}
	else
	{
		return nullptr;
	}
}

Cell * Cell::GetOnBottomCell()
{
	if (m_IndexInRegularGrid[1] > 1)
	{
		return m_RegularGrid->GetCells()[m_IndexInRegularGrid[0]][m_IndexInRegularGrid[1] - 1][m_IndexInRegularGrid[2]];
	}
	else
	{
		return nullptr;
	}
}

Cell * Cell::GetOnLeftCell()
{
	if (m_IndexInRegularGrid[0] > 1)
	{
		return m_RegularGrid->GetCells()[m_IndexInRegularGrid[0] - 1][m_IndexInRegularGrid[1]][m_IndexInRegularGrid[2]];
	}
	else
	{
		return nullptr;
	}
}

Cell * Cell::GetOnRightCell()
{
	if (m_IndexInRegularGrid[1] < m_RegularGrid->GetResolution() - 1)
	{
		return m_RegularGrid->GetCells()[m_IndexInRegularGrid[0] + 1][m_IndexInRegularGrid[1]][m_IndexInRegularGrid[2]];
	}
	else
	{
		return nullptr;
	}
}

Cell * Cell::GetOnFrontCell()
{
	if (m_IndexInRegularGrid[2] < 1)
	{
		return m_RegularGrid->GetCells()[m_IndexInRegularGrid[0]][m_IndexInRegularGrid[1]][m_IndexInRegularGrid[2] - 1];
	}
	else
	{
		return nullptr;
	}
}

Cell * Cell::GetOnBackCell()
{
	if (m_IndexInRegularGrid[1] < m_RegularGrid->GetResolution() - 1)
	{
		return m_RegularGrid->GetCells()[m_IndexInRegularGrid[0]][m_IndexInRegularGrid[1]][m_IndexInRegularGrid[2] + 1];
	}
	else
	{
		return nullptr;
	}
}
