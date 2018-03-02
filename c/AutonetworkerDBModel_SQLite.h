#ifndef _ANET_DB_MODEL_SQLITE_H_
#define _ANET_DB_MODEL_SQLITE_H_

#include <string>
#include <deque>
#include <map>

#include "sqlite3pp.h"
#include "AutonetworkerDBModel.h"

class AutonetworkerDBModel_SQLite: public AutonetworkerDBModel {
  
 public: 
  
  AutonetworkerDBModel_SQLite(); 
  virtual ~AutonetworkerDBModel_SQLite();

  bool connect( const std::string&, const std::string&, const std::string&, const std::string& );

  bool transaction_begin();
  bool transaction_commit();
  bool transaction_rollback();

  bool num_insp( const std::string&, u_int& );
  bool db_size( const std::string&, double& ); // In MB.
  bool erase_all();
  bool erase( const u_int retain, const std::string& machine );
  bool db_create();

 protected:

  sqlite3pp::database* _db;
  sqlite3pp::query* _qry;
  sqlite3pp::transaction* _trn;
  sqlite3pp::query::iterator _qry_begin;
  std::deque<u_int> _loaded_grp_ids;
  std::deque<u_int> _loaded_itm_ids;
  bool isConnected;

  bool sql_select( const std::string& );
  bool sql_select_single_val( const std::string&, std::string& );
  bool sql_query( const std::string& );
  bool sql_query_return_id( const std::string&, int& );
  
  bool get_recipe_id( const std::string&, const std::string&, int& );
  bool get_algo_type_id( const std::string&, eLevel level, int& );
  bool get_next_id( eLevel, int& );
  bool get_group_id( const std::string&, int& );
  bool load_ids( eLevel );

  bool load_cfg_desc( std::map< std::string, u_int >& ret_map );
  const std::string sqlite_result_str( const int result_code );
  void sqlite_err( const int code );
  bool check_journal( const std::string dbname );

};

#endif
