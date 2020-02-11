#include "RegularGrid.h"

#include "Cell.hpp"
#include "Fluid.hpp"

RegularGrid::RegularGrid(int resolution, glm::vec3 size)
{
	SetResolution(resolution);
	SetSize(size);
	
	m_Cells.resize(resolution);
	for (int i = 0; i < resolution; i++)
	{
		m_Cells[i].resize(resolution);
		for (int j = 0; j < resolution; j++)
		{
			m_Cells[i][j].resize(resolution);
			for (int k = 0; k < resolution; k++)
			{
				glm::vec3 position = glm::vec3(size[0] / resolution * i - size[0] / 2.0, size[1] / resolution * j - size[1] / 2.0, size[2] / resolution * k - size[2] / 2.0);
				int indexOfCell[3] = { i, j, k };
				Cell* cell = new Cell(nullptr, position, size[0] / resolution, size[1] / resolution, size[2] / resolution, indexOfCell, this);
				m_Cells[i][j][k] = cell;
			}
		}
	}
}

RegularGrid::~RegularGrid()
{
	for (int i = 0; i < m_Cells.size(); i++)
	{
		for (int j = 0; j < m_Cells[i].size(); j++)
		{
			for (int k = 0; k < m_Cells[i][j].size(); k++)
			{
				delete m_Cells[i][j][k];
			}
		}
	}
}

glm::vec3 RegularGrid::GetSize() { return m_Size; }
int RegularGrid::GetResolution() { return m_Resolution; }
std::vector<std::vector<std::vector<Cell*>>> RegularGrid::GetCells() { return m_Cells; }

void RegularGrid::SetSize(glm::vec3 size) { m_Size = size; }
void RegularGrid::SetResolution(int resolution) { m_Resolution = resolution; }

glm::vec3 RegularGrid::UpdateSpeedOfCells()
{
	glm::vec3 maxSpeed = glm::vec3(0.0);
	glm::vec3 currentSpeed;
	for (int i = 0; i < m_Resolution; i++)
	{
		for (int j = 0; j < m_Resolution; j++)
		{
			for (int k = 0; k < m_Resolution; k++)
			{
				currentSpeed = m_Cells[i][j][k]->UpdateSpeed();
				if (abs(currentSpeed.x) > maxSpeed.x)
				{
					maxSpeed.x = abs(currentSpeed.x);
				}
				if (abs(currentSpeed.y) > maxSpeed.y)
				{
					maxSpeed.y = abs(currentSpeed.y);
				}
				if (abs(currentSpeed.z) > maxSpeed.z)
				{
					maxSpeed.z = abs(currentSpeed.z);
				}
			}
		}
	}

	return maxSpeed;
}

void RegularGrid::UpdateGradientAndVGradVOfCells()
{
	for (int i = 0; i < m_Resolution; i++)
	{
		for (int j = 0; j < m_Resolution; j++)
		{
			for (int k = 0; k < m_Resolution; k++)
			{
				m_Cells[i][j][k]->ComputeGradientAndVGradV();
			}
		}
	}
}

void RegularGrid::UpdateLaplacianOfCells()
{
	for (int i = 0; i < m_Resolution; i++)
	{
		for (int j = 0; j < m_Resolution; j++)
		{
			for (int k = 0; k < m_Resolution; k++)
			{
				m_Cells[i][j][k]->ComputeLaplacian();
			}
		}
	}
}

void RegularGrid::UpdateSpeedVariationNavierStokes(Fluid* fluid)
{
	for (int i = 0; i < m_Resolution; i++)
	{
		for (int j = 0; j < m_Resolution; j++)
		{
			for (int k = 0; k < m_Resolution; k++)
			{
				m_Cells[i][j][k]->UpdateSpeedVariation(fluid);
			}
		}
	}
	
}
void RegularGrid::PushNavierStokesParametersToParticles()
{
	for (int i = 0; i < m_Resolution; i++)
	{
		for (int j = 0; j < m_Resolution; j++)
		{
			for (int k = 0; k < m_Resolution; k++)
			{
				m_Cells[i][j][k]->PushNavierStokesParameters();
			}
		}
	}
}

void RegularGrid::ResetNavierStokesParametersOfCells()
{
	for (int i = 0; i < m_Resolution; i++)
	{
		for (int j = 0; j < m_Resolution; j++)
		{
			for (int k = 0; k < m_Resolution; k++)
			{
				m_Cells[i][j][k]->ResetNavierStokesParameters();
			}
		}
	}
}

void RegularGrid::ResizeGrid(glm::vec3 size)
{
	SetSize(size);

	for (int i = 0; i < m_Resolution; i++)
	{
		for (int j = 0; j < m_Resolution; j++)
		{
			for (int k = 0; k < m_Resolution; k++)
			{
				glm::vec3 position = glm::vec3(size[0] / m_Resolution * i - size[0] / 2.0, size[1] / m_Resolution * j - size[1] / 2.0, size[2] / m_Resolution * k - size[2] / 2.0);
				m_Cells[i][j][k]->SetPosition(position);
				m_Cells[i][j][k]->SetCellWidth(size[0] / m_Resolution);
				m_Cells[i][j][k]->SetCellHeight(size[1] / m_Resolution);
				m_Cells[i][j][k]->SetCellDepth(size[2] / m_Resolution);
			}
		}
	}
}