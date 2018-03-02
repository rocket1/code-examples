#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include "AutonetworkerDBModel_MySQL.h"
#include "AnetException.h"

MYSQL* AutonetworkerDBModel_MySQL::_conn_static = NULL;

static const std::string kCreateDBFile("/opt/mvp/script/anet.mysql.sql");

bool
AutonetworkerDBModel_MySQL::load_cfg_desc( std::map< std::string, u_int >& ret_map )
{
  ret_map.clear();
  my_ulonglong row_count = 0;
  
  std::stringstream sql;
  sql << "SELECT id, cfg_name FROM cfg_meta";
  
  if ( !sql_select(sql.str()) ) {
    do_error( "Failed in load_cfg_desc() retrieving cfg data." );
    return false;
  }
  
  row_count = mysql_num_rows(_result);
  
  // It's okay to have no records yet, e.g. very first inspection.
  if( row_count < 1 ) {
    return true;
  }
  
  while ((_row = mysql_fetch_row(_result)) != NULL)
  {
   
    if ( !_row[0] || !_row[1] ) {
      do_error( "Failed in load_cfg_desc() because the 'id' or 'name' was NULL" );
      return false;
    }

    const u_int id = atoi(_row[0]);
    const std::string name(_row[1]);
    ret_map[name] = id;
  }
  
  return true;
}

AutonetworkerDBModel_MySQL::AutonetworkerDBModel_MySQL() : _connection(NULL),
							   _result(NULL),
							   _group_meta_result(NULL),
							   _item_meta_result(NULL),
							   _use_static_conn(false)
							   
{
  _dbtype = "mysql"; // Just for printing.  Not really a "type code".
}

AutonetworkerDBModel_MySQL::~AutonetworkerDBModel_MySQL()
{
  // Close the connection.
  reset_conn();
}

