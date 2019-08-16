#include <math.h>
#if USE_MPI
# include <mpi.h>
#endif
#if _OPENMP
#include <omp.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <cstdlib>
#include "lulesh.h"

/////////////////////////////////////////////////////////////////////
Domain::Domain(Int_t numRanks, Index_t colLoc,
               Index_t rowLoc, Index_t planeLoc,
               Index_t nx, Int_t tp, Int_t nr, Int_t balance, Int_t cost)
   :
   energyToleranceCutoff(Real_t(1.0e-7)),
   pressureToleranceCutoff(Real_t(1.0e-7)),
   qToleranceCutoff(Real_t(1.0e-7)),
   relativeVolumeToleranceCutoff(Real_t(1.0e-10)),
   velocityToleranceCutoff(Real_t(1.0e-7)),
   hourglassControlCoefficient(Real_t(3.0)),
   m_ss4o3(Real_t(4.0)/Real_t(3.0)),
   excessiveQIndicator(Real_t(1.0e+12)),
   m_monoq_max_slope(Real_t(1.0)),
   m_monoq_limiter_mult(Real_t(2.0)),
   linearQCoefficient(Real_t(0.5)),
   quadraticQCoefficient(Real_t(2.0)/Real_t(3.0)),
   m_qqc(Real_t(2.0)),
   m_eosvmax(Real_t(1.0e+9)),
   m_eosvmin(Real_t(1.0e-9)),
   pressureFloor(Real_t(0.)),
   energyFloor(Real_t(-1.0e+15)),
   maxAllowedVolumeChange(Real_t(0.1)),
   referenceDensity(Real_t(1.0)),
//
// set pointers to (potentially) "new'd" arrays to null to 
// simplify deallocation.
//
   regionNumberPerElement(0),
   m_nodeElemStart(0),
   m_nodeElemCornerList(0),
   regionSetSizes(0),
   regionIndexset(0)
#if USE_MPI
   , 
   commDataSend(0),
   commDataRecv(0)
