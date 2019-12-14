#pragma once

#include <vector>
#include "External/glm/glm/glm.hpp"

class Cell;

class RegularGrid
{
public:
	RegularGrid(int resolution, glm::vec3 size);
	~RegularGrid();

	glm::vec3 GetSize();
	std::vector<std::vector<std::vector<Cell*>>> GetCells();
	int GetResolution();

	void SetSize(glm::vec3 size);
	void SetResolution(int resolution);

	void UpdateSpeedOfCells();
private:
	int m_Resolution;
	glm::vec3 m_Size;
	std::vector<std::vector<std::vector<Cell*>>> m_Cells;
};