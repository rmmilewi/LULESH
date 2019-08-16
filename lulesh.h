#if !defined(USE_MPI)
# error "You should specify USE_MPI=0 or USE_MPI=1 on the compile line"
#endif

#if USE_MPI
#include <mpi.h>

/*
   define one of these three symbols:

   SEDOV_SYNC_POS_VEL_NONE
   SEDOV_SYNC_POS_VEL_EARLY
   SEDOV_SYNC_POS_VEL_LATE
*/

#define SEDOV_SYNC_POS_VEL_EARLY 1
#endif

#include <math.h>
#include <stdlib.h>
#include <vector>

//**************************************************
// Allow flexibility for arithmetic representations 
//**************************************************

#define MAX(a, b) ( ((a) > (b)) ? (a) : (b))


// Precision specification
typedef float        real4 ;
typedef double       real8 ;
typedef long double  real10 ;  // 10 bytes on x86

typedef int32_t Int4_t ;
typedef int64_t Int8_t ;
typedef Int4_t  Index_t ; // array subscript and loop index
typedef real8   Real_t ;  // floating point representation
typedef Int4_t  Int_t ;   // integer representation

enum { VolumeError = -1, QStopError = -2 } ;

inline real4  SQRT(real4  arg) { return sqrtf(arg) ; }
inline real8  SQRT(real8  arg) { return sqrt(arg) ; }
inline real10 SQRT(real10 arg) { return sqrtl(arg) ; }

inline real4  CBRT(real4  arg) { return cbrtf(arg) ; }
inline real8  CBRT(real8  arg) { return cbrt(arg) ; }
inline real10 CBRT(real10 arg) { return cbrtl(arg) ; }

inline real4  FABS(real4  arg) { return fabsf(arg) ; }
inline real8  FABS(real8  arg) { return fabs(arg) ; }
inline real10 FABS(real10 arg) { return fabsl(arg) ; }


// Stuff needed for boundary conditions
// 2 BCs on each of 6 hexahedral faces (12 bits)
#define XI_M        0x00007
#define XI_M_SYMM   0x00001
#define XI_M_FREE   0x00002
#define XI_M_COMM   0x00004

#define XI_P        0x00038
#define XI_P_SYMM   0x00008
#define XI_P_FREE   0x00010
#define XI_P_COMM   0x00020

#define ETA_M       0x001c0
#define ETA_M_SYMM  0x00040
#define ETA_M_FREE  0x00080
#define ETA_M_COMM  0x00100

#define ETA_P       0x00e00
#define ETA_P_SYMM  0x00200
#define ETA_P_FREE  0x00400
#define ETA_P_COMM  0x00800

#define ZETA_M      0x07000
#define ZETA_M_SYMM 0x01000
#define ZETA_M_FREE 0x02000
#define ZETA_M_COMM 0x04000

#define ZETA_P      0x38000
#define ZETA_P_SYMM 0x08000
#define ZETA_P_FREE 0x10000
#define ZETA_P_COMM 0x20000

// MPI Message Tags
#define MSG_COMM_SBN      1024
#define MSG_SYNC_POS_VEL  2048
#define MSG_MONOQ         3072

#define MAX_FIELDS_PER_MPI_COMM 6

// Assume 128 byte coherence
// Assume Real_t is an "integral power of 2" bytes wide
#define CACHE_COHERENCE_PAD_REAL (128 / sizeof(Real_t))

#define CACHE_ALIGN_REAL(n) \
   (((n) + (CACHE_COHERENCE_PAD_REAL - 1)) & ~(CACHE_COHERENCE_PAD_REAL-1))

/*********************************/
/* Data structure implementation */
/*********************************/

/* might want to add access methods so that memory can be */
/* better managed, as in luleshFT */

template <typename T>
T *Allocate(size_t size)
{
   return static_cast<T *>(malloc(sizeof(T)*size)) ;
}

template <typename T>
void Release(T **ptr)
{
   if (*ptr != NULL) {
      free(*ptr) ;
      *ptr = NULL ;
   }
}

//////////////////////////////////////////////////////
// Primary data structure
//////////////////////////////////////////////////////