#endif
{

   Index_t edgeElems = nx ;
   Index_t edgeNodes = edgeElems+1 ;
   this->getImbalanceCost() = cost;

   m_tp       = tp ;
   numberOfRanks = numRanks ;

   ///////////////////////////////
   //   Initialize Sedov Mesh
   ///////////////////////////////

   // construct a uniform box for this processor

   m_colLoc   =   colLoc ;
   m_rowLoc   =   rowLoc ;
   m_planeLoc = planeLoc ;
   
   m_sizeX = edgeElems ;
   m_sizeY = edgeElems ;
   m_sizeZ = edgeElems ;
   numberOfElements = edgeElems*edgeElems*edgeElems ;

   numberOfNodes = edgeNodes*edgeNodes*edgeNodes ;

   regionNumberPerElement = new Index_t[getNumberOfElements()] ;  // material indexset

   // Elem-centered 
   AllocateElemPersistent(getNumberOfElements()) ;

   // Node-centered 
   AllocateNodePersistent(getNumberOfNodes()) ;

   SetupCommBuffers(edgeNodes);

   // Basic Field Initialization 
   for (Index_t i=0; i<getNumberOfElements(); ++i) {
      e(i) =  Real_t(0.0) ;
      getPressure(i) =  Real_t(0.0) ;
      getArtificialViscosity(i) =  Real_t(0.0) ;
      getSoundSpeed(i) = Real_t(0.0) ;
   }

   // Note - v initializes to 1.0, not 0.0!
   for (Index_t i=0; i<getNumberOfElements(); ++i) {
      v(i) = Real_t(1.0) ;
   }

   for (Index_t i=0; i<getNumberOfNodes(); ++i) {
      getVelocityX(i) = Real_t(0.0) ;
      getVelocityY(i) = Real_t(0.0) ;
      getVelocityZ(i) = Real_t(0.0) ;
   }

   for (Index_t i=0; i<getNumberOfNodes(); ++i) {
      getAccelerationX(i) = Real_t(0.0) ;
      getAccelerationY(i) = Real_t(0.0) ;
      getAccelerationZ(i) = Real_t(0.0) ;
   }

   for (Index_t i=0; i<getNumberOfNodes(); ++i) {
      getNodalMass(i) = Real_t(0.0) ;
   }

   BuildMesh(nx, edgeNodes, edgeElems);

#if _OPENMP
   SetupThreadSupportStructures();
#endif

   // Setup region index sets. For now, these are constant sized
   // throughout the run, but could be changed every cycle to 
   // simulate effects of ALE on the lagrange solver
   CreateRegionIndexSets(nr, balance);

   // Setup symmetry nodesets
   SetupSymmetryPlanes(edgeNodes);

   // Setup element connectivities
   SetupElementConnectivities(edgeElems);

   // Setup symmetry planes and free surface boundary arrays
   SetupBoundaryConditions(edgeElems);


   // Setup defaults

   // These can be changed (requires recompile) if you want to run
   // with a fixed timestep, or to a different end time, but it's
   // probably easier/better to just run a fixed number of timesteps
   // using the -i flag in 2.x

   getFixedTimeIncrement() = Real_t(-1.0e-6) ; // Negative means use courant condition
   getStopTime()  = Real_t(1.0e-2); // *Real_t(edgeElems*tp/45.0) ;

   // Initial conditions
   deltatimemultlb() = Real_t(1.1) ;
   deltatimemultub() = Real_t(1.2) ;
   getCourantConstraint() = Real_t(1.0e+20) ;
   getVolumeChangeConstraint()   = Real_t(1.0e+20) ;
   getMaxAllowableTimeIncrement()     = Real_t(1.0e-2) ;
   getCurrentTime()    = Real_t(0.) ;
   getIterationCount()   = Int_t(0) ;

   // initialize field data 
   for (Index_t i=0; i<getNumberOfElements(); ++i) {
      Real_t x_local[8], y_local[8], z_local[8] ;
      Index_t *elemToNode = nodelist(i) ;
      for( Index_t lnode=0 ; lnode<8 ; ++lnode )
      {
        Index_t gnode = elemToNode[lnode];
        x_local[lnode] = getCoordinateX(gnode);
        y_local[lnode] = getCoordinateY(gnode);
        z_local[lnode] = getCoordinateZ(gnode);
      }

      // volume calculations
      Real_t volume = CalcElemVolume(x_local, y_local, z_local );
      getReferenceVolume(i) = volume ;
      elemMass(i) = volume ;
      for (Index_t j=0; j<8; ++j) {
         Index_t idx = elemToNode[j] ;
         getNodalMass(idx) += volume / Real_t(8.0) ;
      }
   }

   // deposit initial energy
   // An energy of 3.948746e+7 is correct for a problem with
   // 45 zones along a side - we need to scale it
   const Real_t ebase = Real_t(3.948746e+7);
   Real_t scale = (nx*m_tp)/Real_t(45.0);
   Real_t einit = ebase*scale*scale*scale;
   if (m_rowLoc + m_colLoc + m_planeLoc == 0) {
      // Dump into the first zone (which we know is in the corner)
      // of the domain that sits at the origin
      e(0) = einit;
   }
   //set initial deltatime base on analytic CFL calculation
   getDeltaTime() = (Real_t(.5)*cbrt(getReferenceVolume(0)))/sqrt(Real_t(2.0)*einit);

} // End constructor


////////////////////////////////////////////////////////////////////////////////
Domain::~Domain()
{
   delete [] regionNumberPerElement;
   delete [] m_nodeElemStart;
   delete [] m_nodeElemCornerList;
   delete [] regionSetSizes;
   for (Index_t i=0 ; i<getNumberOfRegions() ; ++i) {
     delete [] regionIndexset[i];
   }
   delete [] regionIndexset;
   
#if USE_MPI
   delete [] commDataSend;
   delete [] commDataRecv;
#endif
} // End destructor


