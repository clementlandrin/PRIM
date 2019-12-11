#ifndef OCTREENODE_HPP
#define OCTREENODE_HPP

#include "External/glm/glm/glm.hpp"

#include <vector>

class Particle;

class Cell;

class OctreeNode {
public:
	OctreeNode();
	~OctreeNode();
	
	static OctreeNode * BuildOctree(int depth, int maxDepth, Cell* cell);

	bool GetIsALeaf();
	Cell* GetCell();
	std::vector<std::vector<std::vector<OctreeNode*>>> GetChildren();
	int GetDepth();

	void SetIsALeaf(bool isALeaf);
	void SetCell(Cell* cell);
	void SetDepth(int depth);

	void UpdateParticlesInChildrenCells();
private:
  Cell* m_Cell;
  std::vector<std::vector<std::vector<OctreeNode*>>> m_Children;
  bool m_IsALeaf;
  int m_Depth;

  void BuildOctreeFromChildren(int depth, int maxDepth, Cell* cell);
  void PushParticlesInChildrenCells(Cell* cell_left_bottom_front, Cell* cell_right_bottom_front, 
                                    Cell* cell_left_top_front, Cell* cell_right_top_front,
                                    Cell* cell_left_bottom_back, Cell* cell_right_bottom_back,
                                    Cell* cell_left_top_back, Cell* cell_right_top_back);
  void DeleteChildren();
  void AllocateChildren();
};

#endif // OCTREENODE_HPP