/*
 * The implementation of the data abstraction used for lulesh
 * resides entirely in the Domain class below.  You can change
 * grouping and interleaving of fields here to maximize data layout
 * efficiency for your underlying architecture or compiler.
 *
 * For example, fields can be implemented as STL objects or
 * raw array pointers.  As another example, individual fields
 * m_x, m_y, m_z could be budled into
 *
 *    struct { Real_t x, y, z ; } *m_coord ;
 *
 * allowing accessor functions such as
 *
 *  "Real_t &x(Index_t idx) { return m_coord[idx].x ; }"
 *  "Real_t &y(Index_t idx) { return m_coord[idx].y ; }"
 *  "Real_t &z(Index_t idx) { return m_coord[idx].z ; }"
 */

class Domain {

   public:

   // Constructor
   Domain(Int_t numRanks, Index_t colLoc,
          Index_t rowLoc, Index_t planeLoc,
          Index_t nx, Int_t tp, Int_t nr, Int_t balance, Int_t cost);

   // Destructor
   ~Domain();

   //
   // ALLOCATION
   //

   void AllocateNodePersistent(Int_t numNode) // Node-centered
   {
      nodalCoordinateX.resize(numNode);  // coordinates
      nodalCoordinateY.resize(numNode);
      nodalCoordinateZ.resize(numNode);

      nodalVelocityX.resize(numNode); // velocities
      nodalVelocityY.resize(numNode);
      nodalVelocityZ.resize(numNode);

      nodalAccelerationX.resize(numNode); // accelerations
      nodalAccelerationY.resize(numNode);
      nodalAccelerationZ.resize(numNode);

      nodalForceX.resize(numNode);  // forces
      nodalForceY.resize(numNode);
      nodalForceZ.resize(numNode);

      nodalMass.resize(numNode);  // mass
   }

   void AllocateElemPersistent(Int_t numElem) // Elem-centered
   {
      nodeConnectivityList.resize(8*numElem);

      // elem connectivities through face
      m_lxim.resize(numElem);
      m_lxip.resize(numElem);
      m_letam.resize(numElem);
      m_letap.resize(numElem);
      m_lzetam.resize(numElem);
      m_lzetap.resize(numElem);

      m_elemBC.resize(numElem);

      energy.resize(numElem);
      pressure.resize(numElem);

      artificialViscosity.resize(numElem);
      qLinearTerm.resize(numElem);
      qQuadraticTerm.resize(numElem);

      relativeVolume.resize(numElem);

      referenceVolume.resize(numElem);
      m_delv.resize(numElem);
      m_vdov.resize(numElem);

      elementCharacteristicLength.resize(numElem);

      soundSpeed.resize(numElem);

      m_elemMass.resize(numElem);

      m_vnew.resize(numElem) ;
   }

   void AllocateGradients(Int_t numElem, Int_t allElem)
   {
      // Position gradients
      m_delx_xi   = Allocate<Real_t>(numElem) ;
      m_delx_eta  = Allocate<Real_t>(numElem) ;
      m_delx_zeta = Allocate<Real_t>(numElem) ;

      // Velocity gradients
      m_delv_xi   = Allocate<Real_t>(allElem) ;
      m_delv_eta  = Allocate<Real_t>(allElem);
      m_delv_zeta = Allocate<Real_t>(allElem) ;
   }

   void DeallocateGradients()
   {
      Release(&m_delx_zeta);
      Release(&m_delx_eta) ;
      Release(&m_delx_xi)  ;

      Release(&m_delv_zeta);
      Release(&m_delv_eta) ;
      Release(&m_delv_xi)  ;
   }

   void AllocateStrains(Int_t numElem)
   {
      principalStrainsX = Allocate<Real_t>(numElem) ;
      principalStrainsY = Allocate<Real_t>(numElem) ;
      principalStrainsZ = Allocate<Real_t>(numElem) ;
   }

   void DeallocateStrains()
   {
      Release(&principalStrainsZ) ;
      Release(&principalStrainsY) ;
      Release(&principalStrainsX) ;
   }
   
   //
   // ACCESSORS
   //

   // Node-centered

   // Nodal coordinates
   Real_t& getCoordinateX(Index_t idx)    { return nodalCoordinateX[idx] ; }
   Real_t& getCoordinateY(Index_t idx)    { return nodalCoordinateY[idx] ; }
   Real_t& getCoordinateZ(Index_t idx)    { return nodalCoordinateZ[idx] ; }

   // Nodal velocities
   Real_t& getVelocityX(Index_t idx)   { return nodalVelocityX[idx] ; }
   Real_t& getVelocityY(Index_t idx)   { return nodalVelocityY[idx] ; }
   Real_t& getVelocityZ(Index_t idx)   { return nodalVelocityZ[idx] ; }