////////////////////////////////////////////////////////////////////////////////
void
Domain::BuildMesh(Int_t nx, Int_t edgeNodes, Int_t edgeElems)
{
  Index_t meshEdgeElems = m_tp*nx ;

  // initialize nodal coordinates 
  Index_t nidx = 0 ;
  Real_t tz = Real_t(1.125)*Real_t(m_planeLoc*nx)/Real_t(meshEdgeElems) ;
  for (Index_t plane=0; plane<edgeNodes; ++plane) {
    Real_t ty = Real_t(1.125)*Real_t(m_rowLoc*nx)/Real_t(meshEdgeElems) ;
    for (Index_t row=0; row<edgeNodes; ++row) {
      Real_t tx = Real_t(1.125)*Real_t(m_colLoc*nx)/Real_t(meshEdgeElems) ;
      for (Index_t col=0; col<edgeNodes; ++col) {
	getCoordinateX(nidx) = tx ;
	getCoordinateY(nidx) = ty ;
	getCoordinateZ(nidx) = tz ;
	++nidx ;
	// tx += ds ; // may accumulate roundoff... 
	tx = Real_t(1.125)*Real_t(m_colLoc*nx+col+1)/Real_t(meshEdgeElems) ;
      }
      // ty += ds ;  // may accumulate roundoff... 
      ty = Real_t(1.125)*Real_t(m_rowLoc*nx+row+1)/Real_t(meshEdgeElems) ;
    }
    // tz += ds ;  // may accumulate roundoff... 
    tz = Real_t(1.125)*Real_t(m_planeLoc*nx+plane+1)/Real_t(meshEdgeElems) ;
  }


  // embed hexehedral elements in nodal point lattice 
  Index_t zidx = 0 ;
  nidx = 0 ;
  for (Index_t plane=0; plane<edgeElems; ++plane) {
    for (Index_t row=0; row<edgeElems; ++row) {
      for (Index_t col=0; col<edgeElems; ++col) {
	Index_t *localNode = nodelist(zidx) ;
	localNode[0] = nidx                                       ;
	localNode[1] = nidx                                   + 1 ;
	localNode[2] = nidx                       + edgeNodes + 1 ;
	localNode[3] = nidx                       + edgeNodes     ;
	localNode[4] = nidx + edgeNodes*edgeNodes                 ;
	localNode[5] = nidx + edgeNodes*edgeNodes             + 1 ;
	localNode[6] = nidx + edgeNodes*edgeNodes + edgeNodes + 1 ;
	localNode[7] = nidx + edgeNodes*edgeNodes + edgeNodes     ;
	++zidx ;
	++nidx ;
      }
      ++nidx ;
    }
    nidx += edgeNodes ;
  }
}


////////////////////////////////////////////////////////////////////////////////
void
Domain::SetupThreadSupportStructures()
{
#if _OPENMP
   Index_t numthreads = omp_get_max_threads();
#else
   Index_t numthreads = 1;
#endif

  if (numthreads > 1) {
    // set up node-centered indexing of elements 
    Index_t *nodeElemCount = new Index_t[getNumberOfNodes()] ;

    for (Index_t i=0; i<getNumberOfNodes(); ++i) {
      nodeElemCount[i] = 0 ;
    }

    for (Index_t i=0; i<getNumberOfElements(); ++i) {
      Index_t *nl = nodelist(i) ;
      for (Index_t j=0; j < 8; ++j) {
	++(nodeElemCount[nl[j]] );
      }
    }

    m_nodeElemStart = new Index_t[getNumberOfNodes()+1] ;

    m_nodeElemStart[0] = 0;

    for (Index_t i=1; i <= getNumberOfNodes(); ++i) {
      m_nodeElemStart[i] =
	m_nodeElemStart[i-1] + nodeElemCount[i-1] ;
    }
       
    m_nodeElemCornerList = new Index_t[m_nodeElemStart[getNumberOfNodes()]];

    for (Index_t i=0; i < getNumberOfNodes(); ++i) {
      nodeElemCount[i] = 0;
    }

    for (Index_t i=0; i < getNumberOfElements(); ++i) {
      Index_t *nl = nodelist(i) ;
      for (Index_t j=0; j < 8; ++j) {
	Index_t m = nl[j];
	Index_t k = i*8 + j ;
	Index_t offset = m_nodeElemStart[m] + nodeElemCount[m] ;
	m_nodeElemCornerList[offset] = k;
	++(nodeElemCount[m]) ;
      }
    }

    Index_t clSize = m_nodeElemStart[getNumberOfNodes()] ;
    for (Index_t i=0; i < clSize; ++i) {
      Index_t clv = m_nodeElemCornerList[i] ;
      if ((clv < 0) || (clv > getNumberOfElements()*8)) {
	fprintf(stderr,
		"AllocateNodeElemIndexes(): nodeElemCornerList entry out of range!\n");
#if USE_MPI
	MPI_Abort(MPI_COMM_WORLD, -1);
#else
	exit(-1);
#endif
      }
    }

    delete [] nodeElemCount ;
  }
}


