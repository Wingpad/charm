#ifndef _REFINERAPPROX_H_
#define _REFINERAPPROX_H_

#include "CentralLB.h"

#include "Refiner.h"
#include "elements.h"
#include "heap.h"

#ifndef INFTY
#define INFTY 999999
#endif

class RefinerApprox:public Refiner {
public:
  RefinerApprox(double _overload): Refiner(_overload) { 
    overLoad = _overload; computes=0; processors=0; 
  };
  ~RefinerApprox() {};
  void Refine(int count, CentralLB::LDStats* stats, int* cur_p, int* new_p);

protected:
  void create(int count, CentralLB::LDStats* stats, int* cur_p);
  void reinitAssignment();
  virtual int refine(double opt);
  void multirefine(int num_moves);
  int getNumLargeComputes(double opt);
  double getLargestCompute();
  int computeA(processorInfo *p,double opt);
  int computeB(processorInfo *p,double opt);
  int numMoves();
  void printStats(int newStats);
  Set * removeBiggestSmallComputes(int num,processorInfo * p,double opt);
  Set * removeBigComputes(int num,processorInfo * p,double opt);
};

#endif /* _REFINERAPPROX_H_ */


/*@}*/
