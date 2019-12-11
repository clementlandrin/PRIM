#include "RegularGrid.h"

#include "Cell.hpp"

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
				Cell* cell = new Cell(nullptr, position, size[0]/resolution, size[1]/resolution, size[2]/resolution);
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

void RegularGrid::SetSize(glm::vec3 size) { m_Size = size; }
void RegularGrid::SetResolution(int resolution) { m_Resolution = resolution; }