   // Nodal accelerations
   Real_t& getAccelerationX(Index_t idx)  { return nodalAccelerationX[idx] ; }
   Real_t& getAccelerationY(Index_t idx)  { return nodalAccelerationY[idx] ; }
   Real_t& getAccelerationZ(Index_t idx)  { return nodalAccelerationZ[idx] ; }

   // Nodal forces
   Real_t& getForceX(Index_t idx)   { return nodalForceX[idx] ; }
   Real_t& getForceY(Index_t idx)   { return nodalForceY[idx] ; }
   Real_t& getForceZ(Index_t idx)   { return nodalForceZ[idx] ; }

   // Nodal mass
   Real_t& getNodalMass(Index_t idx) { return nodalMass[idx] ; }

   // Nodes on symmertry planes
   Index_t symmX(Index_t idx) { return symmetryPlaneNodesetX[idx] ; }
   Index_t symmY(Index_t idx) { return symmetryPlaneNodesetY[idx] ; }
   Index_t symmZ(Index_t idx) { return symmetryPlaneNodesetZ[idx] ; }
   bool isSymmXEmpty()          { return symmetryPlaneNodesetX.empty(); }
   bool isSymmYEmpty()          { return symmetryPlaneNodesetY.empty(); }
   bool isSymmZEmpty()          { return symmetryPlaneNodesetZ.empty(); }

   //
   // Element-centered
   //
   Index_t&  regElemSize(Index_t idx) { return regionSetSizes[idx] ; }
   Index_t&  regNumList(Index_t idx) { return regionNumberPerElement[idx] ; }
   Index_t*  regNumList()            { return &regionNumberPerElement[0] ; }
   Index_t*  regElemlist(Int_t r)    { return regionIndexset[r] ; }
   Index_t&  regElemlist(Int_t r, Index_t idx) { return regionIndexset[r][idx] ; }

   Index_t*  nodelist(Index_t idx)    { return &nodeConnectivityList[Index_t(8)*idx] ; }

   // elem connectivities through face
   Index_t&  lxim(Index_t idx) { return m_lxim[idx] ; }
   Index_t&  lxip(Index_t idx) { return m_lxip[idx] ; }
   Index_t&  letam(Index_t idx) { return m_letam[idx] ; }
   Index_t&  letap(Index_t idx) { return m_letap[idx] ; }
   Index_t&  lzetam(Index_t idx) { return m_lzetam[idx] ; }
   Index_t&  lzetap(Index_t idx) { return m_lzetap[idx] ; }

   // elem face symm/free-surface flag
   Int_t&  elemBC(Index_t idx) { return m_elemBC[idx] ; }

   // Principal strains - temporary
   Real_t& dxx(Index_t idx)  { return principalStrainsX[idx] ; }
   Real_t& dyy(Index_t idx)  { return principalStrainsY[idx] ; }
   Real_t& dzz(Index_t idx)  { return principalStrainsZ[idx] ; }

   // New relative volume - temporary
   Real_t& vnew(Index_t idx)  { return m_vnew[idx] ; }

   // Velocity gradient - temporary
   Real_t& delv_xi(Index_t idx)    { return m_delv_xi[idx] ; }
   Real_t& delv_eta(Index_t idx)   { return m_delv_eta[idx] ; }
   Real_t& delv_zeta(Index_t idx)  { return m_delv_zeta[idx] ; }

   // Position gradient - temporary
   Real_t& delx_xi(Index_t idx)    { return m_delx_xi[idx] ; }
   Real_t& delx_eta(Index_t idx)   { return m_delx_eta[idx] ; }
   Real_t& delx_zeta(Index_t idx)  { return m_delx_zeta[idx] ; }

   // Energy
   Real_t& e(Index_t idx)          { return energy[idx] ; }

   // Pressure
   Real_t& getPressure(Index_t idx)          { return pressure[idx] ; }

   // Artificial viscosity
   Real_t& getArtificialViscosity(Index_t idx)          { return artificialViscosity[idx] ; }

   // Linear term for q
   Real_t& getLinearTermForQ(Index_t idx)         { return qLinearTerm[idx] ; }

   // Quadratic term for q
   Real_t& getQuadraticTermForQ(Index_t idx)         { return qQuadraticTerm[idx] ; }

