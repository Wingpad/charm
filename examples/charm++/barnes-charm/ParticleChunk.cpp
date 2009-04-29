#include "barnes.h"

//CkReduction::reducerType chunksToPiecesReducerType;

#include "barnes.decl.h"
ParticleChunk::ParticleChunk(int mleaf, int mcell){

  /*allocate leaf/cell space */
  //int NPROC = numchunks;
  // j: don't need these data structures
  //ctab = (cellptr) G_MALLOC((maxcell / NPROC) * sizeof(cell));
  //ltab = (leafptr) G_MALLOC((maxleaf / NPROC) * sizeof(leaf));

  nstep = 0;
  numMsgsToEachTp = new int[numTreePieces];
  particlesToTps.resize(numTreePieces);
  for(int i = 0; i < numTreePieces; i++){
    numMsgsToEachTp[i] = 0;
    particlesToTps[i].reserve(MAX_PARTICLES_PER_MSG);
  }

};

void ParticleChunk::SlaveStart(CmiUInt8 bodyptrstart_, CmiUInt8 bodystart_, CkCallback &cb_){
//void ParticleChunk::SlaveStart(CmiUInt8 bodyptrstart_, CmiUInt8 bodystart_, CmiUInt8 mct_, CmiUInt8 mlt_, CkCallback &cb_){
  unsigned int ProcessId;

  /* Get unique ProcessId */
  ProcessId = thisIndex;

  bodyptr *bodyptrstart = (bodyptr *)bodyptrstart_;
  bodyptr bodystart = (bodyptr)bodystart_;
  /*
  cellptr *cellptrstart = (cellptr *)mct_;
  leafptr *leafptrstart = (leafptr *)mlt_;
  */


  /* POSSIBLE ENHANCEMENT:  Here is where one might pin processes to
     processors to avoid migration */

  /* initialize mybodytabs */
  mybodytab = bodyptrstart + (maxmybody * ProcessId);
  bodytab = bodystart;

  /*
  mycelltab = cellstart + (maxmycell * ProcessId);
  myleaftab = leafstart + (maxmyleaf * ProcessId);
  */
  /* note that every process has its own copy   */
  /* of mybodytab, which was initialized to the */
  /* beginning of the whole array by proc. 0    */
  /* before create                              */
  /* POSSIBLE ENHANCEMENT:  Here is where one might distribute the
     data across physically distributed memories as desired. 

     One way to do this is as follows:

     int i;

     if (ProcessId == 0) {
     for (i=0;i<NPROC;i++) {
     Place all addresses x such that 
     &(Local[i]) <= x < &(Local[i])+
     sizeof(struct local_memory) on node i
     Place all addresses x such that 
     &(Local[i].mybodytab) <= x < &(Local[i].mybodytab)+
     maxmybody * sizeof(bodyptr) - 1 on node i
     Place all addresses x such that 
     &(Local[i].mycelltab) <= x < &(Local[i].mycelltab)+
     maxmycell * sizeof(cellptr) - 1 on node i
     Place all addresses x such that 
     &(Local[i].myleaftab) <= x < &(Local[i].myleaftab)+
     maxmyleaf * sizeof(leafptr) - 1 on node i
     }
     }

     barrier(Global->Barstart,NPROC);

*/

  find_my_initial_bodies(bodytab, nbody, ProcessId);

  contribute(0,0,CkReduction::concat,cb_);
}

/* 
 * FIND_MY_INITIAL_BODIES: puts into mybodytab the initial list of bodies 
 * assigned to the processor.  
 */

void ParticleChunk::find_my_initial_bodies(bodyptr btab, int nbody, unsigned int ProcessId)
/*
bodyptr btab;
int nbody;
unsigned int ProcessId;
*/
{
  int Myindex;
  int equalbodies;
  int extra,offset,i;

  int NPROC = numParticleChunks;

  mynbody = nbody / NPROC;
  extra = nbody % NPROC;
  if (ProcessId < extra) {
    mynbody++;    
    offset = mynbody * ProcessId;
  }
  if (ProcessId >= extra) {
    offset = (mynbody+1)*extra + (ProcessId - extra)*mynbody; 
  }
  for (i=0; i < mynbody; i++) {
     mybodytab[i] = &(btab[offset+i]);
     //CkPrintf("[%d] particle %d: 0x%x\n", thisIndex, i, mybodytab[i]);
  }
  //BARRIER(Global->Barstart,NPROC);
}

void ParticleChunk::acceptRoot(CmiUInt8 root_, CkCallback &mainCb_){
  mainCb = mainCb_;
  G_root = (cellptr) root_;
  CkPrintf("[%d] acceptRoot 0x%x\n", thisIndex, G_root);
  CkCallback cb(CkIndex_ParticleChunk::stepsystemPartII(0), thisProxy);
  contribute(0,0,CkReduction::concat,cb);
}

