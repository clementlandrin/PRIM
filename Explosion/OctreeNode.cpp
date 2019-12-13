#include "OctreeNode.hpp"

#include <iostream>

#include "Cell.hpp"
#include "Particle.hpp"
#include "RegularGrid.h"

#define LEFT 0
#define RIGHT 1
#define BOTTOM 0
#define TOP 1
#define FRONT 0
#define BACK 1

OctreeNode::OctreeNode(int positionInGrid[3], std::vector<RegularGrid*> regularGrids)
{
	m_PositionInGrid[0] = positionInGrid[0];
	m_PositionInGrid[1] = positionInGrid[1];
	m_PositionInGrid[2] = positionInGrid[2];

	m_RegularGrids = regularGrids;
}

OctreeNode::~OctreeNode()
{
  if(!m_IsALeaf)
  {
    DeleteChildren();
  }
}

bool OctreeNode::GetIsALeaf() { return m_IsALeaf; }
std::vector<std::vector<std::vector<OctreeNode*>>> OctreeNode:: GetChildren() { return m_Children; }
Cell* OctreeNode::GetCell() { return m_Cell; }
int OctreeNode::GetDepth() { return m_Depth; }

void OctreeNode::SetIsALeaf(bool isALeaf) { m_IsALeaf = isALeaf; }
void OctreeNode::SetCell(Cell* cell) { m_Cell = cell; }
void OctreeNode::SetDepth(int depth) { m_Depth = depth; }

void OctreeNode::DeleteChildren()
{
  if (m_Children.size() != 0)
  {
	delete m_Children[LEFT][BOTTOM][FRONT];
	delete m_Children[RIGHT][BOTTOM][FRONT];
	delete m_Children[LEFT][TOP][FRONT];
	delete m_Children[RIGHT][TOP][FRONT];
	delete m_Children[LEFT][BOTTOM][BACK];
	delete m_Children[RIGHT][BOTTOM][BACK];
	delete m_Children[LEFT][TOP][BACK];
	delete m_Children[RIGHT][TOP][BACK];
  }
}

OctreeNode * OctreeNode::BuildOctree(int depth, int maxDepth, Cell* cell, int positionInGrid[3], std::vector<RegularGrid*> regularGrids)
{
  OctreeNode * nodePtr = new OctreeNode(positionInGrid, regularGrids);
  nodePtr->SetDepth(depth);
  nodePtr->SetCell(cell);

  cell->ComputeEnergy();
  float data = cell->GetEnergy();

	if(depth>maxDepth)
	{
		nodePtr->SetIsALeaf(true);
	}
	else
	{
    nodePtr->AllocateChildren();
    nodePtr->BuildOctreeFromChildren(depth, maxDepth, cell, regularGrids);

		nodePtr->SetIsALeaf(false);
	}

	return nodePtr;
}

