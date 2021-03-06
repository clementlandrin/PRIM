#include "Cell.hpp"

#include "Particle.hpp"
#include "Fluid.hpp"

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

glm::vec3 Cell::UpdateSpeed()
{
	m_Speed = glm::vec3(0.0);
	for (int i = 0; i < m_Particles.size(); i++)
	{
		m_Speed += m_Particles[i]->GetSpeed();
	}
	if (m_Particles.size() != 0)
	{
		m_Speed /= m_Particles.size();
	}
	return m_Speed;
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

const glm::vec3 Cell::GetSpeed()
{
	return m_Speed;
}

const glm::vec3 Cell::GetSpeedVariation()
{
	return m_SpeedVariation;
}

const glm::vec3 Cell::GetSpeedVariationInterpolated(Particle* particle)
{
	Cell* front_cell = GetOnFrontCell();
	Cell* back_cell = GetOnBackCell();
	Cell* top_cell = GetOnTopCell();
	Cell* bottom_cell = GetOnBottomCell();
	Cell* left_cell = GetOnLeftCell();
	Cell* right_cell = GetOnRightCell();

	glm::vec3 speed_front_cell = front_cell ? front_cell->GetSpeedVariation() : glm::vec3(0.0);
	glm::vec3 speed_back_cell = back_cell ? back_cell->GetSpeedVariation() : glm::vec3(0.0);
	glm::vec3 speed_top_cell = top_cell ? top_cell->GetSpeedVariation() : glm::vec3(0.0);
	glm::vec3 speed_bottom_cell = bottom_cell ? bottom_cell->GetSpeedVariation() : glm::vec3(0.0);
	glm::vec3 speed_left_cell = left_cell ? left_cell->GetSpeedVariation() : glm::vec3(0.0);
	glm::vec3 speed_right_cell = right_cell ? right_cell->GetSpeedVariation() : glm::vec3(0.0);

	glm::vec3 particle_position = particle->GetPosition();

	glm::vec3 interpolated_speed = glm::vec3(0.0f);

	float onX = (particle_position.x - m_Position.x) / m_RegularGrid->GetSize().x;
	float onY = (particle_position.y - m_Position.y) / m_RegularGrid->GetSize().y;
	float onZ = (particle_position.z - m_Position.z) / m_RegularGrid->GetSize().z;

	if (onX >= 0.0f)
	{
		interpolated_speed.x = onX * GetSpeedVariation().x + (1 - onX) * speed_right_cell.x;
	}
	else
	{
		interpolated_speed.x = onX * GetSpeedVariation().x + (1 - abs(onX)) * speed_left_cell.x;
	}

	if (onY >= 0.0f)
	{
		interpolated_speed.y = onY * GetSpeedVariation().y + (1 - onY) * speed_right_cell.y;
	}
	else
	{
		interpolated_speed.y = onY * GetSpeedVariation().y + (1 - abs(onY)) * speed_left_cell.y;
	}

	if (onZ >= 0.0f)
	{
		interpolated_speed.z = onZ * GetSpeedVariation().z + (1 - onZ) * speed_right_cell.z;
	}
	else
	{
		interpolated_speed.z = onZ * GetSpeedVariation().z + (1 - abs(onZ)) * speed_left_cell.z;
	}

	return interpolated_speed;
}

const glm::vec3 Cell::GetGradient()
{
	return m_Gradient;
}

const glm::vec3 Cell::ComputeCenter(bool shouldMoveCenterToParticleBarycenter)
{
	if (m_Particles.size() == 0 || !shouldMoveCenterToParticleBarycenter)
	{
		return glm::vec3(m_Position[0] + m_CellWidth / 2.0, m_Position[1] + m_CellHeight / 2.0, m_Position[2] + m_CellDepth / 2.0);
	}
	glm::vec3 center = glm::vec3(0.0f);
	for (int i = 0; i < m_Particles.size(); i++)
	{
		center += m_Particles[i]->GetPosition();
	}
	return center / (float) m_Particles.size();
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
	if (m_IndexInRegularGrid[0] < m_RegularGrid->GetResolution() - 1)
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
	if (m_IndexInRegularGrid[2] > 1)
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
	if (m_IndexInRegularGrid[2] < m_RegularGrid->GetResolution() - 1)
	{
		return m_RegularGrid->GetCells()[m_IndexInRegularGrid[0]][m_IndexInRegularGrid[1]][m_IndexInRegularGrid[2] + 1];
	}
	else
	{
		return nullptr;
	}
}

void Cell::ComputeGradientAndVGradV()
{
	if (m_Particles.size() == 0)
	{
		m_Gradient = glm::vec3(0.0f);
		m_VGradv = glm::vec3(0.0f);
		m_PressureGradient = glm::vec3(0.0f);
		return;
	}

	Cell* front_cell = GetOnFrontCell();
	Cell* back_cell = GetOnBackCell();
	Cell* top_cell = GetOnTopCell();
	Cell* bottom_cell = GetOnBottomCell();
	Cell* left_cell = GetOnLeftCell();
	Cell* right_cell = GetOnRightCell();

	glm::vec3 speed_front_cell = glm::vec3(0.0);
	glm::vec3 speed_back_cell = glm::vec3(0.0);
	glm::vec3 speed_top_cell = glm::vec3(0.0);
	glm::vec3 speed_bottom_cell = glm::vec3(0.0);
	glm::vec3 speed_left_cell = glm::vec3(0.0);
	glm::vec3 speed_right_cell = glm::vec3(0.0);

	int density_front_cell = 0;
	int density_back_cell = 0;
	int density_top_cell = 0;
	int density_bottom_cell = 0;
	int density_left_cell = 0;
	int density_right_cell = 0;

	if (front_cell)
	{
		density_front_cell = front_cell->GetParticles().size();
		speed_front_cell = front_cell->GetSpeed();
	}
	if (back_cell)
	{
		density_back_cell = back_cell->GetParticles().size();
		speed_back_cell = back_cell->GetSpeed();
	}
	if (top_cell)
	{
		density_top_cell = top_cell->GetParticles().size();
		speed_top_cell = top_cell->GetSpeed();
	}
	if (bottom_cell)
	{
		density_bottom_cell = bottom_cell->GetParticles().size();
		speed_bottom_cell = bottom_cell->GetSpeed();
	}
	if (left_cell)
	{
		density_left_cell = left_cell->GetParticles().size();
		speed_left_cell = left_cell->GetSpeed();
	}
	if (right_cell)
	{
		density_right_cell = right_cell->GetParticles().size();
		speed_right_cell = right_cell->GetSpeed();
	}

	glm::vec3 onX = (speed_right_cell - speed_left_cell)* 0.5f;// *(float)m_RegularGrid->GetSize().x;
	glm::vec3 onY = (speed_top_cell - speed_bottom_cell)* 0.5f;// *(float)m_RegularGrid->GetSize().y;
	glm::vec3 onZ = (speed_back_cell - speed_front_cell)* 0.5f;// *(float)m_RegularGrid->GetSize().z;

	float densityOnX = (density_right_cell - density_left_cell) * 0.5f;// *(float)m_RegularGrid->GetSize().x;
	float densityOnY = (density_top_cell - density_bottom_cell)* 0.5f;// *(float)m_RegularGrid->GetSize().y;
	float densityOnZ = (density_back_cell - density_front_cell)* 0.5f;// *(float)m_RegularGrid->GetSize().z;

	m_Gradient = glm::vec3(onX.x, onY.y, onZ.z);

	m_PressureGradient = glm::vec3(densityOnX, densityOnY, densityOnZ);

	m_VGradv = glm::vec3(
		m_Speed.x * onX.x + m_Speed.y * onY.x + m_Speed.z * onZ.x,
		m_Speed.x * onX.y + m_Speed.y * onY.y + m_Speed.z * onZ.y,
		m_Speed.x * onX.z + m_Speed.y * onY.z + m_Speed.z * onZ.z);
}

void Cell::ComputeLaplacian()
{
	if (m_Particles.size() == 0)
	{
		m_Laplacian = glm::vec3(0.0f);
		return;
	}

	Cell* front_cell = GetOnFrontCell();
	Cell* back_cell = GetOnBackCell();
	Cell* top_cell = GetOnTopCell();
	Cell* bottom_cell = GetOnBottomCell();
	Cell* left_cell = GetOnLeftCell();
	Cell* right_cell = GetOnRightCell();

	glm::vec3 gradient_front_cell = glm::vec3(0.0);
	glm::vec3 gradient_back_cell = glm::vec3(0.0);
	glm::vec3 gradient_top_cell = glm::vec3(0.0);
	glm::vec3 gradient_bottom_cell = glm::vec3(0.0);
	glm::vec3 gradient_left_cell = glm::vec3(0.0);
	glm::vec3 gradient_right_cell = glm::vec3(0.0);

	if (front_cell)
	{
		gradient_front_cell = front_cell->GetGradient();
	}
	if (back_cell)
	{
		gradient_back_cell = back_cell->GetGradient();
	}
	if (top_cell)
	{
		gradient_top_cell = top_cell->GetGradient();
	}
	if (bottom_cell)
	{
		gradient_bottom_cell = bottom_cell->GetGradient();
	}
	if (left_cell)
	{
		gradient_left_cell = left_cell->GetGradient();
	}
	if (right_cell)
	{
		gradient_right_cell = right_cell->GetGradient();
	}

	glm::vec3 onX = (gradient_right_cell - gradient_left_cell) * 0.5f;// *(float)m_RegularGrid->GetSize().x;
	glm::vec3 onY = (gradient_top_cell - gradient_bottom_cell) * 0.5f;// *(float)m_RegularGrid->GetSize().y;
	glm::vec3 onZ = (gradient_back_cell - gradient_front_cell) * 0.5f;// *(float)m_RegularGrid->GetSize().z;

	m_Laplacian = glm::vec3(onX.x + onY.x + onZ.x, onX.y + onY.y + onZ.y, onX.z + onY.z + onZ.z);
}

void Cell::PushNavierStokesParameters()
{
	for (int i = 0; i < m_Particles.size(); i++)
	{
		m_Particles[i]->SetGradient(m_Gradient);
		m_Particles[i]->SetLaplacian(m_Laplacian);
		m_Particles[i]->SetVGradV(m_VGradv);
		m_Particles[i]->SetPressureGradient(m_PressureGradient);
	}
}

void Cell::ResetNavierStokesParameters()
{
	m_Gradient = glm::vec3(0.0);
	m_Laplacian = glm::vec3(0.0);
	m_VGradv = glm::vec3(0.0);
	m_PressureGradient = glm::vec3(0.0);
}

void Cell::UpdateSpeedVariation(Fluid* fluid)
{
	m_SpeedVariation = (0.0f * glm::vec3(0.0f, -9.81f, 0.0f) + 0.0f * fluid->GetViscosity() * GetLaplacian() - 100.0f / fluid->GetParticles().size() * GetPressureGradient() - 0.0f * GetVGradV()) / fluid->GetDensity(); // VGradV a 0.5f
}

const glm::vec3 Cell::GetLaplacian() { return m_Laplacian; }
const glm::vec3 Cell::GetVGradV() { return m_VGradv; }
const glm::vec3 Cell::GetPressureGradient() { return m_PressureGradient; }