void ParticleChunk::partition(CkCallback &cb){
  int ProcessId = thisIndex;
  HouseKeep(); 
  real Cavg = (real) Cost(G_root) / (real)NPROC ;
  workMin = (int) (Cavg * ProcessId);
  workMax = (int) (Cavg * (ProcessId + 1) + (ProcessId == (NPROC - 1)));
  CkPrintf("[%d] cost(root) = %f, workMin: %f, workMax: %f\n", ProcessId, Cavg, workMin, workMax);

  mynbody = 0;
  // find_my_bodies(Global->G_root, 0, BRC_FUC, ProcessId );
  find_my_bodies((nodeptr)G_root, 0, BRC_FUC, ProcessId);
  contribute(0,0,CkReduction::concat,cb);
}

void ParticleChunk::HouseKeep(){
  myn2bcalc = mynbccalc = myselfint = 0;
}

void ParticleChunk::find_my_bodies(nodeptr mycell, int work, int direction, unsigned ProcessId){
  int i;
  leafptr l;
  nodeptr qptr;

  if (Type(mycell) == LEAF) {
    l = (leafptr) mycell;      
    for (i = 0; i < l->num_bodies; i++) {                                                            
      if (work >= workMin-.1) {                                                  
        if((mynbody) > maxmybody) {                                             
          CkPrintf("[%d] find_my_bodies: needs more than %d bodies; increase fleaves. mynbody: %d\n",ProcessId, maxmybody, mynbody); 
        }    
        mybodytab[mynbody++] = Bodyp(l)[i];
      }                                                                                             
      work += Cost(Bodyp(l)[i]);                                                                    
      if (work >= workMax-.1) {                                                    
        break;
      }                                                                                             
    }                                                                                                
  }
  else {
    for(i = 0; (i < NSUB) && (work < (workMax - .1)); i++){                         
      qptr = Subp(mycell)[Child_Sequence[direction][i]];                                            
      if (qptr!=NULL) {                                                                             
        if ((work+Cost(qptr)) >= (workMin -.1)) {                                 
          find_my_bodies(qptr,work, Direction_Sequence[direction][i],                             
              ProcessId);                                                              
        }
        work += Cost(qptr);                                                                        
      } 
    }
  }  
}

void ParticleChunk::ComputeForces (CkCallback &cb)
{
   bodyptr p,*pp;
   vector acc1, dacc, dvel, vel1, dpos;
   unsigned ProcessId = thisIndex;

   for (pp = mybodytab; pp < mybodytab+mynbody; pp++) {  
      p = *pp;
      SETV(acc1, Acc(p));
      Cost(p)=0;
      hackgrav(p,ProcessId);
      myn2bcalc += myn2bterm; 
      mynbccalc += mynbcterm;
      if (skipself) {       /*   did we miss self-int?  */
	 myselfint++;        /*   count another goofup   */
      }
      if (nstep > 0) {
	 /*   use change in accel to make 2nd order correction to vel      */
	 SUBV(dacc, Acc(p), acc1);
	 MULVS(dvel, dacc, dthf);
	 ADDV(Vel(p), Vel(p), dvel);
      }
   }

   contribute(0,0,CkReduction::concat,cb);
}

void ParticleChunk::stepsystemPartII(CkReductionMsg *msg){

    delete msg;

    unsigned int ProcessId = thisIndex;

    if ((ProcessId == 0) && (nstep >= 2)) {
        //CLOCK(treebuildstart);
    }

    /* load bodies into tree   */
    maketree(ProcessId);
    flushParticles();
    doneSendingParticles();

    if ((ProcessId == 0) && (nstep >= 2)) {
        //CLOCK(treebuildend);
        //Global->treebuildtime += treebuildend - treebuildstart;
    }
}

/*
 * MAKETREE: initialize tree structure for hack force calculation.
 */

void ParticleChunk::maketree(unsigned int ProcessId)
{
   bodyptr p, *pp;

   // j: don't need these 
   //myncell = 0;
   //mynleaf = 0;
   if (ProcessId == 0) {
      //mycelltab[myncell++] = G_root; 
   }
   Current_Root = (nodeptr) G_root;
   for (pp = mybodytab; pp < mybodytab+mynbody; pp++) {
      p = *pp;
      if (Mass(p) != 0.0) {
	 Current_Root = (nodeptr) loadtree(p, (cellptr) Current_Root, ProcessId);
      }
      else {
	 fprintf(stderr, "Process %d found body 0x%x to have zero mass\n",
		 ProcessId, p);	
      }
   }
}

/*
 * LOADTREE: descend tree and insert particle.
 */

