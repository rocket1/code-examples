#include "AnetException.h"
#include "AnetTransaction.h"

void
AnetTransaction::commit()
{
  _try_commit = true;

  if (!_dbmodel->transaction_commit()) {
    throw AnetException("Failed Transaction Commit in AnetTransaction::~AnetTransaction ().");
  }
}

AnetTransaction::AnetTransaction ( Autonetworker* anet, AutonetworkerLog* log )
{
  if (!anet) {
    throw AnetException("Autonetworker was null in AnetTransaction::AnetTransaction().");
  }

  if (!log) {
    throw AnetException("Log was null in AnetTransaction::AnetTransaction().");
  }

  _try_commit = false;

  _anet       = anet;
  _dbmodel    = _anet->get_dbmodel();
  _log        = log;

  if (!_dbmodel) {
    throw AnetException( "Couldn't get the DB Model in AnetTransaction::AnetTransaction()." );
  }
}

void
AnetTransaction::begin()
{
  if (_anet->debug()) {
    std::cerr << "AnetTransaction::begin()\n";
  }

  if (!_anet->connect()) {
    throw AnetExceptionConnFail( "Connect failed in AnetTransaction::begin()." );
  }

  if (!_dbmodel->transaction_begin()) {
    throw AnetException( "Failed starting transaction." );
  }
}

AnetTransaction::~AnetTransaction ()
{

  // Don't attempt Rollback if we tried a sommit already.
  // It will most likely seg fault on Rollback (SQLite at least) if the commit previously failed.

  if ( _dbmodel && !_try_commit ) {

    if (!_dbmodel->transaction_rollback()) {
      //      throw AnetException("Failed Transaction Rollback in AnetTransaction::~AnetTransaction ().");
    }

    _log->write("Transaction Rollback.");
  }
}
    