bool
AutonetworkerDBModel_MySQL::connect( const std::string& host,
			       const std::string& user,
			       const std::string& pass,
			       const std::string& db )
{
  /*  if ( mysql_init(&_mysql) == NULL ) {
    do_error( "Couldn't initialize MYSQL object." );
    return false;
    }*/

  _connection = mysql_init(NULL);

  _dbname = db;

  const std::string kEmpty("<empty>");
  const std::string kHidden("<hidden>");
  
  const std::string host_display = host.empty() ? kEmpty : host;
  const std::string user_display = user.empty() ? kEmpty : user;
  const std::string pass_display = pass.empty() ? kEmpty : kHidden;
  const std::string db_display   = db.empty()   ? kEmpty : db;

  std::stringstream info;
  info << "[host: " << host << ", type: MySQL, user: " << user << ", db: " << db << "]"; 

  if ( mysql_real_connect(_connection, host.c_str(), user.c_str(), pass.c_str(), 0, 0, 0, 0) == NULL ) {

    std::stringstream err;
    err << "CONNECT FAILED " << info.str() << " MySQL [" << mysql_errno(_connection) << " " << mysql_error(_connection) << "]";
    do_error(err.str());

    return false;
  }

  std::stringstream connmsg;
  connmsg << "CONNECT " << info.str();
  _log->write(connmsg.str());

  if ( !db_create(_dbname) ) {
    do_error( "db_create() failed in AutonetworkerDBModel_MySQL::connect()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel_MySQL::reset_conn()
{
  if (_connection) {
    mysql_close(_connection);
  }
  _connection = NULL;
  return true;
}

bool
AutonetworkerDBModel_MySQL::do_query( const std::string& query )
{
  if (query.empty()) {
    do_error( "Query was empty in AutonetworkerDBModel::do_query()." );
    return false;
  }

  std::stringstream msg;
  
  switch (mysql_query( _connection, query.c_str())) {

  case 0:
    return true;
    
  case CR_SERVER_GONE_ERROR:
  case CR_SERVER_LOST:
    msg << "Query Failed (BROKEN LINK):" << query << std::endl << "MySQL errno: " << mysql_errno(_connection) << " MySQL msg: " << mysql_error(_connection);
    do_error( std::string("Query Failed (BROKEN LINK): " + msg.str()) );
    throw AnetExceptionBrokenLink("Broken Link.");
    return false;
    
  case CR_UNKNOWN_ERROR:
  default:
    msg << "Query Failed: " << query << std::endl << "MySQL errno: " << mysql_errno(_connection) << " MySQL msg: " << mysql_error(_connection);
    do_error(msg.str());
    return false;
  }
  
  // NEVER REACHED.
  return false;
} 

bool
AutonetworkerDBModel_MySQL::sql_query( const std::string& query )
{
  return do_query(query);
}

bool
AutonetworkerDBModel_MySQL::sql_select( const std::string& query )
{
  return do_query(query) && (_result = mysql_store_result(_connection));
}

bool
AutonetworkerDBModel_MySQL::sql_select_single_val( const std::string& sql, std::string& ret_val )
{
  if ( sql.empty() ) {
    do_error( "SQL was empty in AutonetworkerDBModel::sql_select_single_val()." );
    return false;
  }

  if ( !sql_select(sql) ) {
    do_error( "Query failed in AutonetworkerDBModel::sql_select_single_val()." );
    return false;
  }

  my_ulonglong row_count = mysql_num_rows(_result);

  if ( row_count < 1
       || (_row = mysql_fetch_row(_result)) == NULL
       || !_row[0])
    {
      mysql_free_result(_result);
      return false;
    }

  ret_val = std::string(_row[0]);
  mysql_free_result(_result);
  return true;
}

bool
AutonetworkerDBModel_MySQL::sql_query_return_id( const std::string& query, int& ret_val )
{
  if ( !sql_query(query.c_str()) ) {
    return false;
  }
  
  ret_val = static_cast<int>( mysql_insert_id(_connection) );
  _insert_id = ret_val; // for convenience.
  return true;
}

bool
AutonetworkerDBModel_MySQL::num_insp( const std::string& lot_id, u_int& ret_val )
{
  std::stringstream sql;
  sql << "SELECT COUNT(*) FROM insp_hdr WHERE lot_id='" << lot_id << "'";

  if ( !sql_query(sql.str()) ) {
    do_error( "Query Failed in num_insp()." );
    return false;
  }

  if ( (_result = mysql_store_result(_connection)) == NULL ) {
    do_error( "Failed storing query result in num_insp()." );
    return false;
  }

  if ( (_row = mysql_fetch_row(_result)) == NULL || !_row[0] ) {
    do_error( "Failed getting num insp row in num_insp()." );
    return false;
  }
  
  ret_val = atoi(_row[0]);
  mysql_free_result(_result);
  return true;
}

bool
AutonetworkerDBModel_MySQL::get_recipe_id( const std::string& recipe, const std::string& recipe_mod, int& ret_val )
{
  ret_val = kInvalidId;

  if (recipe.empty()) {
    do_error( "Error in get_recipe_id(). Recipe name was empty." );
    return false;
  }
  
  if( recipe_mod.empty() ) {
    do_error( "Error in get_recipe_id().  Recipe modification date was empty." );
    return false;
  }
  
  std::stringstream query;
  query << "SELECT recipe_id FROM recipe_meta WHERE recipe_name='"
	<< recipe << "' AND recipe_mod='" << recipe_mod << "'"; 
  
  if ( !sql_select(query.str()) ) {
    do_error( "Query failed in get_recipe_id()." );
    return false;
  }

  my_ulonglong row_count = mysql_num_rows(_result);
  
  if( row_count < 1 ) {
    ret_val = kInvalidId;
    mysql_free_result(_result);
    return true;
  }
  
  if ( (_row = mysql_fetch_row(_result)) == NULL || !_row[0]) {
    do_error( "Failed fetching row in get_recipe_id()." );
    mysql_free_result(_result);
    return false;
  }

  ret_val = atoi(_row[0]);
  mysql_free_result(_result);
  return true;
}

bool
AutonetworkerDBModel_MySQL::get_algo_type_id( const std::string& algo_type, eLevel level, int& ret_val )
{
  ret_val = kInvalidId;

  if (algo_type.empty()) {
    do_error("algo_type was empty in AutonetworkerDBModel::get_algo_type_id().");
    return false;
  }

  std::stringstream query;
  query << "SELECT algo_type_id FROM algo_type WHERE algo_type='" << algo_type << "' AND algo_level='" << static_cast<u_int>(level) << "'";

  if ( !sql_select(query.str()) ) {
    do_error( "Query failed in AutonetworkerDBModel::get_algo_type_id()." );
    return false;
  }

  my_ulonglong row_count = mysql_num_rows(_result);

  if( row_count < 1 ) {
    ret_val = kInvalidId;
    mysql_free_result(_result);
    return true;
  }

  if ( (_row = mysql_fetch_row(_result)) == NULL || !_row[0]) {
    do_error( "Failed fetching row in get_algo_type_id()." );
    mysql_free_result(_result);
    return false;
  }

  ret_val = atoi(_row[0]);
  mysql_free_result(_result);
  return true;
}

bool
AutonetworkerDBModel_MySQL::get_group_id( const std::string& ref_name, int& ret_val )
{
  if ( ref_name.empty() ) {
    do_error( "ref_name was empty in AutonetworkerDBModel::get_group_id()." );
    return false;
  }

  if ( !_group_meta_result ) {
    do_error( "group_meta_results was NULL in AutonetworkerDBModel::get_group_id()." );
    return false;
  }

  MYSQL_ROW_OFFSET offset_orig = mysql_row_tell(_group_meta_result);
  
  while ((_row = mysql_fetch_row(_group_meta_result)) != NULL)
  {
    
    const std::string meta_ref_name( _row[kGroupMetaRefNameIndex] );

    if ( ref_name == meta_ref_name ) {
      ret_val = atoi(_row[kGroupMetaIDIndex]);
      return true;
    }
  }

  mysql_row_seek( _group_meta_result, offset_orig );
  return false;
}

bool
AutonetworkerDBModel_MySQL::get_next_id( eLevel level, int& ret_val )
{
  switch (level) {

  case eLevel2:
    _result = _group_meta_result;
    break;
    
  case eLevel1:
    _result = _item_meta_result;
    break;
    
  default:
    break;
  }

  if ( _result && (_row = mysql_fetch_row(_result)) ) {
    ret_val = atoi(_row[0]);
    return true;
  }

  return false;
}

bool
AutonetworkerDBModel_MySQL::load_ids( eLevel level )
{
  std::stringstream query;
  
  switch (level) {
    
  case eLevel2:
    query << "SELECT group_id,ref_name,psr_panel FROM group_meta WHERE group_meta.visible='1' AND recipe_id='"
	  << _curr_recipe_id
	  << "'";

    if ( !sql_query(query.str()) )
      return false;
    
    return (_group_meta_result = mysql_store_result( _connection )) != NULL;
    
  case eLevel1:
    query << "SELECT item_id FROM item_meta WHERE item_meta.visible='1' AND recipe_id='"
	  << _curr_recipe_id
	  << "'";
    
    if ( !sql_query(query.str()) )
      return false;
    
    return (_item_meta_result = mysql_store_result( _connection )) != NULL;
    
  default:
    break;
  }
  
  return false; // not reached.
}

bool
AutonetworkerDBModel_MySQL::erase_all()
{
  // USE WITH CARE!!
  // This function will erase ALL the data.

  const char *wild = NULL;
  _result = mysql_list_tables( _connection, wild );

  if (!_result) {
    do_error( "Delete failed in AutonetworkerDBModel_MySQL::erase_all()." );
    return false;
  }
  
  while ((_row = mysql_fetch_row(_result)) != NULL)
  {

    std::stringstream tablename;
    tablename << _row[0];
    std::stringstream del_query;
    del_query << "TRUNCATE " << tablename.str();
    std::cerr << del_query.str() << std::endl;

    if ( !sql_query(del_query.str()) ) {
      mysql_free_result(_result);
      return false;
    }

    std::stringstream sql;
    sql << "ALTER TABLE `" << tablename.str() << "` AUTO_INCREMENT=0";

    if ( !sql_query(sql.str()) ) {
      mysql_free_result(_result);
      return false;
    }

    sql.str( std::string() ); // Clear old sql.
    sql << "OPTIMIZE NO_WRITE_TO_BINLOG TABLE `" << tablename.str() << "`";

    /*    if ( !sql_query(sql.str()) ) {
      mysql_free_result(_result);
      do_error( std::string( "Failed optimizing table `" + tablename.str() + "`"));
      return false;
      }*/
  }

  sql_query( "FLUSH TABLES" );
  mysql_free_result(_result);
  return true;
}

bool
AutonetworkerDBModel_MySQL::erase( const u_int retain, const std::string& machine )
{
  if ( machine.empty() ) {
    do_error( "The machine name was not specified in AutonetworkerDBModel::erase()." );
    return false;
  }

  // Lame. MySQL does not allow LIMITS in sub queries so we must ad hoc(hack) it.

  std::stringstream subsqlss;
  subsqlss << "SELECT insp_id FROM insp_hdr WHERE machine='" << machine << "' ORDER BY insp_id desc LIMIT " << retain;

  if ( !sql_select(subsqlss.str()) ) {
    do_error("Failed getting insp_ids in AutonetworkerDBModel::erase().");
    return false;
  }

  u_int min_insp_id = 0;

  // Get value of insp_id from last row.  This will give you the min
  // insp_id that will leave "retain" amount in the database.

  while ((_row = mysql_fetch_row(_result)) != NULL)
  {
    min_insp_id = atoi(_row[0]);
  }
  
  std::vector<std::string> tbl;

  tbl.push_back("group_results");
  tbl.push_back("item_results");
  tbl.push_back("panel_results");
  tbl.push_back("insp_hdr");

  std::vector<std::string>::iterator i = tbl.begin();

  for (; i != tbl.end(); ++i) {

    std::stringstream sql;
    sql << "DELETE FROM " << *i << " WHERE insp_id < " << min_insp_id;

    if ( !sql_query(sql.str()) ) {
      goto not_ok;
    }
  }

 ok:
  mysql_free_result(_result);
  return true;

 not_ok:
  mysql_free_result(_result);
  return false;
}

bool
AutonetworkerDBModel_MySQL::transaction_begin()
{
  if (!_connection) {
    return false;
  }

  std::string sql("START TRANSACTION");

  if ( !sql_query(sql) ) {
    do_error( "Failed starting transaction in AutonetworkerDBModel::transaction_begin()" );
    return false;
  }
  return true;
}

bool
AutonetworkerDBModel_MySQL::transaction_commit()
{
  if (!_connection) {
    return false;
  }

  std::string sql("COMMIT");

  if ( !sql_query(sql) ) {
    do_error( "Failed committing transaction in AutonetworkerDBModel::transaction_commit()" );
    return false;
  }
  return true;
}

bool
AutonetworkerDBModel_MySQL::transaction_rollback()
{
  if (!_connection) {
    return false;
  }

  std::string sql("ROLLBACK");

  if ( !sql_query(sql) ) {
    do_error( "Failed rollback of transaction in AutonetworkerDBModel::transaction_rollback()" );
    return false;
  }
  return true;
}

bool
AutonetworkerDBModel_MySQL::db_create( const std::string dbname )
{
  if (!_connection) {
    return false;
  }

  // Check for existence first...
  std::stringstream sql;
  sql << "SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = '" << dbname << "'";

  if ( !sql_select(sql.str()) ) {
    do_error( "Failed selecting SCHEMA_NAME to detect for database existence." );
    return false;
  }

  bool db_exists = mysql_num_rows(_result) > 0;

  if ( !db_exists ) {
    
    const std::string db_create_sql = "CREATE DATABASE IF NOT EXISTS `" + dbname + "`";
    
    if ( !sql_query(db_create_sql) ) {
      do_error( "Failed creating database." );
      return false;
    }
  }

  const std::string use_db_sql = "USE `" + dbname + "`";
  
  if ( !sql_query(use_db_sql) ) {
    do_error( "Failed issuing USE statement." );
    return false;
  }

  if ( !db_exists && !exec_from_file( kCreateDBFile ) ) {
    do_error( std::string("Failed executing SQL file \"" + kCreateDBFile + "\".") );
    return false;
  }
  
  return true;
}

bool
AutonetworkerDBModel_MySQL::db_size( const std::string& dbname, double& ret )
{
  ret = -1;
  std::stringstream sql;
  std::string val;
  
  sql << "SELECT CONCAT(sum(ROUND(((DATA_LENGTH + INDEX_LENGTH - DATA_FREE) / 1024 / 1024),2))) AS Size FROM INFORMATION_SCHEMA.TABLES where TABLE_SCHEMA = '" << dbname << "'";
  
  if ( sql_select_single_val(sql.str(), val) ) {
    ret = static_cast<double>( atof(val.c_str()) );
    return true;
  }
  
  do_error( "Failed getting db size." );
  return false;
}
