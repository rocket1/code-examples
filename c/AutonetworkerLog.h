#ifndef _AUTONETWORKER_LOG_H_
#define _AUTONETWORKER_LOG_H_

#include <fstream>
#include "Elog.H"

typedef u_int ANET_DEBUG_LEVEL;

const ANET_DEBUG_LEVEL ANET_LOG         = 1; // dump to log only.
const ANET_DEBUG_LEVEL ANET_DEBUG       = 2; // dump to console only.
const ANET_DEBUG_LEVEL ANET_VERBOSE     = ANET_DEBUG | ANET_LOG; // dump to console AND log file.

class AutonetworkerLog 
{
 public:

  static AutonetworkerLog* get();
  ~AutonetworkerLog();
  
  void write( const std::string& );
  void write( const std::string&, ANET_DEBUG_LEVEL );
  bool set_log_file( const std::string& );

 private:

  AutonetworkerLog();

  Elog* _elog;
  std::ofstream _out;
  std::string _anet_logfile;
  ANET_DEBUG_LEVEL _debug_level;

  std::string get_timestamp();
};

#endif
