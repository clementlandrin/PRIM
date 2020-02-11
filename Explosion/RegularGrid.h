#pragma once

#include <vector>
#include "External/glm/glm/glm.hpp"

class Cell;
class Fluid;

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

	glm::vec3 UpdateSpeedOfCells();
	void UpdateGradientAndVGradVOfCells();
	void UpdateLaplacianOfCells();
	void UpdateSpeedVariationNavierStokes(Fluid* fluid);

	void ResizeGrid(glm::vec3 size);

	void ResetNavierStokesParametersOfCells();
	void PushNavierStokesParametersToParticles();
private:
	int m_Resolution;
	glm::vec3 m_Size;
	std::vector<std::vector<std::vector<Cell*>>> m_Cells;
};