////////////////////////////////////////////////////////////////////////////////
void
Domain::SetupCommBuffers(Int_t edgeNodes)
{
  // allocate a buffer large enough for nodal ghost data 
  Index_t maxEdgeSize = MAX(this->sizeX(), MAX(this->sizeY(), this->sizeZ()))+1 ;
  maxPlaneSize = CACHE_ALIGN_REAL(maxEdgeSize*maxEdgeSize) ;
  maxEdgeSize = CACHE_ALIGN_REAL(maxEdgeSize) ;

  // assume communication to 6 neighbors by default 
  m_rowMin = (m_rowLoc == 0)        ? 0 : 1;
  m_rowMax = (m_rowLoc == m_tp-1)     ? 0 : 1;
  m_colMin = (m_colLoc == 0)        ? 0 : 1;
  m_colMax = (m_colLoc == m_tp-1)     ? 0 : 1;
  m_planeMin = (m_planeLoc == 0)    ? 0 : 1;
  m_planeMax = (m_planeLoc == m_tp-1) ? 0 : 1;

#if USE_MPI   
  // account for face communication 
  Index_t comBufSize =
    (m_rowMin + m_rowMax + m_colMin + m_colMax + m_planeMin + m_planeMax) *
    maxPlaneSize * MAX_FIELDS_PER_MPI_COMM ;

  // account for edge communication 
  comBufSize +=
    ((m_rowMin & m_colMin) + (m_rowMin & m_planeMin) + (m_colMin & m_planeMin) +
     (m_rowMax & m_colMax) + (m_rowMax & m_planeMax) + (m_colMax & m_planeMax) +
     (m_rowMax & m_colMin) + (m_rowMin & m_planeMax) + (m_colMin & m_planeMax) +
     (m_rowMin & m_colMax) + (m_rowMax & m_planeMin) + (m_colMax & m_planeMin)) *
    maxEdgeSize * MAX_FIELDS_PER_MPI_COMM ;

  // account for corner communication 
  // factor of 16 is so each buffer has its own cache line 
  comBufSize += ((m_rowMin & m_colMin & m_planeMin) +
		 (m_rowMin & m_colMin & m_planeMax) +
		 (m_rowMin & m_colMax & m_planeMin) +
		 (m_rowMin & m_colMax & m_planeMax) +
		 (m_rowMax & m_colMin & m_planeMin) +
		 (m_rowMax & m_colMin & m_planeMax) +
		 (m_rowMax & m_colMax & m_planeMin) +
		 (m_rowMax & m_colMax & m_planeMax)) * CACHE_COHERENCE_PAD_REAL ;

  this->commDataSend = new Real_t[comBufSize] ;
  this->commDataRecv = new Real_t[comBufSize] ;
  // prevent floating point exceptions 
  memset(this->commDataSend, 0, comBufSize*sizeof(Real_t)) ;
  memset(this->commDataRecv, 0, comBufSize*sizeof(Real_t)) ;
#endif   

  // Boundary nodesets
  if (m_colLoc == 0)
    symmetryPlaneNodesetX.resize(edgeNodes*edgeNodes);
  if (m_rowLoc == 0)
    symmetryPlaneNodesetY.resize(edgeNodes*edgeNodes);
  if (m_planeLoc == 0)
    symmetryPlaneNodesetZ.resize(edgeNodes*edgeNodes);
}