nodeptr
ParticleChunk::loadtree(bodyptr p, cellptr root, unsigned ProcessId)
   //bodyptr p;                        /* body to load into tree */
   //cellptr root;
   //unsigned ProcessId;
{
   int l, xq[NDIM], xp[NDIM], flag;
   int i, j, root_level;
   bool valid_root;
   int kidIndex;
   nodeptr *qptr, mynode;
   cellptr c;
   leafptr le;

   CkAssert(intcoord(xp, Pos(p)));
   root = G_root;
   mynode = (nodeptr) root;
   kidIndex = subindex(xp, Level(mynode));
   //qptr = &Subp(mynode)[kidIndex];

   l = Level(mynode) >> 1;

   int depth = log8floor(numTreePieces);
   int lowestLevel = Level(mynode) >> depth;
   int fact = NSUB/2;
   int whichTp = 0;
   int d = depth;

   for(int level = Level(mynode); level > lowestLevel; level >>= 1){
     kidIndex = subindex(xp, Level(mynode));
     mynode = Subp(mynode)[kidIndex];
     whichTp += kidIndex*(1<<(fact*(d-1)));
     //CkPrintf("Particle (%f,%f,%f) -> (%d,%d,%d) : %d\n", Pos(p)[0], Pos(p)[1], Pos(p)[2], xp[0], xp[1], xp[2], kidIndex);
     d--;     
   }
   //ckout << "which TP: " << whichTp << endl;

   int howMany = particlesToTps[whichTp].push_back_v(p); 
   if(howMany == MAX_PARTICLES_PER_MSG-1){ // enough particles to send 
     sendParticlesToTp(whichTp);
   }
}

void ParticleChunk::flushParticles(){
  // send any remaining particles to their 
  // intended host treepieces
  for(int tp = 0; tp < numTreePieces; tp++){
    sendParticlesToTp(tp); 
  }
}

void ParticleChunk::sendParticlesToTp(int tp){
  int len = particlesToTps[tp].length();
  if(len > 0){
    CkPrintf("[%d] sending %d particles to piece %d\n", thisIndex, len, tp);
    ParticleMsg *msg = new (len) ParticleMsg(); 
    memcpy(msg->particles, particlesToTps[tp].getVec(), len*sizeof(bodyptr));
    /*
    for(int i = 0; i < len; i++){
      CkPrintf("[%d] 0x%x\n", thisIndex, msg->particles[i]);
    }
    */
    msg->num = len; 
    numMsgsToEachTp[tp]++;
    particlesToTps[tp].length() = 0;
    pieces[tp].recvParticles(msg);
  }
}

void ParticleChunk::doneSendingParticles(){
  // send counts to chunk 0
  CkCallback cb(CkIndex_TreePiece::recvTotalMsgCountsFromChunks(0), pieces);
  contribute(numTreePieces*sizeof(int), numMsgsToEachTp, CkReduction::sum_int, cb);   
}

void ParticleChunk::doneTreeBuild(){
  CkPrintf("[%d] all pieces have completed buildTree()\n", thisIndex);
  mainCb.send();
}

void ParticleChunk::advance(){
  /* advance my bodies */
  vector max, min;
  SETVS(min,1E99);
  SETVS(max,-1E99);

  for (pp = mybodytab; pp < mybodytab+mynbody; pp++) {
    p = *pp;
    MULVS(dvel, Acc(p), dthf);
    ADDV(vel1, Vel(p), dvel);
    MULVS(dpos, vel1, dtime);
    ADDV(Pos(p), Pos(p), dpos);
    ADDV(Vel(p), vel1, dvel);

    for (i = 0; i < NDIM; i++) {
      if (Pos(p)[i]<min[i]) {
        min[i]=Pos(p)[i];
      }
      if (Pos(p)[i]>max[i]) {
        max[i]=Pos(p)[i] ;
      }
    }
  }

  /*
  LOCK(Global->CountLock);
  for (i = 0; i < NDIM; i++) {
    if (Global->min[i] > min[i]) {
      Global->min[i] = min[i];
    }
    if (Global->max[i] < max[i]) {
      Global->max[i] = max[i];
    }
  }
  UNLOCK(Global->CountLock);
  */

  // FIXME - start here; contribute max,min to global max and min

  /* bar needed to make sure that every process has computed its min */
  /* and max coordinates, and has accumulated them into the global   */
  /* min and max, before the new dimensions are computed             */
  BARRIER(Global->Barpos,NPROC);

  /*
  if ((ProcessId == 0) && (nstep >= 2)) {
    CLOCK(trackend);
    Global->tracktime += trackend - trackstart;
  }
  */
  if (ProcessId==0) {
    Global->rsize=0;
    SUBV(Global->max,Global->max,Global->min);
    for (i = 0; i < NDIM; i++) {
      if (Global->rsize < Global->max[i]) {
        Global->rsize = Global->max[i];
      }
    }
    ADDVS(Global->rmin,Global->min,-Global->rsize/100000.0);
    Global->rsize = 1.00002*Global->rsize;
    SETVS(Global->min,1E99);
    SETVS(Global->max,-1E99);
  }
  nstep++;
  tnow = tnow + dtime;
}