   // Relative volume
   Real_t& v(Index_t idx)          { return relativeVolume[idx] ; }
   Real_t& delv(Index_t idx)       { return m_delv[idx] ; }

   // Reference volume
   Real_t& getReferenceVolume(Index_t idx)       { return referenceVolume[idx] ; }

   // volume derivative over volume
   Real_t& vdov(Index_t idx)       { return m_vdov[idx] ; }

   // Element characteristic length
   Real_t& getElementCharacteristicLength(Index_t idx)     { return elementCharacteristicLength[idx] ; }

   // Sound speed
   Real_t& getSoundSpeed(Index_t idx)         { return soundSpeed[idx] ; }

   // Element mass
   Real_t& elemMass(Index_t idx)  { return m_elemMass[idx] ; }

   Index_t nodeElemCount(Index_t idx)
   { return m_nodeElemStart[idx+1] - m_nodeElemStart[idx] ; }

   Index_t *nodeElemCornerList(Index_t idx)
   { return &m_nodeElemCornerList[m_nodeElemStart[idx]] ; }

   // Parameters 

   // Cutoffs
   Real_t getVelocityToleranceCutoff() const               { return velocityToleranceCutoff ; }
   Real_t getEnergyToleranceCutoff() const               { return energyToleranceCutoff ; }
   Real_t getPressureToleranceCutoff() const               { return pressureToleranceCutoff ; }
   Real_t getQToleranceCutoff() const               { return qToleranceCutoff ; }
   Real_t getRelativeVolumeToleranceCutoff() const               { return relativeVolumeToleranceCutoff ; }

   // Other constants (usually are settable via input file in real codes)
   Real_t getHourglassControlCoefficient() const              { return hourglassControlCoefficient ; }
   Real_t getExcessiveQIndicator() const               { return excessiveQIndicator ; }
   Real_t monoq_max_slope() const     { return m_monoq_max_slope ; }
   Real_t monoq_limiter_mult() const  { return m_monoq_limiter_mult ; }
   Real_t ss4o3() const               { return m_ss4o3 ; }
   Real_t getLinearQCoefficient() const           { return linearQCoefficient ; }
   Real_t getQuadraticQCoefficient() const           { return quadraticQCoefficient ; }
   Real_t qqc() const                 { return m_qqc ; }

   Real_t eosvmax() const             { return m_eosvmax ; }
   Real_t eosvmin() const             { return m_eosvmin ; }
   Real_t getPressureFloor() const                { return pressureFloor ; }
   Real_t getEnergyFloor() const                { return energyFloor ; }
   Real_t getMaxAllowedVolumeChange() const             { return maxAllowedVolumeChange ; }
   Real_t getReferenceDensity() const             { return referenceDensity ; }

   // Timestep controls, etc...
   Real_t& getCurrentTime()                 { return currentTime ; }
   Real_t& getDeltaTime()            { return deltaTime ; }
   Real_t& deltatimemultlb()      { return m_deltatimemultlb ; }
   Real_t& deltatimemultub()      { return m_deltatimemultub ; }
   Real_t& getStopTime()             { return stopTime ; }
   Real_t& getCourantConstraint()            { return courantConstraint ; }
   Real_t& getVolumeChangeConstraint()              { return volumeChangeConstraint ; }
   Real_t& getMaxAllowableTimeIncrement()                { return maxAllowableTimeIncrement ; }
   Real_t& getFixedTimeIncrement()              { return fixedTimeIncrement ; }

   Int_t&  getIterationCount()                { return iterationCount ; }
   Index_t&  getNumberOfRanks()           { return numberOfRanks ; }

   Index_t&  colLoc()             { return m_colLoc ; }
   Index_t&  rowLoc()             { return m_rowLoc ; }
   Index_t&  planeLoc()           { return m_planeLoc ; }
   Index_t&  tp()                 { return m_tp ; }

   Index_t&  sizeX()              { return m_sizeX ; }
   Index_t&  sizeY()              { return m_sizeY ; }
   Index_t&  sizeZ()              { return m_sizeZ ; }
   Index_t&  getNumberOfRegions()             { return numberOfRegions ; }
   Int_t&  getImbalanceCost()             { return imbalanceCost ; }
   Index_t&  getNumberOfElements()            { return numberOfElements ; }
   Index_t&  getNumberOfNodes()            { return numberOfNodes ; }
   
   Index_t&  getMaxPlaneSize()       { return maxPlaneSize ; }
   Index_t&  getMaxEdgeSize()        { return maxEdgeSize ; }
   
