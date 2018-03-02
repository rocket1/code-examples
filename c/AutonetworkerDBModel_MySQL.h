#ifndef _ANET_DB_MODEL_MYSQL_H_
#define _ANET_DB_MODEL_MYSQL_H_

#include <string>
#include <vector>
#include <map>
#include <mysql/mysql.h>
#include "AnetUtil.h"
#include "AutonetworkerDBModel.h"
#include "AutonetworkerLog.h"

class AutonetworkerDBModel_MySQL : public AutonetworkerDBModel {
  
 public: 
  
  AutonetworkerDBModel_MySQL(); 
  virtual ~AutonetworkerDBModel_MySQL();

  bool connect( const std::string&, const std::string&, const std::string&, const std::string& );
  bool reset_conn();

  bool transaction_begin();
  bool transaction_commit();
  bool transaction_rollback();

  bool num_insp( const std::string&, u_int& );
  bool db_size( const std::string&, double& ); // In MB.

  bool erase_all();
  bool erase( const u_int retain, const std::string& machine );
  bool db_create( const std::string dbname );

  // MySQL only...
  void use_static_conn( bool );

 private:

  static MYSQL* _conn_static;
  MYSQL* _connection;

  MYSQL  _mysql;

  MYSQL_RES* _result;
  MYSQL_RES* _group_meta_result;
  MYSQL_RES* _item_meta_result;
  MYSQL_ROW  _row;

  bool _use_static_conn;

  bool sql_select( const std::string& );
  bool sql_select_single_val( const std::string&, std::string& );
  bool do_query( const std::string& ); // Does the actual mysql_query().
  bool sql_query( const std::string& );
  bool sql_query_return_id( const std::string&, int& );
  
  bool get_recipe_id( const std::string&, const std::string&, int& );
  bool get_algo_type_id( const std::string&, eLevel level, int& );
  bool get_next_id( eLevel, int& );
  bool get_group_id( const std::string&, int& );
  bool load_ids( eLevel );

  bool load_cfg_desc( std::map< std::string, u_int >& ret_map );
};

#endif
