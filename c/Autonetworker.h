#ifndef _AUTONETWORKER_H_
#define _AUTONETWORKER_H_

#include <vector>

#include "AutonetworkerDBModel.h"
#include "AutonetworkerLog.h"
#include "DualLane.H"
#include "Results.H"
#include "BackingData.H"
#include "cfg_class.h"

class Autonetworker;

typedef AlgoLevel1 *(*create_l1_func)();
typedef AlgoLevel2 *(*create_l2_func)();

typedef bool (Autonetworker::*AnetMemFn)();
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember)) 

typedef enum { eSuggestedCodes, eRepairCodes } eRepairCodeType;

class Autonetworker {

 public:

  Autonetworker();
  ~Autonetworker();

  bool connect(); // Use login credentials from configuration.
  bool connect( const std::string& host, const std::string& user, const std::string& pass, const std::string& db );

  AutonetworkerDBModel* get_dbmodel(); // used by AnetTransaction.

  bool save_inspection();
  bool save_repair();

  bool erase_all(); // Erase ALL data in the connected database. Use with care.
  bool erase( const u_int retain ); // Retain certain number inspection after erase. 
  bool do_erase_all();
  bool do_erase();

  // Just some setters...
  void repair_file( const std::string& fname ); // Call before save_repair()!!!
  void lane( lane_type_t );
  void centi_timestamp( const std::string& );

  long save_duration(); // How long did the save take?
  double db_size(); // in MB

  // This is for debug.  If saving the same inspection over and over
  // this will apply a random fudge to each value.  This is so we have
  // meaningful values for graphs, means, std deviations etc...
  void debug( bool );
  bool debug();

  bool rand_fudge( bool );
  bool db_create(); // execute CREATE TABLE... file from disk somewhere.

  static const std::string db_enum_to_str( eDatabaseTypes );
  
 private:
  
  bool _debug;

  AutonetworkerDBModel* _dbmodel;
  AutonetworkerLog* _log;
  lane_type_t _lane;
  std::string _centi_timestamp;
  bool _rand_fudge;
  long _save_duration;

  // refname derived panel number mapped to barcode for the panel.
  std::map<u_int, std::string> _panel_bc_map;

  // refname derived panel number we should skip (e.g. skip mark fails).
  std::vector<u_int> _skip_fail_list;
  std::vector<u_int> _fid_fail_list;

  // Name/value pairs from /etc/opt/mvp/syspar.d/defaults.res.
  std::map<std::string, std::string> _cfg_map;

  // Members for saving repair data.
  std::string _repair_file;
  Inspection  _insp;
  HeaderData  _hdr_data;
  RepairData  _repair_data;

  // DB Params for convenience.
  std::string _host;
  std::string _user;
  std::string _pass;
  std::string _db;

  // Call function within db transaction.
  bool trans( AnetMemFn );

  // Save inspection.
  bool do_preprocess();
  bool do_save();
  bool save_recipe_meta();
  bool save_algo_meta( eLevel algo_level );
  bool save_component_meta();
  bool save_insp_hdr();
  bool save_component_results();
  bool save_panel_results();
  bool save_cfg();

  // Save repair.
  bool do_save_repair();
  bool load_repair_file();
  bool save_repair_hdr();
  bool save_component_repair();
  bool repair_codes( const GroupRepairData&, eRepairCodeType, std::string& , std::string &);
  bool repair_codes( const ItemRepairData&, eRepairCodeType, std::string& , std::string &);

  bool res2str( Results*, std::string& );
  bool get_op_name( std::string& );
  bool iv_names( const std::string&, eLevel, std::vector<std::string>& );
  bool iv_names_l1( const std::string&, std::vector<std::string>& );
  bool iv_names_l2( const std::string&, std::vector<std::string>& );
  bool keep_this_group( const Group* );
  bool keep_this_item( const Item* );
  bool is_gonogo( const Group* );
  bool is_skip_group( const Group* Dg );
  bool is_fid_group( const Group* Dg );

  // Case-insensitive substring match.
  bool substr_match( const std::string& needle, const std::string& haystack, bool ignore_case );

  bool err( const std::string& );
  std::string lane();
  
  void timer_start( struct timeval* start_time );
  long timer_stop( struct timeval *start_time );
};

#endif