   //
   // MPI-Related additional data
   //

#if USE_MPI   
   // Communication Work space 
   Real_t *commDataSend ;
   Real_t *commDataRecv ;
   
   // Maximum number of block neighbors 
   MPI_Request recvRequest[26] ; // 6 faces + 12 edges + 8 corners 
   MPI_Request sendRequest[26] ; // 6 faces + 12 edges + 8 corners 
#endif

  private:

   void BuildMesh(Int_t nx, Int_t edgeNodes, Int_t edgeElems);
   void SetupThreadSupportStructures();
   void CreateRegionIndexSets(Int_t nreg, Int_t balance);
   void SetupCommBuffers(Int_t edgeNodes);
   void SetupSymmetryPlanes(Int_t edgeNodes);
   void SetupElementConnectivities(Int_t edgeElems);
   void SetupBoundaryConditions(Int_t edgeElems);

   //
   // IMPLEMENTATION
   //

   /* Node-centered */
   std::vector<Real_t> nodalCoordinateX ;  /* coordinates */
   std::vector<Real_t> nodalCoordinateY ;
   std::vector<Real_t> nodalCoordinateZ ;

   std::vector<Real_t> nodalVelocityX ; /* velocities */
   std::vector<Real_t> nodalVelocityY ;
   std::vector<Real_t> nodalVelocityZ ;

   std::vector<Real_t> nodalAccelerationX ; /* accelerations */
   std::vector<Real_t> nodalAccelerationY ;
   std::vector<Real_t> nodalAccelerationZ ;

   std::vector<Real_t> nodalForceX ;  /* forces */
   std::vector<Real_t> nodalForceY ;
   std::vector<Real_t> nodalForceZ ;

   std::vector<Real_t> nodalMass ;  /* mass */

   std::vector<Index_t> symmetryPlaneNodesetX ;  /* symmetry plane nodesets */
   std::vector<Index_t> symmetryPlaneNodesetY ;
   std::vector<Index_t> symmetryPlaneNodesetZ ;

   // Element-centered

   // Region information
   Int_t    numberOfRegions ;
   Int_t    imbalanceCost; //imbalance cost
   Index_t *regionSetSizes ;   // Size of region sets
   Index_t *regionNumberPerElement ;    // Region number per domain element
   Index_t **regionIndexset ;  // region indexset 

   std::vector<Index_t>  nodeConnectivityList ;     /* elemToNode connectivity */

   std::vector<Index_t>  m_lxim ;  /* element connectivity across each face */
   std::vector<Index_t>  m_lxip ;
   std::vector<Index_t>  m_letam ;
   std::vector<Index_t>  m_letap ;
   std::vector<Index_t>  m_lzetam ;
   std::vector<Index_t>  m_lzetap ;

   std::vector<Int_t>    m_elemBC ;  /* symmetry/free-surface flags for each elem face */

   Real_t             *principalStrainsX ;  /* principal strains -- temporary */
   Real_t             *principalStrainsY ;
   Real_t             *principalStrainsZ ;

   Real_t             *m_delv_xi ;    /* velocity gradient -- temporary */
   Real_t             *m_delv_eta ;
   Real_t             *m_delv_zeta ;

   Real_t             *m_delx_xi ;    /* coordinate gradient -- temporary */
   Real_t             *m_delx_eta ;
   Real_t             *m_delx_zeta ;
   
   std::vector<Real_t> energy ;   /* energy */

   std::vector<Real_t> pressure ;   /* pressure */
   std::vector<Real_t> artificialViscosity ;   /* q */
   std::vector<Real_t> qLinearTerm ;  /* linear term for q */
   std::vector<Real_t> qQuadraticTerm ;  /* quadratic term for q */

   std::vector<Real_t> relativeVolume ;     /* relative volume */
   std::vector<Real_t> referenceVolume ;  /* reference volume */
   std::vector<Real_t> m_vnew ;  /* new relative volume -- temporary */
   std::vector<Real_t> m_delv ;  /* m_vnew - m_v */
   std::vector<Real_t> m_vdov ;  /* volume derivative over volume */

   std::vector<Real_t> elementCharacteristicLength ;  /* characteristic length of an element */
   
   std::vector<Real_t> soundSpeed ;      /* "sound speed" */

   std::vector<Real_t> m_elemMass ;  /* mass */

