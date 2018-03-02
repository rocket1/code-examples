#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/stat.h>
#include <mysql/mysql.h>
#include "AutonetworkerDBModel.h"

extern std::string itoa(long n);

////////////////////////////////////////////////////////////////////////////////
//
// R E C I P E
//
////////////////////////////////////////////////////////////////////////////////

bool
AutonetworkerDBModel::insert_recipe_meta( const TABLE_VAL_MAP& val_map )
{
  std::string recipe_name;
  std::string recipe_mod;
  
  if ( !get_table_val( "recipe_name", val_map, recipe_name ) || recipe_name.empty() ) {
    do_error( "Failed getting recipe_name from table map." );
    return false;
  }

  if ( !get_table_val( "recipe_mod", val_map, recipe_mod ) || recipe_mod.empty() ) {
    do_error( "Failed getting recipe_mod from table map." );
    return false;
  }
 
  if ( !get_recipe_id( recipe_name, recipe_mod, _curr_recipe_id ) ) {
    do_error( "Failed getting recipe id." );
    return false;
  }

  _has_meta = _curr_recipe_id != kInvalidId;

  if (!_has_meta) {

    std::string recipe_sql_string;

    if ( !recipe_sql(val_map, recipe_sql_string) ) {
      do_error( "Failed creating recipe sql." );
      return false;
    }

    if ( !sql_query_return_id(recipe_sql_string, _curr_recipe_id) ) {
      do_error( "Failed getting return id for new recipe meta." );
      return false;
    }
  }

  return true;
}