////////////////////////////////////////////////////////////////////////////////
void
Domain::CreateRegionIndexSets(Int_t nr, Int_t balance)
{
#if USE_MPI   
   int myRank;
   MPI_Comm_rank(MPI_COMM_WORLD, &myRank) ;
   srand(myRank);
#else
   srand(0);
   Index_t myRank = 0;
#endif
   this->getNumberOfRegions() = nr;
   regionSetSizes = new Index_t[getNumberOfRegions()];
   regionIndexset = new Index_t*[getNumberOfRegions()];
   Index_t nextIndex = 0;
   //if we only have one region just fill it
   // Fill out the regNumList with material numbers, which are always
   // the region index plus one 
   if(getNumberOfRegions() == 1) {
      while (nextIndex < getNumberOfElements()) {
	 this->regNumList(nextIndex) = 1;
         nextIndex++;
      }
      regElemSize(0) = 0;
   }
   //If we have more than one region distribute the elements.
   else {
      Int_t regionNum;
      Int_t regionVar;
      Int_t lastReg = -1;
      Int_t binSize;
      Index_t elements;
      Index_t runto = 0;
      Int_t costDenominator = 0;
      Int_t* regBinEnd = new Int_t[getNumberOfRegions()];
      //Determine the relative weights of all the regions.  This is based off the -b flag.  Balance is the value passed into b.  
      for (Index_t i=0 ; i<getNumberOfRegions() ; ++i) {
         regElemSize(i) = 0;
	 costDenominator += pow((i+1), balance);  //Total sum of all regions weights
	 regBinEnd[i] = costDenominator;  //Chance of hitting a given region is (regBinEnd[i] - regBinEdn[i-1])/costDenominator
      }
      //Until all elements are assigned
      while (nextIndex < getNumberOfElements()) {
	 //pick the region
	 regionVar = rand() % costDenominator;
	 Index_t i = 0;
         while(regionVar >= regBinEnd[i])
	    i++;
         //rotate the regions based on MPI rank.  Rotation is Rank % NumRegions this makes each domain have a different region with 
         //the highest representation
	 regionNum = ((i + myRank) % getNumberOfRegions()) + 1;
	 // make sure we don't pick the same region twice in a row
         while(regionNum == lastReg) {
	    regionVar = rand() % costDenominator;
	    i = 0;
            while(regionVar >= regBinEnd[i])
	       i++;
	    regionNum = ((i + myRank) % getNumberOfRegions()) + 1;
         }
	 //Pick the bin size of the region and determine the number of elements.
         binSize = rand() % 1000;
	 if(binSize < 773) {
	   elements = rand() % 15 + 1;
	 }
	 else if(binSize < 937) {
	   elements = rand() % 16 + 16;
	 }
	 else if(binSize < 970) {
	   elements = rand() % 32 + 32;
	 }
	 else if(binSize < 974) {
	   elements = rand() % 64 + 64;
	 } 
	 else if(binSize < 978) {
	   elements = rand() % 128 + 128;
	 }
	 else if(binSize < 981) {
	   elements = rand() % 256 + 256;
	 }
	 else
	    elements = rand() % 1537 + 512;
	 runto = elements + nextIndex;
	 //Store the elements.  If we hit the end before we run out of elements then just stop.
         while (nextIndex < runto && nextIndex < getNumberOfElements()) {
	    this->regNumList(nextIndex) = regionNum;
	    nextIndex++;
	 }
	 lastReg = regionNum;
      }

      delete [] regBinEnd; 
   }
   // Convert regNumList to region index sets
   // First, count size of each region 
   for (Index_t i=0 ; i<getNumberOfElements() ; ++i) {
      int r = this->regNumList(i)-1; // region index == regnum-1
      regElemSize(r)++;
   }
   // Second, allocate each region index set
   for (Index_t i=0 ; i<getNumberOfRegions() ; ++i) {
      regionIndexset[i] = new Index_t[regElemSize(i)];
      regElemSize(i) = 0;
   }
   // Third, fill index sets
   for (Index_t i=0 ; i<getNumberOfElements() ; ++i) {
      Index_t r = regNumList(i)-1;       // region index == regnum-1
      Index_t regndx = regElemSize(r)++; // Note increment
      regElemlist(r,regndx) = i;
   }
   
}

