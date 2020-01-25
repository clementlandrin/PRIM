#include "RegularGrid.h"

#include "Cell.hpp"

RegularGrid::RegularGrid(int resolution[3], glm::vec3 size)
{
	SetResolution(resolution);
	SetSize(size);
	
	m_Cells.resize(resolution[0]);
	for (int i = 0; i < resolution[0]; i++)
	{
		m_Cells[i].resize(resolution[1]);
		for (int j = 0; j < resolution[1]; j++)
		{
			m_Cells[i][j].resize(resolution[2]);
			for (int k = 0; k < resolution[2]; k++)
			{
				glm::vec3 position = glm::vec3(size[0] / resolution[0] * i - size[0] / 2.0, size[1] / resolution[1] * j - size[1] / 2.0, size[2] / resolution[2] * k - size[2] / 2.0);
				int indexOfCell[3] = { i, j, k };
				Cell* cell = new Cell(nullptr, position, size[0] / resolution[0], size[1] / resolution[1], size[2] / resolution[2], indexOfCell, this);
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
int* RegularGrid::GetResolution() { return m_Resolution; }
std::vector<std::vector<std::vector<Cell*>>> RegularGrid::GetCells() { return m_Cells; }

void RegularGrid::SetSize(glm::vec3 size) { m_Size = size; }
void RegularGrid::SetResolution(int resolution[3]) { m_Resolution[0] = resolution[0]; m_Resolution[1] = resolution[1]; m_Resolution[2] = resolution[2]; }