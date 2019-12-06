// Cell.hpp
#ifndef CELL_HPP // include guard
#define CELL_HPP

#include "External/glm/glm/glm.hpp"

#include <vector>

class Particle;

class Cell
{
public:
	Cell();
	Cell(Cell* parent, glm::vec3 position, float width, float height, float depth);

	void AddParticle(Particle* particle);
	void EraseParticleAtIndex(int index);
	void ComputeEnergy();

  	std::vector<Particle*> GetParticles();
	const glm::vec3 GetPosition();
  	const float GetCellWidth();
  	const float GetCellHeight();
  	const float GetCellDepth();
  	const float GetEnergy();
	const Cell* GetParent();

	void SetParticles(std::vector<Particle*> particles);
	void SetPosition(glm::vec3 position);
	void SetCellWidth(float width);
	void SetCellHeight(float height);
	void SetCellDepth(float depth);
	void SetEnergy(float energy);
	void SetParent(Cell* parent);
	
private:
	Cell* m_Parent;
	glm::vec3 m_Position;
	float m_CellWidth;
	float m_CellHeight;
	float m_CellDepth;
	glm::vec3 m_Speed;
	float m_Energy;
	std::vector<Particle*> m_Particles;
};

#endif /* CELL_HPP */