/////////////////////////////////////////////////////////////
void 
Domain::SetupSymmetryPlanes(Int_t edgeNodes)
{
  Index_t nidx = 0 ;
  for (Index_t i=0; i<edgeNodes; ++i) {
    Index_t planeInc = i*edgeNodes*edgeNodes ;
    Index_t rowInc   = i*edgeNodes ;
    for (Index_t j=0; j<edgeNodes; ++j) {
      if (m_planeLoc == 0) {
	symmetryPlaneNodesetZ[nidx] = rowInc   + j ;
      }
      if (m_rowLoc == 0) {
	symmetryPlaneNodesetY[nidx] = planeInc + j ;
      }
      if (m_colLoc == 0) {
	symmetryPlaneNodesetX[nidx] = planeInc + j*edgeNodes ;
      }
      ++nidx ;
    }
  }
}



/////////////////////////////////////////////////////////////
void
Domain::SetupElementConnectivities(Int_t edgeElems)
{
   lxim(0) = 0 ;
   for (Index_t i=1; i<getNumberOfElements(); ++i) {
      lxim(i)   = i-1 ;
      lxip(i-1) = i ;
   }
   lxip(getNumberOfElements()-1) = getNumberOfElements()-1 ;

   for (Index_t i=0; i<edgeElems; ++i) {
      letam(i) = i ; 
      letap(getNumberOfElements()-edgeElems+i) = getNumberOfElements()-edgeElems+i ;
   }
   for (Index_t i=edgeElems; i<getNumberOfElements(); ++i) {
      letam(i) = i-edgeElems ;
      letap(i-edgeElems) = i ;
   }

   for (Index_t i=0; i<edgeElems*edgeElems; ++i) {
      lzetam(i) = i ;
      lzetap(getNumberOfElements()-edgeElems*edgeElems+i) = getNumberOfElements()-edgeElems*edgeElems+i ;
   }
   for (Index_t i=edgeElems*edgeElems; i<getNumberOfElements(); ++i) {
      lzetam(i) = i - edgeElems*edgeElems ;
      lzetap(i-edgeElems*edgeElems) = i ;
   }
}

