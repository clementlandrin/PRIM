// Cell.hpp
#ifndef CELL_HPP // include guard
#define CELL_HPP

#include "External/glm/glm/glm.hpp"

#include <vector>

class Particle;

class RegularGrid;

class Cell
{
public:
	Cell(Cell* parent, glm::vec3 position, float width, float height, float depth, int indexInRegularGrid[3], RegularGrid* regularGrid);
	~Cell();

	void AddParticle(Particle* particle);
	void ClearParticles();
	void EraseParticleAtIndex(int index);
	void ComputeEnergy();
	void UpdateSpeed();

  	std::vector<Particle*> GetParticles();
	const glm::vec3 GetPosition();
  	const float GetCellWidth();
  	const float GetCellHeight();
  	const float GetCellDepth();
  	const float GetEnergy();
	const Cell* GetParent();
	const glm::vec3 ComputeCenter();

	void SetParticles(std::vector<Particle*> particles);
	void SetPosition(glm::vec3 position);
	void SetCellWidth(float width);
	void SetCellHeight(float height);
	void SetCellDepth(float depth);
	void SetEnergy(float energy);
	void SetParent(Cell* parent);
	
private:
	Cell* GetOnTopCell();
	Cell* GetOnBottomCell();
	Cell* GetOnLeftCell();
	Cell* GetOnRightCell();
	Cell* GetOnFrontCell();
	Cell* GetOnBackCell();

	RegularGrid* m_RegularGrid;
	Cell* m_Parent;
	glm::vec3 m_Position;
	float m_CellWidth;
	float m_CellHeight;
	float m_CellDepth;
	glm::vec3 m_Speed;
	float m_Energy;
	std::vector<Particle*> m_Particles;
	int m_IndexInRegularGrid[3];
};

#endif /* CELL_HPP */
