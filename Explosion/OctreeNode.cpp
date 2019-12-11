#include "OctreeNode.hpp"

#include <iostream>

#include "Cell.hpp"
#include "Particle.hpp"

#define LEFT 0
#define RIGHT 1
#define BOTTOM 0
#define TOP 1
#define FRONT 0
#define BACK 1

OctreeNode::OctreeNode(){

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
	delete m_Children[0][0][0];
	delete m_Children[0][0][1];
	delete m_Children[0][1][0];
	delete m_Children[0][1][1];
	delete m_Children[1][0][0];
	delete m_Children[1][0][1];
	delete m_Children[1][1][0];
	delete m_Children[1][1][1];
  }
}

OctreeNode * OctreeNode::BuildOctree(int depth, int maxDepth, Cell* cell)
{
  OctreeNode * nodePtr = new OctreeNode();
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
    nodePtr->BuildOctreeFromChildren(depth, maxDepth, cell);

		nodePtr->SetIsALeaf(false);
	}

	return nodePtr;
}

void OctreeNode::BuildOctreeFromChildren(int depth, int maxDepth, Cell* cell)
{
  float child_width = cell->GetCellWidth()/2.0;
  float child_height = cell->GetCellHeight()/2.0;
  float child_depth = cell->GetCellDepth()/2.0;

  Cell* cell_left_bottom_front  = new Cell(m_Cell,
                                                 cell->GetPosition(),
                                                 child_width,
                                                 child_height,
                                                 child_depth);
  Cell* cell_right_bottom_front = new Cell(m_Cell,
                                                 cell->GetPosition() + glm::vec3(child_width, 0.0, 0.0),
                                                 child_width,
                                                 child_height,
                                                 child_depth);
  Cell* cell_left_top_front     = new Cell(m_Cell,
                                                 cell->GetPosition() + glm::vec3(0.0, child_height, 0.0),
                                                 child_width,
                                                 child_height,
                                                 child_depth);
  Cell* cell_right_top_front    = new Cell(m_Cell,
                                                 cell->GetPosition() + glm::vec3(child_width, child_height, 0.0),
                                                 child_width,
                                                 child_height,
                                                 child_depth);
  Cell* cell_left_bottom_back   = new Cell(m_Cell,
                                                 cell->GetPosition() + glm::vec3(0.0, 0.0, child_depth),
                                                 child_width,
                                                 child_height,
                                                 child_depth);
  Cell* cell_right_bottom_back  = new Cell(m_Cell,
                                                 cell->GetPosition() + glm::vec3(child_width, 0.0, child_depth),
                                                 child_width,
                                                 child_height,
                                                 child_depth);
  Cell* cell_left_top_back      = new Cell(m_Cell,
                                                 cell->GetPosition() + glm::vec3(0.0, child_height, child_depth),
                                                 child_width,
                                                 child_height,
                                                 child_depth);
  Cell* cell_right_top_back     = new Cell(m_Cell,
                                                 cell->GetPosition() + glm::vec3(child_width, child_height, child_depth),
                                                 child_width,
                                                 child_height,
                                                 child_depth);

  PushParticlesInChildrenCells(cell_left_bottom_front, cell_right_bottom_front, cell_left_top_front, cell_right_top_front,
                               cell_left_bottom_back, cell_right_bottom_back, cell_left_top_back, cell_right_top_back);

  m_Children[LEFT][BOTTOM][FRONT]  = BuildOctree(depth+1, maxDepth, cell_left_bottom_front);
  m_Children[RIGHT][BOTTOM][FRONT] = BuildOctree(depth+1, maxDepth, cell_right_bottom_front);

  m_Children[LEFT][TOP][FRONT]     = BuildOctree(depth+1, maxDepth, cell_left_top_front);
  m_Children[RIGHT][TOP][FRONT]    = BuildOctree(depth+1, maxDepth, cell_right_top_front);

  m_Children[LEFT][BOTTOM][BACK]   = BuildOctree(depth+1, maxDepth, cell_left_bottom_back);
  m_Children[RIGHT][BOTTOM][BACK]  = BuildOctree(depth+1, maxDepth, cell_right_bottom_back);

  m_Children[LEFT][TOP][BACK]      = BuildOctree(depth+1, maxDepth, cell_left_top_back);
  m_Children[RIGHT][TOP][BACK]     = BuildOctree(depth+1, maxDepth, cell_right_top_back);
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
        if (particle->GetPosition().z > m_Cell->GetPosition().z + m_Cell->GetCellDepth()/2.0)
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
        if (particle->GetPosition().z > m_Cell->GetPosition().z + m_Cell->GetCellDepth()/2.0)
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
        if (particle->GetPosition().z > m_Cell->GetPosition().z + m_Cell->GetCellDepth()/2.0)
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
        if (particle->GetPosition().z > m_Cell->GetPosition().z + m_Cell->GetCellDepth()/2.0)
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
		m_Children[LEFT][BOTTOM][FRONT]->GetCell()->GetParticles().clear();
		m_Children[RIGHT][BOTTOM][FRONT]->GetCell()->GetParticles().clear();
		m_Children[LEFT][TOP][FRONT]->GetCell()->GetParticles().clear();
		m_Children[RIGHT][TOP][FRONT]->GetCell()->GetParticles().clear();

		m_Children[LEFT][BOTTOM][BACK]->GetCell()->GetParticles().clear();
		m_Children[RIGHT][BOTTOM][BACK]->GetCell()->GetParticles().clear();
		m_Children[LEFT][TOP][BACK]->GetCell()->GetParticles().clear();
		m_Children[RIGHT][TOP][BACK]->GetCell()->GetParticles().clear();

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