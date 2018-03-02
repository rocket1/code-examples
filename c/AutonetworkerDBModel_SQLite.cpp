#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "AutonetworkerDBModel_SQLite.h"

static const std::string kCreateDBFile("/opt/mvp/script/anet.sqlite.sql");
static const std::string kDefaultDB("/opt/mvp/stats/anet.db.sqlite");

bool
AutonetworkerDBModel_SQLite::load_cfg_desc( std::map< std::string, u_int >& ret_map )
{
  try {
    
    ret_map.clear();
    
    std::stringstream sql;
    sql << "SELECT id, cfg_name FROM cfg_meta";
    
    if ( !sql_select(sql.str()) ) {
      do_error( "Failed in load_cfg_desc() retrieving cfg data." );
      return false;
    }
    
    sqlite3pp::query::iterator i = _qry->begin();

    // It's okay to have no records yet, e.g. very first inspection.
    if (i == _qry->end()) {
      return true;
    }
    
    for (; i != _qry->end(); ++i) {
      const u_int id = atoi(i->get<char const*>(0));
      const char* name = i->get<char const*>(0);
      if (name[0] == '\0') {
	continue;
      }
      ret_map[std::string(name)] = id;
    }
    
    return true;
    
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

AutonetworkerDBModel_SQLite::AutonetworkerDBModel_SQLite() : _db(NULL), _qry(NULL), _trn(NULL), isConnected(false)
{
  _dbtype = "sqlite"; // Just for printing.  Not really a "type code".
}

AutonetworkerDBModel_SQLite::~AutonetworkerDBModel_SQLite()
{
  std::cout << " AutonetworkerDBModel_SQLite::disconnect....." 
      << std::endl;
  if (_db) {

    _db->disconnect();
    delete _db;
    _db = NULL;
  }

  if (_qry) {
    delete _qry;
    _qry = NULL;
  }
  
  if (_trn) {
    delete _trn;
    _trn = NULL;
  }
}

bool
AutonetworkerDBModel_SQLite::connect( const std::string& host,
				      const std::string& user,
				      const std::string& pass,
				      const std::string& db )
{
  if ( isConnected ) 
      return true;
  std::cout << " AutonetworkerDBModel_SQLite::connect....." 
      << std::endl;
  isConnected = false;
  const std::string dbname( db.empty() ? kDefaultDB : db );
  _dbname = dbname;

  const std::string kEmpty("<empty>");
  const std::string kHidden("<hidden>");

  const std::string host_display = host.empty() ? kEmpty : host;
  const std::string user_display = user.empty() ? kEmpty : user;
  const std::string pass_display = pass.empty() ? kEmpty : kHidden;
  const std::string db_display   = db.empty()   ? kEmpty : db;
      

  std::stringstream info;
  info << "[host: " << host_display << ", type: SQLite, db: " << dbname << "]";

  try {

    // Make sure no EMPTY database exists.  This may occur if
    // another program has attempted to connect before any
    // inspection data is written.  This is a potential
    // race condition and should be handled cleaner...

    struct stat sbuf;

    if ( (stat(dbname.c_str(), &sbuf) == 0) && (sbuf.st_size == 0) ) {

      if ( unlink(dbname.c_str()) != 0 ) {
	do_error( std::string("Failed unlinking sqlite database \"" + dbname + "\"."));
	return false;
      }
    }

    // Some situations to be determined cause a -journal file hanging around,
    // accompanying some corrupt data.
    if ( ! check_journal(dbname) ) {
      return false;
    }

    _db = new sqlite3pp::database(dbname.c_str());
    
    if (!db_create()) {
      do_error("db_create() failed in AutonetworkerDBModel_SQLite::connect().");
      return false;
    }

    std::stringstream connmsg;
    connmsg << "CONNECT " << info.str();
    _log->write(connmsg.str());

    if ( int ret = chmod( dbname.c_str(), 0777 ) != 0 ) {

      switch (ret) {
      case ENOENT:
	_log->write( "The named file doesn't exist." );
	break;
      case EPERM:
	_log->write( "This process does not have permission to change the access permissions of this file. Only the file's owner (as judged by the effective user ID of the process) or a privileged user can change them." );
	break;
      case EROFS:
	_log->write( "The file resides on a read-only file system." );
	break;
      default:
	break;
      }

      do_error( std::string("Failed changing permissions on \"" + dbname + "\"") );
    }

    isConnected = true;
    return true;
  }
  catch ( std::runtime_error e ) {
    
    std::stringstream err;
    err << "CONNECT FAILED " << info.str();
    do_error(err.str());

    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::sql_select( const std::string& query )
{
  try {

     if (!_db) {
      do_error( "Connection was NULL in sql_query()" );
      return false;
    }
    
    if (query.empty()) {
      do_error( "Query was empty in sql_query()." );
      return false;
    }

    if (_qry) {
      delete _qry;
      _qry = NULL;
    }

    _qry = new sqlite3pp::query( *_db, query.c_str() );
    return true;
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::sql_select_single_val( const std::string& query, std::string& ret_val )
{
  try {

    if ( query.empty() ) {
      return false;
    }

    if ( !sql_select(query) ) {
      return false;
    }

    sqlite3pp::query::iterator i = _qry->begin();  

    if ( i == _qry->end() ) {
      return false;
    }

    const char* val = i->get<const char*>(0);

    if (val == '\0') {
      return false;
    }

    ret_val = std::string(val);
    return true;
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::sql_query( const std::string& query )
{
  try {

    if (_debug) {
      std::cerr << "SQLite SQL: " << query << std::endl;
    }

    if (!_db) {
      do_error( "Connection was NULL in AutonetworkerDBModel_SQLite::sql_query()" );
      return false;
    }
    
    if (query.empty()) {
      return false;
    }
    
    sqlite3pp::command cmd( *_db, query.c_str() );
    int ret = cmd.execute();

    if ( ret != SQLITE_OK ) {
      sqlite_err( ret );
      return false;
    }

    return true;
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::sql_query_return_id( const std::string& query, int& ret_val )
{
  try {
    
    if ( !sql_query(query.c_str()) ) {
      return false;
    }
    
    ret_val = static_cast<int>( _db->last_insert_rowid() );
    _insert_id = ret_val; // for convenience.
    return true;
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::num_insp( const std::string& lot_id, u_int& ret_val )
{
  try {
    
    std::stringstream sql;
    sql << "SELECT COUNT(*) FROM insp_hdr WHERE lot_id='" << lot_id << "'";
    
    if ( !sql_select(sql.str()) ) {
      do_error( "Query Failed in num_insp()." );
      return false;
    }
    
    sqlite3pp::query::iterator i = _qry->begin();
    ret_val = static_cast<u_int>(i->get<int>(0));
    return true;
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::get_recipe_id( const std::string& recipe, const std::string& recipe_mod, int& ret_val )
{
  try {

    ret_val = kInvalidId;

    if (recipe.empty()) {
      do_error( "Error in get_recipe_id(). Recipe name was empty." );
      return false;
    }
    
    if( recipe_mod.empty() ) {
      do_error( "Error in get_recipe_id().  Recipe modification date was empty." );
      return false;
    }
    
    std::stringstream sql;
    sql << "SELECT recipe_id FROM recipe_meta WHERE recipe_name='" << recipe << "' AND recipe_mod='" << recipe_mod << "'"; 

    if ( !sql_select(sql.str()) ) {
      do_error( "Query failed in get_recipe_id()." );
      return false;
    }
    
    sqlite3pp::query::iterator i = _qry->begin();

    if (i == _qry->end()) {
      return true; // It's okay to have not found recipe id.
    }

    ret_val = i->get<int>(0);
    return true;

  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::get_algo_type_id( const std::string& algo_type, eLevel level, int& ret_val )
{
  try {

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

    if (_qry->begin() == _qry->end()) {
      // It's okay that we didn't find one because
      // might be first db record.
      return true;
    }
    
    sqlite3pp::query::iterator i = _qry->begin();
    ret_val = i->get<int>(0);

    if (ret_val < 1) {
      ret_val = kInvalidId;
    }

    return true;
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::get_next_id( eLevel level, int& ret_val )
{
  switch (level) {

  case eLevel2:

    if (_loaded_grp_ids.size() < 1) {
      do_error( "Ran out of group ids in AutonetworkerDBModel_SQLite::get_next_id()." );
      return false;
    }
    
    ret_val = _loaded_grp_ids.front();
    _loaded_grp_ids.pop_front();
    return true;
    
  case eLevel1:

    if (_loaded_itm_ids.size() < 1) {
      do_error( "Ran out of item ids in AutonetworkerDBModel_SQLite::get_next_id()." );
      return false;
    }

    ret_val = _loaded_itm_ids.front();
    _loaded_itm_ids.pop_front();
    return true;
    
  default:
    break;
  }

  return false;
}

bool
AutonetworkerDBModel_SQLite::load_ids( eLevel level )
{
  try {
    
    std::stringstream query;
    sqlite3pp::query::iterator i;
    
    switch (level) {
      
    case eLevel2:
      
      query << "SELECT group_id FROM group_meta WHERE recipe_id='"
	    << _curr_recipe_id
	    << "' AND group_meta.visible='1'";
      
      if ( !sql_select(query.str()) ) {
	return false;
      }
      
      _loaded_grp_ids.clear();
      i = _qry->begin();

      for (; i != _qry->end(); ++i) {
	const u_int id = static_cast<u_int>(i->get<int>(0));
	_loaded_grp_ids.push_back(id);
      }

      return true;
      
    case eLevel1:
      
      query << "SELECT item_id FROM item_meta WHERE recipe_id='"
	    << _curr_recipe_id
	    << "' AND item_meta.visible='1'";
      
      if ( !sql_select(query.str()) ) {
	return false;
      }
      
      _loaded_itm_ids.clear();
      i = _qry->begin();
      
      for (; i != _qry->end(); ++i) {
	const u_int id = static_cast<u_int>(i->get<int>(0));
	_loaded_itm_ids.push_back(id);
      }
      
      return true;
      
    default:
      break;
    }
    
    return false;
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::erase_all()
{
  // USE WITH CARE!!
  // This function will erase ALL the data.

  std::vector<std::string> tbl = table_names();
  std::vector<std::string>::iterator i = tbl.begin();

  for (; i != tbl.end(); ++i) {

    std::stringstream sql;
    sql << "DELETE FROM `" << *i << "`";
    
    if ( !sql_query(sql.str()) ) {
      do_error( std::string("Failed truncating table '" + *i + "' in AutonetworkerDBModel_SQLite::erase_all().") );
      return false;
    }
  }

  return true;
}

bool
AutonetworkerDBModel_SQLite::erase( const u_int retain, const std::string& machine )
{
  if ( machine.empty() ) {
    do_error( "The machine name was not specified in AutonetworkerDBModel_SQLite::erase()." );
    return false;
  }

  std::stringstream subsqlss;
  subsqlss << "SELECT insp_id FROM insp_hdr WHERE machine='" << machine << "' ORDER BY insp_id desc LIMIT " << retain;

  if ( !sql_select(subsqlss.str()) ) {
    do_error("Failed getting insp_ids in AutonetworkerDBModel_SQLite::erase().");
    return false;
  }

  // Get value of insp_id from last row.  This will give you the min
  // insp_id that will leave "retain" amount in the database.

  u_int min_insp_id = 0;

  for (sqlite3pp::query::iterator i = _qry->begin(); i != _qry->end(); ++i) {
    min_insp_id = atoi(i->get<char const*>(0));
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
      return false;
    }
  }

  return true;
}

bool
AutonetworkerDBModel_SQLite::transaction_begin()
{
  try {
    
    if (!_db) {
      do_error( "Connection not valid in AutonetworkerDBModel_SQLite::transaction_begin()." );
      return false;
    }
    
    if (_trn) {
      delete _trn;
      _trn = NULL;
    }

    _trn = new sqlite3pp::transaction(*_db);
    sql_query( "PRAGMA journal_mode = OFF" );
    if (!_trn) {
      do_error( "Transaction not valid in AutonetworkerDBModel_SQLite::transaction_begin()." );
      return false;
    }

    if (_debug) {
      do_error( "AutonetworkerDBModel_SQLite::transaction_begin()" );
    }

    return true;
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::transaction_commit()
{
  try {
    
    if (!_db) {
      do_error( "Connection not valid in AutonetworkerDBModel_SQLite::transaction_commit()." );
      return false;
    }

    if (!_trn) {
      do_error( "Transaction not valid in AutonetworkerDBModel_SQLite::transaction_commit()." );
      return false;
    }
    
    int trans_code = _trn->commit();

    if ( trans_code != SQLITE_OK ) {
      sqlite_err( trans_code );
      return false;
    }

    return true;
  }
  catch ( std::runtime_error e ) {
    std::stringstream msg;
    msg << "Failed in AutonetworkerDBModel_SQLite::transaction_commit(). " << e.what();
    do_error(msg.str());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::transaction_rollback()
{
  try {
    
    if (!_db) {
      do_error( "Connection not valid in AutonetworkerDBModel_SQLite::transaction_rollback()." );
      return false;
    }

    if (!_trn) {
      do_error( "Transaction not valid in AutonetworkerDBModel_SQLite::transaction_rollback()." );
      return false;
    }
   
    int trans_code = _trn->rollback();

    if (_debug) {
      std::cerr << "SQLite: Transaction ROLLBACK returned code: " << trans_code << std::endl;
    }

    if ( trans_code != SQLITE_OK ) {
      std::stringstream msg;
      msg << "Rollback failed. SQLite code: " << trans_code;
      do_error( msg.str() );
      return false;
    }

    return true;
  }
  catch ( std::runtime_error e ) {
    do_error(e.what());
    return false;
  }
}

bool
AutonetworkerDBModel_SQLite::db_create()
{
  return exec_from_file( kCreateDBFile );
}

bool
AutonetworkerDBModel_SQLite::db_size( const std::string& dbname, double& ret )
{
  ret = -1;
  
  struct stat sbuf;
  
  if ( stat( dbname.c_str(), &sbuf ) == 0 ) {
    ret = static_cast<double>(sbuf.st_size * 10^-6);
    return true;
  }
  
  do_error( "Failed getting db size." );
  return false;
}

const std::string
AutonetworkerDBModel_SQLite::sqlite_result_str( const int result_code )
{
  switch (result_code) {
  case SQLITE_OK:
    return "Successful result";
  case SQLITE_ERROR:
    return "SQL error or missing database";
  case SQLITE_INTERNAL:
    return "Internal logic error in SQLite";
  case SQLITE_PERM:
    return "Access permission denied";
  case SQLITE_ABORT:
    return "Callback routine requested an abort";
  case SQLITE_BUSY:
    return "The database file is locked";
  case SQLITE_LOCKED:
    return "A table in the database is locked";
  case SQLITE_NOMEM:
    return "A malloc() failed";
  case SQLITE_READONLY:
    return "Attempt to write a readonly database";
  case SQLITE_INTERRUPT:
    return "Operation terminated by sqlite3_interrupt()";
  case SQLITE_IOERR:
    return "Some kind of disk I/O error occurred";
  case SQLITE_CORRUPT:
    return "The database disk image is malformed";
  case SQLITE_NOTFOUND:
    return "Unknown opcode in sqlite3_file_control()";
  case SQLITE_FULL:
    return "Insertion failed because database is full";
  case SQLITE_CANTOPEN:
    return "Unable to open the database file";
  case SQLITE_PROTOCOL:
    return "Database lock protocol error";
  case SQLITE_EMPTY:
    return "Database is empty";
  case SQLITE_SCHEMA:
    return "The database schema changed";
  case SQLITE_TOOBIG:
    return "String or BLOB exceeds size limit";
  case SQLITE_CONSTRAINT:
    return "Abort due to constraint violation";
  case SQLITE_MISMATCH:
    return "Data type mismatch";
  case SQLITE_MISUSE:
    return "Library used incorrectly";
  case SQLITE_NOLFS:
    return "Uses OS features not supported on host";
  case SQLITE_AUTH:
    return "Authorization denied";
  case SQLITE_FORMAT:
    return "Auxiliary database format error";
  case SQLITE_RANGE:
    return "2nd parameter to sqlite3_bind out of range";
  case SQLITE_NOTADB:
    return "File opened that is not a database file";
  case SQLITE_ROW:
    return "sqlite3_step() has another row ready";
  case SQLITE_DONE:
    return "sqlite3_step() has finished executing";
  default:
    return "NOT DEFINED";
  }
}

void
AutonetworkerDBModel_SQLite::sqlite_err( const int code )
{
  std::stringstream msg;
  msg << "SQLite error: " << sqlite_result_str( code );
  do_error( msg.str() );
}

bool
AutonetworkerDBModel_SQLite::check_journal( const std::string dbname )
{
  std::string journal_file( dbname + "-journal" );
  struct stat sbuf;

  if ( (stat(journal_file.c_str(), &sbuf) == 0) ) {

    do_error( "Journal file found. Removing corrupt data." );

    if ( unlink(journal_file.c_str()) != 0 ) {
      do_error( std::string("Failed deleting journal file \"" + journal_file + "\""));
      return false;
    }

    if ( unlink(dbname.c_str()) != 0 ) {
      do_error( std::string("Failed deleting corrupt database \"" + dbname + "\""));
      return false;
    }
  }

  return true;
}
