#ifndef _ANET_TRANSACTION_H_
#define _ANET_TRANSACTION_H_

#include "Autonetworker.h"
#include "AutonetworkerDBModel.h"

class AnetTransaction 
{
 public:
  AnetTransaction ( Autonetworker*, AutonetworkerLog* );
  virtual ~AnetTransaction();
  void begin();
  void commit();
  
 private:
  bool                  _try_commit;
  Autonetworker*        _anet;
  AutonetworkerDBModel* _dbmodel;
  AutonetworkerLog*     _log;
};

#endif