   // Cutoffs (treat as constants)
   const Real_t  energyToleranceCutoff ;             // energy tolerance 
   const Real_t  pressureToleranceCutoff ;             // pressure tolerance
   const Real_t  qToleranceCutoff ;             // q tolerance
   const Real_t  relativeVolumeToleranceCutoff ;             // relative volume tolerance 
   const Real_t  velocityToleranceCutoff ;             // velocity tolerance 

   // Other constants (usually setable, but hardcoded in this proxy app)

   const Real_t  hourglassControlCoefficient ;            // hourglass control
   const Real_t  m_ss4o3 ;
   const Real_t  excessiveQIndicator ;             // excessive q indicator 
   const Real_t  m_monoq_max_slope ;
   const Real_t  m_monoq_limiter_mult ;
   const Real_t  linearQCoefficient ;         // linear term coef for q 
   const Real_t  quadraticQCoefficient ;         // quadratic term coef for q 
   const Real_t  m_qqc ;
   const Real_t  m_eosvmax ;
   const Real_t  m_eosvmin ;
   const Real_t  pressureFloor ;              // pressure floor 
   const Real_t  energyFloor ;              // energy floor 
   const Real_t  maxAllowedVolumeChange ;           // maximum allowable volume change 
   const Real_t  referenceDensity ;           // reference density

   // Variables to keep track of timestep, simulation time, and cycle
   Real_t  courantConstraint ;         // courant constraint 
   Real_t  volumeChangeConstraint ;           // volume change constraint 
   Int_t   iterationCount ;             // iteration count for simulation 
   Real_t  fixedTimeIncrement ;           // fixed time increment 
   Real_t  currentTime ;              // current time 
   Real_t  deltaTime ;         // variable time increment 
   Real_t  m_deltatimemultlb ;
   Real_t  m_deltatimemultub ;
   Real_t  maxAllowableTimeIncrement ;             // maximum allowable time increment 
   Real_t  stopTime ;          // end time for simulation 


   Int_t   numberOfRanks ;

   Index_t m_colLoc ;
   Index_t m_rowLoc ;
   Index_t m_planeLoc ;
   Index_t m_tp ;

   Index_t m_sizeX ;
   Index_t m_sizeY ;
   Index_t m_sizeZ ;
   Index_t numberOfElements ;
   Index_t numberOfNodes ;

   Index_t maxPlaneSize ;
   Index_t maxEdgeSize ;

   // OMP hack 
   Index_t *m_nodeElemStart ;
   Index_t *m_nodeElemCornerList ;

   // Used in setup
   Index_t m_rowMin, m_rowMax;
   Index_t m_colMin, m_colMax;
   Index_t m_planeMin, m_planeMax ;

} ;

typedef Real_t &(Domain::* Domain_member )(Index_t) ;

struct cmdLineOpts {
   Int_t its; // -i 
   Int_t nx;  // -s 
   Int_t numReg; // -r 
   Int_t numFiles; // -f
   Int_t showProg; // -p
   Int_t quiet; // -q
   Int_t viz; // -v 
   Int_t cost; // -c
   Int_t balance; // -b
};



// Function Prototypes

// lulesh-par
Real_t CalcElemVolume( const Real_t x[8],
                       const Real_t y[8],
                       const Real_t z[8]);

// lulesh-util
void ParseCommandLineOptions(int argc, char *argv[],
                             Int_t myRank, struct cmdLineOpts *opts);
void VerifyAndWriteFinalOutput(Real_t elapsed_time,
                               Domain& locDom,
                               Int_t nx,
                               Int_t numRanks);

// lulesh-viz
void DumpToVisit(Domain& domain, int numFiles, int myRank, int numRanks);

// lulesh-comm
void CommRecv(Domain& domain, Int_t msgType, Index_t xferFields,
              Index_t dx, Index_t dy, Index_t dz,
              bool doRecv, bool planeOnly);
void CommSend(Domain& domain, Int_t msgType,
              Index_t xferFields, Domain_member *fieldData,
              Index_t dx, Index_t dy, Index_t dz,
              bool doSend, bool planeOnly);
void CommSBN(Domain& domain, Int_t xferFields, Domain_member *fieldData);
void CommSyncPosVel(Domain& domain);
void CommMonoQ(Domain& domain);

// lulesh-init
void InitMeshDecomp(Int_t numRanks, Int_t myRank,
                    Int_t *col, Int_t *row, Int_t *plane, Int_t *side);