void OctreeNode::BuildOctreeFromChildren(int depth, int maxDepth, Cell* cell, std::vector<RegularGrid*> regularGrids)
{
  float child_width = cell->GetCellWidth()/2.0;
  float child_height = cell->GetCellHeight()/2.0;
  float child_depth = cell->GetCellDepth()/2.0;

  int position_in_grid_left_bottom_front[3] = { 2 * m_PositionInGrid[0],     2 * m_PositionInGrid[1],     2 * m_PositionInGrid[2] };
  int position_in_grid_right_bottom_front[3] = { 2 * m_PositionInGrid[0] + 1, 2 * m_PositionInGrid[1],     2 * m_PositionInGrid[2] };
  int position_in_grid_left_top_front[3] = { 2 * m_PositionInGrid[0],     2 * m_PositionInGrid[1] + 1, 2 * m_PositionInGrid[2] };
  int position_in_grid_right_top_front[3] = { 2 * m_PositionInGrid[0] + 1, 2 * m_PositionInGrid[1] + 1, 2 * m_PositionInGrid[2] };
  int position_in_grid_left_bottom_back[3] = { 2 * m_PositionInGrid[0],     2 * m_PositionInGrid[1],     2 * m_PositionInGrid[2] + 1 };
  int position_in_grid_right_bottom_back[3] = { 2 * m_PositionInGrid[0] + 1, 2 * m_PositionInGrid[1],     2 * m_PositionInGrid[2] + 1 };
  int position_in_grid_left_top_back[3] = { 2 * m_PositionInGrid[0],     2 * m_PositionInGrid[1] + 1, 2 * m_PositionInGrid[2] + 1 };
  int position_in_grid_right_top_back[3] = { 2 * m_PositionInGrid[0] + 1, 2 * m_PositionInGrid[1] + 1, 2 * m_PositionInGrid[2] + 1 };

  Cell* cell_left_bottom_front = regularGrids[depth + 1]->GetCells()[position_in_grid_left_bottom_front[0]][position_in_grid_left_bottom_front[1]][position_in_grid_left_bottom_front[2]];
  Cell* cell_right_bottom_front = regularGrids[depth + 1]->GetCells()[position_in_grid_right_bottom_front[0]][position_in_grid_right_bottom_front[1]][position_in_grid_right_bottom_front[2]];
  Cell* cell_left_top_front     = regularGrids[depth + 1]->GetCells()[position_in_grid_left_top_front[0]][position_in_grid_left_top_front[1]][position_in_grid_left_top_front[2]];
  Cell* cell_right_top_front    = regularGrids[depth + 1]->GetCells()[position_in_grid_right_top_front[0]][position_in_grid_right_top_front[1]][position_in_grid_right_top_front[2]];
  Cell* cell_left_bottom_back   = regularGrids[depth + 1]->GetCells()[position_in_grid_left_bottom_back[0]][position_in_grid_left_bottom_back[1]][position_in_grid_left_bottom_back[2]];
  Cell* cell_right_bottom_back  = regularGrids[depth + 1]->GetCells()[position_in_grid_right_bottom_back[0]][position_in_grid_right_bottom_back[1]][position_in_grid_right_bottom_back[2]];
  Cell* cell_left_top_back      = regularGrids[depth + 1]->GetCells()[position_in_grid_left_top_back[0]][position_in_grid_left_top_back[1]][position_in_grid_left_top_back[2]];
  Cell* cell_right_top_back     = regularGrids[depth + 1]->GetCells()[position_in_grid_right_top_back[0]][position_in_grid_right_top_back[1]][position_in_grid_right_top_back[2]];

  PushParticlesInChildrenCells(cell_left_bottom_front, cell_right_bottom_front, cell_left_top_front, cell_right_top_front,
                               cell_left_bottom_back, cell_right_bottom_back, cell_left_top_back, cell_right_top_back);

  m_Children[LEFT][BOTTOM][FRONT] = BuildOctree(depth + 1, maxDepth, cell_left_bottom_front, position_in_grid_left_bottom_front, regularGrids);
  m_Children[RIGHT][BOTTOM][FRONT] = BuildOctree(depth+1, maxDepth, cell_right_bottom_front, position_in_grid_right_bottom_front, regularGrids);

  m_Children[LEFT][TOP][FRONT]     = BuildOctree(depth+1, maxDepth, cell_left_top_front, position_in_grid_left_top_front, regularGrids);
  m_Children[RIGHT][TOP][FRONT]    = BuildOctree(depth+1, maxDepth, cell_right_top_front, position_in_grid_right_top_front, regularGrids);

  m_Children[LEFT][BOTTOM][BACK]   = BuildOctree(depth+1, maxDepth, cell_left_bottom_back, position_in_grid_left_bottom_back, regularGrids);
  m_Children[RIGHT][BOTTOM][BACK]  = BuildOctree(depth+1, maxDepth, cell_right_bottom_back, position_in_grid_right_bottom_back, regularGrids);

  m_Children[LEFT][TOP][BACK]      = BuildOctree(depth+1, maxDepth, cell_left_top_back, position_in_grid_left_top_back, regularGrids);
  m_Children[RIGHT][TOP][BACK]     = BuildOctree(depth+1, maxDepth, cell_right_top_back, position_in_grid_right_top_back, regularGrids);
}