/////////////////////////////////////////////////////////////
void
Domain::SetupBoundaryConditions(Int_t edgeElems) 
{
  Index_t ghostIdx[6] ;  // offsets to ghost locations

  // set up boundary condition information
  for (Index_t i=0; i<getNumberOfElements(); ++i) {
     elemBC(i) = Int_t(0) ;
  }

  for (Index_t i=0; i<6; ++i) {
    ghostIdx[i] = INT_MIN ;
  }

  Int_t pidx = getNumberOfElements() ;
  if (m_planeMin != 0) {
    ghostIdx[0] = pidx ;
    pidx += sizeX()*sizeY() ;
  }

  if (m_planeMax != 0) {
    ghostIdx[1] = pidx ;
    pidx += sizeX()*sizeY() ;
  }

  if (m_rowMin != 0) {
    ghostIdx[2] = pidx ;
    pidx += sizeX()*sizeZ() ;
  }

  if (m_rowMax != 0) {
    ghostIdx[3] = pidx ;
    pidx += sizeX()*sizeZ() ;
  }

  if (m_colMin != 0) {
    ghostIdx[4] = pidx ;
    pidx += sizeY()*sizeZ() ;
  }

  if (m_colMax != 0) {
    ghostIdx[5] = pidx ;
  }

  // symmetry plane or free surface BCs 
  for (Index_t i=0; i<edgeElems; ++i) {
    Index_t planeInc = i*edgeElems*edgeElems ;
    Index_t rowInc   = i*edgeElems ;
    for (Index_t j=0; j<edgeElems; ++j) {
      if (m_planeLoc == 0) {
	elemBC(rowInc+j) |= ZETA_M_SYMM ;
      }
      else {
	elemBC(rowInc+j) |= ZETA_M_COMM ;
	lzetam(rowInc+j) = ghostIdx[0] + rowInc + j ;
      }

      if (m_planeLoc == m_tp-1) {
	elemBC(rowInc+j+getNumberOfElements()-edgeElems*edgeElems) |=
	  ZETA_P_FREE;
      }
      else {
	elemBC(rowInc+j+getNumberOfElements()-edgeElems*edgeElems) |=
	  ZETA_P_COMM ;
	lzetap(rowInc+j+getNumberOfElements()-edgeElems*edgeElems) =
	  ghostIdx[1] + rowInc + j ;
      }

      if (m_rowLoc == 0) {
	elemBC(planeInc+j) |= ETA_M_SYMM ;
      }
      else {
	elemBC(planeInc+j) |= ETA_M_COMM ;
	letam(planeInc+j) = ghostIdx[2] + rowInc + j ;
      }

      if (m_rowLoc == m_tp-1) {
	elemBC(planeInc+j+edgeElems*edgeElems-edgeElems) |= 
	  ETA_P_FREE ;
      }
      else {
	elemBC(planeInc+j+edgeElems*edgeElems-edgeElems) |= 
	  ETA_P_COMM ;
	letap(planeInc+j+edgeElems*edgeElems-edgeElems) =
	  ghostIdx[3] +  rowInc + j ;
      }

      if (m_colLoc == 0) {
	elemBC(planeInc+j*edgeElems) |= XI_M_SYMM ;
      }
      else {
	elemBC(planeInc+j*edgeElems) |= XI_M_COMM ;
	lxim(planeInc+j*edgeElems) = ghostIdx[4] + rowInc + j ;
      }

      if (m_colLoc == m_tp-1) {
	elemBC(planeInc+j*edgeElems+edgeElems-1) |= XI_P_FREE ;
      }
      else {
	elemBC(planeInc+j*edgeElems+edgeElems-1) |= XI_P_COMM ;
	lxip(planeInc+j*edgeElems+edgeElems-1) =
	  ghostIdx[5] + rowInc + j ;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////
void InitMeshDecomp(Int_t numRanks, Int_t myRank,
                    Int_t *col, Int_t *row, Int_t *plane, Int_t *side)
{
   Int_t testProcs;
   Int_t dx, dy, dz;
   Int_t myDom;
   
   // Assume cube processor layout for now 
   testProcs = Int_t(cbrt(Real_t(numRanks))+0.5) ;
   if (testProcs*testProcs*testProcs != numRanks) {
      printf("Num processors must be a cube of an integer (1, 8, 27, ...)\n") ;
#if USE_MPI      
      MPI_Abort(MPI_COMM_WORLD, -1) ;
#else
      exit(-1);
#endif
   }
   if (sizeof(Real_t) != 4 && sizeof(Real_t) != 8) {
      printf("MPI operations only support float and double right now...\n");
#if USE_MPI      
      MPI_Abort(MPI_COMM_WORLD, -1) ;
#else
      exit(-1);
#endif
   }
   if (MAX_FIELDS_PER_MPI_COMM > CACHE_COHERENCE_PAD_REAL) {
      printf("corner element comm buffers too small.  Fix code.\n") ;
#if USE_MPI      
      MPI_Abort(MPI_COMM_WORLD, -1) ;
#else
      exit(-1);
#endif
   }

   dx = testProcs ;
   dy = testProcs ;
   dz = testProcs ;

   // temporary test
   if (dx*dy*dz != numRanks) {
      printf("error -- must have as many domains as procs\n") ;
#if USE_MPI      
      MPI_Abort(MPI_COMM_WORLD, -1) ;
#else
      exit(-1);
#endif
   }
   Int_t remainder = dx*dy*dz % numRanks ;
   if (myRank < remainder) {
      myDom = myRank*( 1+ (dx*dy*dz / numRanks)) ;
   }
   else {
      myDom = remainder*( 1+ (dx*dy*dz / numRanks)) +
         (myRank - remainder)*(dx*dy*dz/numRanks) ;
   }

   *col = myDom % dx ;
   *row = (myDom / dx) % dy ;
   *plane = myDom / (dx*dy) ;
   *side = testProcs;

   return;
}

