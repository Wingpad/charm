/*****************************************************************************
 * $Source$
 * $Author$
 * $Date$
 * $Revision$
 *****************************************************************************/

/**
 * \addtogroup CkLdb
*/
/*@{*/

#ifndef LBCOMM_H
#define LBCOMM_H

#include "converse.h"
#include "lbdb.h"

// point to point communication data
class LBCommData {

friend class LBCommTable;

public:
  LBCommData(int _src_proc, LDOMid _destOM, LDObjid _destObj, int _destObjProc) {
    src_proc = _src_proc;
    destObj.init_objmsg(_destOM, _destObj, _destObjProc);
    n_messages = 0;
    n_bytes = 0;
    mykey = compute_key();
  };

  LBCommData(LDObjHandle _srcObj, LDOMid _destOM, LDObjid _destObj, int _destObjProc) {
    src_proc = -1;
    srcObj = _srcObj;
    destObj.init_objmsg(_destOM, _destObj, _destObjProc);
    n_messages = 0;
    n_bytes = 0;
    mykey = compute_key();
  };

  LBCommData(const LBCommData& d) {
    src_proc = d.src_proc;
    if (!from_proc()) {
      srcObj = d.srcObj;
//      srcOM = d.srcOM;
    }
    destObj = d.destObj;
    n_messages = d.n_messages;
    n_bytes = d.n_bytes;
    mykey = d.mykey;
  };

  ~LBCommData() { };

  LBCommData& operator=(const LBCommData& d) {
    src_proc = d.src_proc;
    if (!from_proc()) { 
      srcObj = d.srcObj;
//      srcOM = d.srcOM;
    }
    destObj = d.destObj;
    n_messages = d.n_messages;
    n_bytes = d.n_bytes;
    mykey = d.mykey;
    return *this;
  };

  void addMessage(int bytes) {
    n_messages++;
    n_bytes += bytes;
  };

  inline int key() const { return mykey; };
  CmiBool equal(const LBCommData _d2) const;

  inline int from_proc() const { return (src_proc != -1); }
private:
  LBCommData() {};
  
  int compute_key();
  int hash(const int i, const int m) const;

  int mykey;
  int src_proc;
  LDObjHandle srcObj;
  LDCommDesc   destObj;
  int n_messages;
  int n_bytes;
};

class LBCommTable {
public:

  LBCommTable() {
    NewTable(initial_sz);
  };

  ~LBCommTable() {
    delete [] set;
    delete [] state;
  };

  LBCommData* HashInsert(const LBCommData &data);
  LBCommData* HashInsertUnique(const LBCommData &data);
  LBCommData* HashSearch(const LBCommData &data);
  int CommCount() { return in_use; };
  void GetCommData(LDCommData* data);
	
private:
  void NewTable(int _sz) {
    set = new LBCommData[_sz];
    state = new TableState[_sz];
    cur_sz = _sz;
    in_use = 0;
    for(int i=0; i < _sz; i++)
      state[i] = nil;
  };
  
  void Resize();

  enum { initial_sz = 10000 };
  enum TableState { nil, InUse } ;
  LBCommData* set;
  TableState* state;
  int cur_sz;
  int in_use;
public:
  int useMem() { return cur_sz*(sizeof(LBCommData) + sizeof(TableState)) + sizeof(LBCommTable); }
};


#endif

/*@}*/