void OctreeNode::PushParticlesInChildrenCells(Cell* cell_left_bottom_front, Cell* cell_right_bottom_front, 
                                    Cell* cell_left_top_front, Cell* cell_right_top_front,
                                    Cell* cell_left_bottom_back, Cell* cell_right_bottom_back,
                                    Cell* cell_left_top_back, Cell* cell_right_top_back)
{
  Particle* particle;

  for (int i = 0; i < m_Cell->GetParticles().size(); i++)
  {
    particle = m_Cell->GetParticles()[i];

    // Left
    if (particle->GetPosition().x < m_Cell->GetPosition().x + m_Cell->GetCellWidth()/2.0)
    {
      // Left Bottom
      if (particle->GetPosition().y < m_Cell->GetPosition().y + m_Cell->GetCellHeight()/2.0)
      {
        // Left Bottom Front
        if (particle->GetPosition().z < m_Cell->GetPosition().z + m_Cell->GetCellDepth()/2.0)
        {
			cell_left_bottom_front->AddParticle(particle);
        }
        else // Left Bottom Back
        {
			cell_left_bottom_back->AddParticle(particle);
        }
      }
      else // Left Top
      {
        // Left Top Front
        if (particle->GetPosition().z < m_Cell->GetPosition().z + m_Cell->GetCellDepth()/2.0)
        {
			cell_left_top_front->AddParticle(particle);
        }
        else // Left Top Back
        {
			cell_left_top_back->AddParticle(particle);
        }
      }
    }
    else // Right
    {
      // Right Bottom
      if (particle->GetPosition().y < m_Cell->GetPosition().y + m_Cell->GetCellHeight()/2.0)
      {
        // Right Bottom Front
        if (particle->GetPosition().z < m_Cell->GetPosition().z + m_Cell->GetCellDepth()/2.0)
        {
			cell_right_bottom_front->AddParticle(particle);
        }
        else // Right Bottom Back
        {
			cell_right_bottom_back->AddParticle(particle);
        }
      }
      else // Right Top
      {
        // Right Top Front
        if (particle->GetPosition().z < m_Cell->GetPosition().z + m_Cell->GetCellDepth()/2.0)
        {
			cell_right_top_front->AddParticle(particle);
        }
        else // Right Top Back
        {
			cell_right_top_back->AddParticle(particle);
        }
      }
    }
  }
}

void OctreeNode::UpdateParticlesInChildrenCells()
{
	if (!m_IsALeaf)
	{
		m_Children[LEFT][BOTTOM][FRONT]->GetCell()->ClearParticles();
		m_Children[RIGHT][BOTTOM][FRONT]->GetCell()->ClearParticles();
		m_Children[LEFT][TOP][FRONT]->GetCell()->ClearParticles();
		m_Children[RIGHT][TOP][FRONT]->GetCell()->ClearParticles();
		m_Children[LEFT][BOTTOM][BACK]->GetCell()->ClearParticles();
		m_Children[RIGHT][BOTTOM][BACK]->GetCell()->ClearParticles();
		m_Children[LEFT][TOP][BACK]->GetCell()->ClearParticles();
		m_Children[RIGHT][TOP][BACK]->GetCell()->ClearParticles();
		
		
		PushParticlesInChildrenCells(m_Children[LEFT][BOTTOM][FRONT]->GetCell(), m_Children[RIGHT][BOTTOM][FRONT]->GetCell(),
									 m_Children[LEFT][TOP][FRONT]->GetCell(), m_Children[RIGHT][TOP][FRONT]->GetCell(), 
									 m_Children[LEFT][BOTTOM][BACK]->GetCell(), m_Children[RIGHT][BOTTOM][BACK]->GetCell(),
									 m_Children[LEFT][TOP][BACK]->GetCell(), m_Children[RIGHT][TOP][BACK]->GetCell());

		m_Children[LEFT][BOTTOM][FRONT]->UpdateParticlesInChildrenCells();
		m_Children[RIGHT][BOTTOM][FRONT]->UpdateParticlesInChildrenCells();
		m_Children[LEFT][TOP][FRONT]->UpdateParticlesInChildrenCells();
		m_Children[RIGHT][TOP][FRONT]->UpdateParticlesInChildrenCells();
		m_Children[LEFT][BOTTOM][BACK]->UpdateParticlesInChildrenCells();
		m_Children[RIGHT][BOTTOM][BACK]->UpdateParticlesInChildrenCells();
		m_Children[LEFT][TOP][BACK]->UpdateParticlesInChildrenCells();
		m_Children[RIGHT][TOP][BACK]->UpdateParticlesInChildrenCells();
	}
}

void OctreeNode::AllocateChildren()
{
  m_Children.resize(2);
  m_Children[0].resize(2);
  m_Children[1].resize(2);
  m_Children[0][0].resize(2);
  m_Children[0][1].resize(2);
  m_Children[1][0].resize(2);
  m_Children[1][1].resize(2);
}