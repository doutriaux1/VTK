/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractEdges.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkExtractEdges.h"
#include "vtkEdgeTable.h"
#include "vtkMergePoints.h"

// Construct object.
vtkExtractEdges::vtkExtractEdges()
{
  this->Locator = NULL;
}

vtkExtractEdges::~vtkExtractEdges()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

// Generate feature edges for mesh
void vtkExtractEdges::Execute()
{
  vtkDataSet *input= this->GetInput();
  vtkPolyData *output= this->GetOutput();
  vtkPoints *newPts;
  vtkCellArray *newLines;
  int numCells, cellNum, edgeNum, numEdgePts, numCellEdges;
  int numPts, i, pts[2], pt2, newId;
  int pt1 = 0;
  float *x;
  vtkEdgeTable *edgeTable;
  vtkCell *cell, *edge;
  vtkPointData *pd, *outPD;
  vtkCellData *cd, *outCD;

  vtkDebugMacro(<<"Executing edge extractor");

  //  Check input
  //
  numPts=input->GetNumberOfPoints();
  if ( (numCells=input->GetNumberOfCells()) < 1 || numPts < 1 )
    {
    vtkErrorMacro(<<"No input data!");
    return;
    }

  // Set up processing
  //
  edgeTable = vtkEdgeTable::New();
  edgeTable->InitEdgeInsertion(numPts);
  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  newLines = vtkCellArray::New();
  newLines->EstimateSize(numPts*4,2);

  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd,numPts);

  cd = input->GetCellData();
  outCD = output->GetCellData();
  outCD->CopyAllocate(cd,numCells);
  
  // Get our locator for merging points
  //
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPts, input->GetBounds());

  // Loop over all cells, extracting non-visited edges. 
  //
  for (cellNum=0; cellNum < numCells; cellNum++ )
    {
    if ( ! (cellNum % 10000) ) //manage progress reports / early abort
      {
      this->UpdateProgress ((float)cellNum / numCells);
      if ( this->GetAbortExecute() ) 
        {
        break;
        }
      }

    cell = input->GetCell(cellNum);
    numCellEdges = cell->GetNumberOfEdges();
    for (edgeNum=0; edgeNum < numCellEdges; edgeNum++ )
      {
      edge = cell->GetEdge(edgeNum);
      numEdgePts = edge->GetNumberOfPoints();
      
      for ( i=0; i < numEdgePts; i++, pt1=pt2, pts[0]=pts[1] )
        {
        pt2 = edge->PointIds->GetId(i);
	x = input->GetPoint(pt2);
        if ( this->Locator->InsertUniquePoint(x, pts[1]) )
	  {
	  outPD->CopyData (pd,pt2,pts[1]);
	  }
        if ( i > 0 && edgeTable->IsEdge(pt1,pt2) == -1 )
          {
          edgeTable->InsertEdge(pt1, pt2);
          newId = newLines->InsertNextCell(2,pts);
          outCD->CopyData(cd, cellNum, newId);
          }
        }
      }//for all edges of cell
    }//for all cells

  vtkDebugMacro(<<"Created " << newLines->GetNumberOfCells() << " edges");

  //
  //  Update ourselves.
  //
  edgeTable->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  output->Squeeze();
}

// Specify a spatial locator for merging points. By
// default an instance of vtkMergePoints is used.
void vtkExtractEdges::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator ) 
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( locator )
    {
    locator->Register(this);
    }
  this->Locator = locator;
  this->Modified();
}

void vtkExtractEdges::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkExtractEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}


unsigned long int vtkExtractEdges::GetMTime()
{
  unsigned long mTime=this-> vtkDataSetToPolyDataFilter::GetMTime();
  unsigned long time;

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}

