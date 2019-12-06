#pragma once

#include <vector>
#include "External/glm/glm/glm.hpp"

class Cell;

class RegularGrid
{
public:
	RegularGrid(int resolution, glm::vec3 origin, glm::vec3 size);
	~RegularGrid();

	glm::vec3 GetSize();
	glm::vec3 GetOrigin();
	int GetResolution();

	void SetSize(glm::vec3 size);
	void SetOrigin(glm::vec3 origin);
	void SetResolution(int resolution);
private:
	int m_Resolution;
	glm::vec3 m_Size;
	std::vector<std::vector<std::vector<Cell*>>> m_Cells;
	glm::vec3 m_Origin;
};