bool
AutonetworkerDBModel::recipe_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  if ( !get_insert_stmt(val_map, "recipe_meta", ret_val) ) {
    do_error( "Failed creating INSERT statement in recipe_sql()." );
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// I N S P E C T I O N
//
////////////////////////////////////////////////////////////////////////////////

bool
AutonetworkerDBModel::insert_insp_hdr( const TABLE_VAL_MAP& val_map )
{
  if (_curr_recipe_id < 0) {
    do_error( "No valid recipe id set when inserting inspection header." );
    return false;
  }

  if ( !load_ids(eLevel2) ) {
    do_error( "Failed loading group ids from group meta." );
    return false;
  }

  if ( !load_ids(eLevel1) ) {
    do_error( "Failed loading item ids from item meta." );
    return false;
  }
    
  std::string insp_hdr_sql_string;

  if ( !insp_hdr_sql(val_map, insp_hdr_sql_string) ) {
    do_error( "Failed creating INSERT statement in insert_insp_hdr()." );
    return false;
  }

  if ( !sql_query_return_id( insp_hdr_sql_string, _curr_insp_id ) ) {
    do_error( "Failed getting return id for newly created inpection header." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::insp_hdr_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  // recipe_id,serial,run,time_of_insp,cycle_time,operator,machine,line,centi_time_of_insp,lot_id,lane,cfg

  std::string run = "1";
  
  std::string sql( "SELECT MAX(run) FROM insp_hdr WHERE lot_id='" + val_map["lot_id"] + "'" );
  std::string val;

  if ( sql_select_single_val(sql, val) ) {
    int intval = atoi( val.c_str() );
    intval++;
    run = std::string( itoa(intval) );
  }

  val_map["run"] = run;
  val_map["recipe_id"] = int2str(_curr_recipe_id);

  if ( !get_insert_stmt(val_map, "insp_hdr", ret_val) ) {
    do_error( "Failed creating INSERT statement in insp_hdr_sql()." );
    return false;
  }
  return true;
}		

////////////////////////////////////////////////////////////////////////////////
//
// G R O U P
//
////////////////////////////////////////////////////////////////////////////////

bool
AutonetworkerDBModel::insert_group_meta( const TABLE_VAL_MAP& val_map ) 
{
  if (_has_meta) {
    return true;
  }

  if (_curr_recipe_id < 0) {
    do_error( "No valid recipe id set when inserting group meta." );
    return false;
  }

  std::string group_meta_sql_string;
  
  if ( !group_meta_sql(val_map, group_meta_sql_string) ) {
    do_error( "Failed getting group meta sql string." );
    return false;
  }

  if ( !sql_query_return_id(group_meta_sql_string, _curr_group_id) ) {
    do_error( "Failed getting returned id for new group meta." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::insert_group_results( const TABLE_VAL_MAP& val_map )
{
  if (_curr_insp_id < 0) {
    do_error( "Failed inserting group results because inspection id was invalid." );
    return false;
  }

  std::string ref_name;

  if ( !get_table_val( "ref_name", val_map, ref_name ) ) {
    do_error( "Failed getting ref_name from table map." );
    return false;
  }

  if ( !get_next_id(eLevel2, _curr_group_id) ) {
    do_error( "Failed getting next group id from group meta." );
    return false;
  } 

  if (_curr_group_id < 0) {
    do_error( "No valid group id set when inserting group results." );
    return false;
  }

  std::string group_results_sql_string;

  // The table doesn't have a ref name column, but we needed it to do the meta lookup.
  // We must remove it as not screw up our generated query.
  TABLE_VAL_MAP val_map_modified = val_map;
  erase_val_map_key( "ref_name", val_map_modified );

  if ( !group_results_sql(val_map_modified, group_results_sql_string) ) {
    do_error( "Failed getting group results sql string." );
    return false;
  }

  if ( !sql_query(group_results_sql_string) ) {
    do_error( "Failed Query in add_group_sql()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::group_meta_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  // recipe_id,ref_name,x_loc,y_loc,dx_coord,dy_coord,algo_number,orient,psr_panel,panel_x,panel_y

  val_map["recipe_id"] = int2str(_curr_recipe_id);
  int ret;

  if ( !find_algo_id( _curr_recipe_id, atoi((val_map["algo_number"]).c_str()), eLevel2, ret ) ) {
    do_error( "Failed getting algo_id in AutonetworkerDBModel::group_meta_sql()" );
    return false;
  }

  val_map["algo_id"] = std::string(itoa(ret));
 
  if ( !get_insert_stmt(val_map, "group_meta", ret_val) ) {
    do_error( "Failed creating INSERT statement in group_meta_sql()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::group_results_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  // insp_id,group_id,result_vector,decision,condition

  val_map["insp_id"] = int2str(_curr_insp_id);
  val_map["group_id"] = int2str(_curr_group_id);

  if ( !get_insert_stmt(val_map, "group_results", ret_val) ) {
    do_error( "Failed creating INSERT statement in recipe_sql()." );
    return false;
  }
  
  return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// I T E M
//
////////////////////////////////////////////////////////////////////////////////

bool
AutonetworkerDBModel::insert_item_meta( const TABLE_VAL_MAP& val_map )
{
  if (_has_meta)
    return true;

  if (_curr_group_id < 0) {
    do_error( "No valid group id set when inserting item meta." );
    return false;
  }

  std::string item_meta_sql_string;

  if ( !item_meta_sql(val_map, item_meta_sql_string) ) {
    do_error( "Failed getting item meta sql string." );
    return false;
  }

  if ( !sql_query_return_id(item_meta_sql_string, _curr_item_id) ) {
    do_error( "Failed getting returned id for new item meta." );
    return false;
  }
  
  return true;
}

bool
AutonetworkerDBModel::insert_item_results( const TABLE_VAL_MAP& val_map )
{
  if (_curr_insp_id < 0) {
    do_error( "Failed inserting item results because inspection id was invalid." );
    return false;
  }

  if ( !get_next_id(eLevel1, _curr_item_id) ) {
    do_error( "Failed getting next item id from group meta." );
    return false;
  }

  if (_curr_item_id < 0) {
    do_error( "No valid item id set when inserting item results." );
    return false;
  }

  std::string item_results_sql_string;
  
  if ( !item_results_sql(val_map, item_results_sql_string) ) {
    do_error( "Failed getting item results sql string." );
    return false;
  }
  
  if ( !sql_query(item_results_sql_string) ) {
    do_error( "Failed Query in insert_item_results()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::item_meta_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  // group_id,recipe_id,pin,x_loc,y_loc,dx_loc,dy_loc,algo_number,orient,is_fiducial,psr_panel,panel_x,panel_y

  val_map["group_id"] = int2str(_curr_group_id);
  val_map["recipe_id"] = int2str(_curr_recipe_id);
  int ret;

  if ( !find_algo_id( _curr_recipe_id, atoi((val_map["algo_number"]).c_str()), eLevel1, ret ) ) {
    do_error( "Failed getting algo_id in AutonetworkerDBModel::group_meta_sql()" );
    return false;
  }

  val_map["algo_id"] = std::string(itoa(ret));

  if ( !get_insert_stmt(val_map, "item_meta", ret_val) ) {
    do_error( "Failed creating INSERT statement in item_meta_sql()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::item_results_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  // insp_id,item_id,result_vector,decision,condition

  val_map["insp_id"] = int2str(_curr_insp_id);  
  val_map["item_id"] = int2str(_curr_item_id);

  if ( !get_insert_stmt(val_map, "item_results", ret_val) ) {
    do_error( "Failed creating INSERT statement in item_results_sql()." );
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// P A N E L
//
////////////////////////////////////////////////////////////////////////////////

bool
AutonetworkerDBModel::insert_panel_results( const TABLE_VAL_MAP& val_map )
{
  if (_curr_insp_id < 0) {
    do_error( "No valid inspection when inserting panel results." );
    return false;
  }

  std::string panel_results_sql_string;

  if ( !panel_results_sql(val_map, panel_results_sql_string) ) {
    do_error( "Failed getting panel results sql string." );
    return false;
  }

  if ( !sql_query(panel_results_sql_string) ) {
    do_error( "Failed query in insert_panel_results()." );
    return false;
  }
  return true;
}

bool
AutonetworkerDBModel::panel_results_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  // insp_id,psr_panel,failed_count,barcode,condition,panel_x,panel_y

  val_map["insp_id"] = int2str(_curr_insp_id);

  if ( !get_insert_stmt(val_map, "panel_results", ret_val) ) {
    do_error( "Failed creating INSERT statement in panel_results_sql()." );
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// A L G O
//
////////////////////////////////////////////////////////////////////////////////

bool
AutonetworkerDBModel::insert_algo_type( const TABLE_VAL_MAP& val_map, const std::vector<std::string>& iv_names_vec )
{
  if (_has_meta) {
    return true;
  }

  std::string algo_type;
  std::string algo_level;

  if ( !get_table_val( "algo_type", val_map, algo_type ) || algo_type.empty() ) {
    do_error( "Failed getting algo_type from table map in AutonetworkerDBModel::insert_algo_type()." );
    return false;
  }
  
  if ( !get_table_val( "algo_level", val_map, algo_level ) || algo_level.empty() ) {
    do_error( "Failed getting algo_level from table map in AutonetworkerDBModel::insert_algo_type()." );
    return false;
  }

  eLevel algo_level_enum;

  if (algo_level == "1") {
    algo_level_enum = eLevel1;
  }
  else if (algo_level == "2") {
    algo_level_enum = eLevel2;
  }
  else {
    do_error("The string version of algo level did not correspond to a real level (i.e. was not \"1\" or \"2\").");
    return false;
  }

  _curr_algo_type_id = kInvalidId;

  if ( !get_algo_type_id( algo_type, algo_level_enum, _curr_algo_type_id ) ) {
    do_error( "Failed getting algo type id in AutonetworkerDBModel::insert_algo_type()." );
    return false;
  }

  if ( _curr_algo_type_id == kInvalidId ) { 
  
    std::string algo_type_sql_string;
    
    if ( !algo_type_sql(val_map, algo_type_sql_string) ) {
      do_error( "Failed creating algo_type sql in AutonetworkerDBModel::insert_algo_type()." );
      return false;
    }
    
    if ( !sql_query_return_id(algo_type_sql_string, _curr_algo_type_id) ) {
      do_error( "Failed getting return id for new algo type in AutonetworkerDBModel::insert_algo_type()." );
      return false;
    }

    std::vector<std::string>::const_iterator i = iv_names_vec.begin();
    u_int ndx = 0;

    for (; i != iv_names_vec.end(); i++) {

      TABLE_VAL_MAP algo_iv_names_val_map;

      algo_iv_names_val_map["iv_name"] = *i;
      algo_iv_names_val_map["iv_ndx"] = itoa(ndx);

      ndx++;

      if ( !insert_algo_iv_names(algo_iv_names_val_map) ) {
        do_error( "Failed inserting algo iv name in AutonetworkerDBModel::insert_algo_type()" );
        return false;
      }
    }
  }
  
  return true;
}

bool
AutonetworkerDBModel::insert_algo_iv_names( const TABLE_VAL_MAP& val_map )
{
  if (_has_meta)
    return true;

  std::string algo_iv_names_sql_string;

  if ( !algo_iv_names_sql(val_map, algo_iv_names_sql_string) ) {
    do_error( "Failed creating algo_iv_names_meta sql in AutonetworkerDBModel::insert_algo_iv_names()." );
    return false;
  }

  if ( !sql_query(algo_iv_names_sql_string) ) {
    do_error( "Failed inserting new algo iv name in AutonetworkerDBModel::insert_algo_iv_names()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::insert_algo_meta( const TABLE_VAL_MAP& val_map )
{
  if (_has_meta)
    return true;

  std::string algo_meta_sql_string;

  if ( !algo_meta_sql(val_map, algo_meta_sql_string) ) {
    do_error( "Failed getting algo meta sql string in AutonetworkerDBModel::insert_algo_meta()." );
    return false;
  }

  if ( !sql_query(algo_meta_sql_string) ) {
    do_error( "Failed saving algo meta in AutonetworkerDBModel::insert_algo_meta()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::algo_type_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  if ( !get_insert_stmt(val_map, "algo_type", ret_val) ) {
    do_error( "Failed creating INSERT statement in algo_type_sql()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::algo_iv_names_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  val_map["algo_type_id"] = int2str(_curr_algo_type_id);

  if ( !get_insert_stmt(val_map, "algo_iv_names", ret_val) ) {
    do_error( "Failed creating INSERT statement in algo_meta_sql()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::algo_meta_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  // recipe_id,algo_number,algo_label,algo_type,chain_to,threshold,algo_level

  val_map["recipe_id"] = int2str(_curr_recipe_id);
  val_map["algo_type_id"] = int2str(_curr_algo_type_id);

  if ( !get_insert_stmt(val_map, "algo_meta", ret_val) ) {
    do_error( "Failed creating INSERT statement in algo_meta_sql()." );
    return false;
  }
  
  return true;
}

bool
AutonetworkerDBModel::insert_cfg( const TABLE_VAL_MAP& val_map )
{
  if ( val_map.empty() ) {
    do_error( "The value map was empty in AutonetworkerDBModel::insert_cfg()." );
    return false;
  }
  
  // Map of cfg name -> internal cfg id.
  // Seems backwards, but we're using name as key here.
  std::map< std::string, u_int > db_cfg_map;
  
  if ( !load_cfg_desc(db_cfg_map) ) {
    // Something really bad happened.
    return false;
  }
  
  std::map< std::string, u_int >::iterator i = db_cfg_map.end(); // init as NOT FOUND
  TABLE_VAL_MAP::const_iterator j = val_map.begin();
  
  for (; j != val_map.end(); j++) {
    
    TABLE_VAL_MAP cfg_value_val_map;
    std::string cfg_value( j->second );
    
    cfg_value_val_map["insp_id"] = int2str(_curr_insp_id);
    cfg_value_val_map["cfg_value"] = cfg_value;
    
    std::string cfg_name( j->first );
    i = db_cfg_map.find(cfg_name);
    bool found = (i != db_cfg_map.end());
    
    if (!found) {
      
      TABLE_VAL_MAP cfg_desc_val_map;
      cfg_desc_val_map["cfg_name"] = cfg_name;
      
      if ( !insert_cfg_desc(cfg_desc_val_map) ) {
	std::stringstream msg;
	msg << "Failed saving cfg name \"" << cfg_name << "\".";
	do_error(msg.str());
	return false;
      }
    }
    
    cfg_value_val_map["cfg_meta_id"] = int2str(found ? i->second : _insert_id);
    
    if ( !insert_cfg_value(cfg_value_val_map) ) {
      std::stringstream msg;
      msg << "Failed saving cfg value \"" << cfg_value << "\" for cfg name \"" << cfg_name << "\".";
      do_error(msg.str());
      return false;
    }
  }
  
  return true;
}

bool
AutonetworkerDBModel::insert_cfg_desc( const TABLE_VAL_MAP& val_map )
{
  std::string cfg_desc_sql_string;
  
  if ( !cfg_desc_sql(val_map, cfg_desc_sql_string) ) {
    do_error( "Failed creating INSERT statement in insert_cfg_desc()." );
    return false;
  }
  
  if ( !sql_query_return_id( cfg_desc_sql_string, _insert_id ) ) {
    do_error( "Failed getting return id for newly created cfg description." );
    return false;
  }
  
  return true;
}

bool
AutonetworkerDBModel::cfg_desc_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  if ( !get_insert_stmt(val_map, "cfg_meta", ret_val) ) {
    do_error( "Failed creating INSERT statement in cfg_desc_sql()." );
    return false;
  }
  return true;
}

bool
AutonetworkerDBModel::insert_cfg_value( const TABLE_VAL_MAP& val_map )
{
  std::string cfg_value_sql_string;
  
  if ( !cfg_value_sql(val_map, cfg_value_sql_string) ) {
    do_error( "Failed creating INSERT statement in insert_cfg_value()." );
    return false;
  }
  
  if ( !sql_query(cfg_value_sql_string) ) {
    do_error( "Query failed in insert_cfg_value()" );
    return false;
  }
  
  return true;
}

bool
AutonetworkerDBModel::cfg_value_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  if ( !get_insert_stmt(val_map, "cfg_value", ret_val) ) {
    do_error( "Failed creating INSERT statement in cfg_value_sql()." );
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// R E P A I R
//
////////////////////////////////////////////////////////////////////////////////

bool
AutonetworkerDBModel::insert_repair_hdr( const TABLE_VAL_MAP& val_map )
{
  std::string serial;
  std::string centi_timestamp;

  if ( !get_table_val( "serial", val_map, serial ) ) {
    do_error( "Failed getting serial from val map in AutonetworkerDBModel::insert_repair_hdr()." );
    return false;
  }

  if ( !get_table_val( "centi_timestamp", val_map, centi_timestamp ) ) {
    do_error( "Failed getting centi_timestamp from val map in AutonetworkerDBModel::insert_repair_hdr()." );
    return false;
  }
  
  if ( !find_insp_id(serial, centi_timestamp, _curr_insp_id) ) {
    do_error( "Failed retrieving insp_id in AutonetworkerDBModel::insert_repair_hdr()" );
    return false;
  }
  
  if ( !find_recipe_id(_curr_insp_id, _curr_recipe_id) ) {
    do_error( "Failed retrieving recipe_id in AutonetworkerDBModel::insert_repair_hdr()" );
    return false;
  }

  TABLE_VAL_MAP val_map_modified = val_map;
  val_map_modified["insp_id"] = int2str(_curr_insp_id);
  erase_val_map_key( "centi_timestamp", val_map_modified );

  std::string sql;

  if ( !repair_hdr_sql(val_map_modified, sql) ) {
    do_error( "Failed creating INSERT statement in repair_insp_hdr()." );
    return false;
  }

  if ( !sql_query_return_id(sql, _curr_repair_id) ) {
    do_error( "Failed getting return id for newly created repair header." );
    return false;
  }
  
  return true;
}

bool
AutonetworkerDBModel::insert_repair_group( const TABLE_VAL_MAP& val_map )
{
  std::string ref_name;

  if ( !get_table_val("ref_name", val_map, ref_name) ) {
    do_error( "Failed getting ref name from val map in AutonetworkerDBModel::insert_group_repair." );
    return false;
  }

  if ( !find_group_id(_curr_recipe_id, ref_name, _curr_group_id) ) {
    do_error( "Failed retrieving group_id in AutonetworkerDBModel::insert_group_repair()" );
    return false;
  }

  TABLE_VAL_MAP val_map_modified = val_map;
  val_map_modified["group_id"] = int2str(_curr_group_id);
  erase_val_map_key( "ref_name", val_map_modified );

  std::string sql;

  if ( !group_repair_sql(val_map_modified, sql) ) {
    do_error( "Failed creating INSERT statement in AutonetworkerDBModel::insert_repair_group()." );
    return false;
  }

  if ( !sql_query(sql) ) {
    do_error( std::string("Query failed in AutonetworkerDBModel::insert_repair_group(): " + sql) );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::insert_repair_item( const TABLE_VAL_MAP& val_map )
{
  std::string pin_num;

  if ( !get_table_val("pin_num", val_map, pin_num) ) {
    do_error( "Failed getting pin num from val map in AutonetworkerDBModel::insert_repair_item()." );
    return false;
  }

  if ( !find_item_id(_curr_group_id, atoi(pin_num.c_str()), _curr_item_id) ) {
    do_error( "Failed retrieving group_id in AutonetworkerDBModel::insert_repair_item()." );
    return false;
  }

  TABLE_VAL_MAP val_map_modified = val_map;
  val_map_modified["item_id"] = int2str(_curr_item_id);
  erase_val_map_key( "pin_num", val_map_modified );

  std::string sql;

  if ( !item_repair_sql(val_map_modified, sql) ) {
    do_error( "Failed creating INSERT statement in AutonetworkerDBModel::insert_repair_item()." );
    return false;
  }

  if ( !sql_query(sql) ) {
    do_error( "Query failed in AutonetworkerDBModel::insert_repair_item()." );
    return false;
  }

  return true;
}

bool
AutonetworkerDBModel::repair_hdr_sql( const TABLE_VAL_MAP& val_map, std::string& ret_val )
{
  if ( !get_insert_stmt(val_map, "repair_hdr", ret_val) ) {
    do_error( "Failed creating INSERT statement in repair_hdr_sql()." );
    return false;
  }
  return true;
}

bool
AutonetworkerDBModel::group_repair_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  val_map["repair_id"] = int2str(_curr_repair_id);
  val_map["group_id"] = int2str(_curr_group_id);

  if ( !get_insert_stmt(val_map, "group_repair", ret_val) ) {
    do_error( "Failed creating INSERT statement in AutonetworkerDBModel::group_repair_sql()." );
    return false;
  }
  return true;
}

bool
AutonetworkerDBModel::item_repair_sql( TABLE_VAL_MAP val_map, std::string& ret_val )
{
  val_map["repair_id"] = int2str(_curr_repair_id);
  val_map["item_id"] = int2str(_curr_item_id);

  if ( !get_insert_stmt(val_map, "item_repair", ret_val) ) {
    do_error( "Failed creating INSERT statement in AutonetworkerDBModel::item_repair_sql()." );
    return false;
  }
  return true;
}

AutonetworkerDBModel::AutonetworkerDBModel() : _debug(false),
					       _insert_id(kInvalidId),
					       _has_meta(false),
					       _curr_group_id(kInvalidId),
					       _curr_item_id(kInvalidId),
					       _curr_insp_id(kInvalidId),
					       _curr_recipe_id(kInvalidId),
					       _curr_algo_type_id(kInvalidId),
					       _curr_repair_id(kInvalidId),
					       _dbname(std::string()),
					       _dbtype(std::string())
					       
{
  _log = AutonetworkerLog::get();
}

AutonetworkerDBModel::~AutonetworkerDBModel()
{}

bool
AutonetworkerDBModel::do_error( const std::string& msg ) 
{
  _log->write(msg);
  return true;
}

bool
AutonetworkerDBModel::get_table_val( const std::string& needle, const TABLE_VAL_MAP& haystack, std::string& ret_val )
{
  if (needle.empty()) {
    do_error( "Searching for empty needle in get_table_val()." );
    return false;
  }

  if (haystack.empty()) {
    do_error( "Haystack was empty in get_table_val()." );
    return false;
  }

  TABLE_VAL_MAP::const_iterator i = haystack.find(needle);

  if ( i == haystack.end() ) {
    return false;
  }

  if ( (i->second).empty() ) {
    do_error( "Found needle in haystack, but needle was empty in get_table_val()." );
    return false;
  }

  ret_val = i->second;
  return true;
}

bool
AutonetworkerDBModel::update_insp_hdr( const TABLE_VAL_MAP& val_map )
{
  std::string sql;
  
  if ( !get_update_stmt(val_map, "insp_hdr", sql) ) {
    return false;
  }

  return sql_query(sql);
}

bool
AutonetworkerDBModel::get_update_stmt( const TABLE_VAL_MAP& val_map, const std::string& table_name, std::string& ret_val )
{
  if (val_map.empty()) {
    do_error( "val_map was empty in get_insert_stmt()." );
    return false;
  }

  if (table_name.empty()) {
    do_error( "table_name was empty in get_insert_stmt()." );
    return false;
  }

  std::stringstream sql, vals_tmp;

  sql << "UPDATE `" << table_name << "` SET";
  TABLE_VAL_MAP::const_iterator i = val_map.begin();

  static const size_t MAX_SQL_VAL = 10000 * 1024; // 10MB big enough??

  while (1) {

    sql << " `" << i->first << "`=";
    std::string target;
    size_t sz = (i->second).size();

    if ( sz > MAX_SQL_VAL ) {
      std::stringstream msg;
      msg << "SQL value size (" << sz << ") was bigger than MAX_SQL_VAL(" << MAX_SQL_VAL << ").";
      do_error(msg.str());
      return false;
    }
    
    escape_string( i->second, target );

    sql << "\"" << target << "\"";
    if (++i == val_map.end())
      break;
    sql << ",";
  }

  sql << " WHERE insp_id='" << _curr_insp_id << "'";

  ret_val = sql.str();
  return true;
} 

bool
AutonetworkerDBModel::get_insert_stmt( const TABLE_VAL_MAP& val_map, const std::string& table_name, std::string& ret_val )
{
  if (val_map.empty()) {
    do_error( "val_map was empty in get_insert_stmt()." );
    return false;
  }

  if (table_name.empty()) {
    do_error( "table_name was empty in get_insert_stmt()." );
    return false;
  }
  
  std::stringstream sql, vals_tmp;

  sql << "INSERT INTO " << table_name << "(";
  TABLE_VAL_MAP::const_iterator i = val_map.begin();
  
  static const size_t MAX_SQL_VAL = 10000 * 1024; // 10MB big enough??

  while (1) {

    sql << "`" << i->first << "`";
    std::string target;
    size_t sz = (i->second).size();

    if ( sz > MAX_SQL_VAL ) {
      std::stringstream msg;
      msg << "SQL value size (" << sz << ") was bigger than MAX_SQL_VAL(" << MAX_SQL_VAL << ").";
      do_error(msg.str());
      return false;
    }

    escape_string( i->second, target );

    vals_tmp << "\"" << target << "\"";
    if (++i == val_map.end())
      break;
    sql << ",";
    vals_tmp << ",";
  }

  sql << ") VALUES (" << vals_tmp.str() << ")";
  ret_val = sql.str();
  return true;
}

std::string
AutonetworkerDBModel::int2str( const int i )
{
  std::stringstream ss;
  ss << i;
  return ss.str();
}

bool
AutonetworkerDBModel::erase_val_map_key( const std::string& key, TABLE_VAL_MAP& val_map )
{
  TABLE_VAL_MAP::iterator i = val_map.find(key);

  if (i == val_map.end()) {
    do_error( std::string("Failed erasing key: " + key + " in AutonetworkerDBModel::erase_val_map_key().") );
    return false;
  }
  
  val_map.erase(i);
  return true;
}

bool
AutonetworkerDBModel::find_recipe_id( const int insp_id, int& ret_val )
{
  std::stringstream sql_ss;
  sql_ss << "SELECT recipe_id FROM insp_hdr WHERE insp_id='" << insp_id << "'";
  std::string val;

  if ( !sql_select_single_val(sql_ss.str(), val) ) {
    do_error( std::string("Query failed in AutonetworkerDBModel::find_recipe_id(): " + sql_ss.str()) );
    return false;
  }

  ret_val = atoi(val.c_str());
  return true;
}

bool
AutonetworkerDBModel::find_insp_id( const std::string& serial, const std::string& centi_timestamp, int& ret_val )
{
  std::string sql( "SELECT insp_id FROM insp_hdr WHERE serial='" + serial + "' AND centi_timestamp='" + centi_timestamp + "'" );
  std::string val;

  if ( !sql_select_single_val(sql, val) ) {
    do_error( std::string("Query failed in AutonetworkerDBModel::find_insp_id(): " + sql) );
    return false;
  }

  ret_val = atoi(val.c_str());
  return true;
}

bool
AutonetworkerDBModel::find_group_id( const int recipe_id, const std::string& ref_name, int& ret_val )
{
  std::stringstream sql_ss;
  sql_ss << "SELECT group_id FROM group_meta WHERE recipe_id='" << recipe_id << "' AND ref_name='" << ref_name << "'";
  std::string val;

  if ( !sql_select_single_val(sql_ss.str(), val) ) {
    do_error( std::string("Query failed in AutonetworkerDBModel::find_group_id(): " + sql_ss.str()) );
    return false;
  }

  ret_val = atoi(val.c_str());
  return true;
}
 
bool
AutonetworkerDBModel::find_algo_id( const u_int recipe_id, const u_int algo_number, eLevel level, int& ret_val )
{
  std::stringstream sql_ss;
  sql_ss << "SELECT algo_id FROM algo_meta, algo_type WHERE recipe_id='" << recipe_id << "' AND algo_number='" << algo_number << "' AND algo_type.algo_level='" << (level == eLevel1 ? "1" : "2") << "' AND algo_meta.algo_type_id=algo_type.algo_type_id";
  
  std::string val;

  if ( !sql_select_single_val(sql_ss.str(), val) ) {
    do_error( std::string("Query failed in AutonetworkerDBModel::find_algo_id(): " + sql_ss.str()) );
    return false;
  }

  ret_val = atoi(val.c_str());
  return true;
}

bool
AutonetworkerDBModel::find_item_id( const int group_id, const int pin, int& ret_val )
{
  std::stringstream sql_ss;
  sql_ss << "SELECT item_id FROM item_meta WHERE group_id='" << group_id << "' AND pin='" << pin << "'";
  std::string val;

  if ( !sql_select_single_val(sql_ss.str(), val) ) {
    do_error( std::string("Query failed in AutonetworkerDBModel::find_item_id(): " + sql_ss.str()) );
    return false;
  }

  ret_val = atoi(val.c_str());
  return true;
}

#if 0
bool
AutonetworkerDBModel::delete_inspection( int insp_id )
{
  const std::string tablenames[] = { "recipe_meta" };
  int tablenames_size = sizeof(tablenames)/sizeof(tablenames[0]);							
  
  for( int i = 0; i < tablenames_size; i++ ) {
    std::stringstream del_query;
    del_query << "DELETE FROM " << tablenames[i] << " WHERE insp_id=" << insp_id;
    sql_query( del_query.str() );
  }
}
#endif

bool
AutonetworkerDBModel::escape_string( const std::string& src, std::string& targ )
{
  if ( src.empty() ) {
    return false;
  }

  u_int sz = src.size();
  u_int newsz = 3 * sz; // Just a guess.
  char * targ_chars = new char[newsz];
  ::memset( targ_chars, '\0', newsz );
  char * save = targ_chars;

  for (u_int i = 0; i < sz; i++) {
    
    if (src[i] == '\"') {
      *targ_chars = '\\';
      targ_chars++;
    }

    *targ_chars = src[i];
    targ_chars++;
  }

  targ = std::string( save );
  delete [] save;

  return true;
}

bool
AutonetworkerDBModel::exec_from_file( const std::string& fname ) 
{
  bool ok = true;
  std::ifstream in;
  in.open( fname.c_str(), std::ios::in );

  if (in.fail()) {
    std::stringstream msg;
    msg << "Failed opening \"" << fname << "\" for reading.";
    do_error(msg.str());
    ok = false;
    goto done;
  }

  static const u_int LINE_LEN = 4096;
  char buf[LINE_LEN];

  while( !in.getline(buf, LINE_LEN).eof() ) {

    std::string sql(buf);

    if (sql.empty()) {
      continue;
    }

    if ( !sql_query(sql) ) {
      do_error( "Query failed in AutonetworkerDBModel::exec_from_file()." );
      ok = false;
      goto done;
    }
  }

 done:

  if (in.is_open()) {
    in.close();
  }

  return ok;
}

std::vector<std::string>
AutonetworkerDBModel::table_names()
{
  
  std::vector<std::string> vec;

  vec.push_back("cfg_meta");
  vec.push_back("group_repair");
  vec.push_back("item_meta");
  vec.push_back("panel_results");
  vec.push_back("algo_meta");
  vec.push_back("cfg_value");
  vec.push_back("group_results");
  vec.push_back("item_repair");
  vec.push_back("recipe_meta");
  vec.push_back("algo_type");
  vec.push_back("group_meta");
  vec.push_back("insp_hdr");
  vec.push_back("item_results");
  vec.push_back("repair_hdr");

  return vec;
}

const std::string
AutonetworkerDBModel::dbname()
{
  return _dbname;
}

const std::string
AutonetworkerDBModel::dbtype()
{
  return _dbtype;
}

bool
AutonetworkerDBModel::reset_conn()
{
  return true;
}

void 
AutonetworkerDBModel::debug( bool d )
{
  _debug = d;
}
