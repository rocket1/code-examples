#ifndef _ANET_DB_MODEL_H_
#define _ANET_DB_MODEL_H_

#include <string>
#include <vector>
#include <map>

#include "AnetUtil.h"
#include "AutonetworkerLog.h"

typedef enum { eLevel1 = 1, eLevel2 = 2 } eLevel;
typedef std::map<std::string, std::string> TABLE_VAL_MAP;
const int kInvalidId = -1;
const u_int kGroupMetaIDIndex      = 0;
const u_int kGroupMetaRefNameIndex = 1;

class AutonetworkerDBModel {
  
 public: 
  
  AutonetworkerDBModel(); 
  virtual ~AutonetworkerDBModel();

  virtual bool connect( const std::string&, const std::string&, const std::string&, const std::string& ) = 0;
  virtual bool reset_conn();

  virtual bool transaction_begin() = 0;
  virtual bool transaction_commit() = 0;
  virtual bool transaction_rollback() = 0;

  bool insert_recipe_meta( const TABLE_VAL_MAP& );
  bool insert_algo_type( const TABLE_VAL_MAP&, const std::vector<std::string>& );
  bool insert_algo_meta( const TABLE_VAL_MAP& );
  bool insert_group_meta( const TABLE_VAL_MAP& );
  bool insert_item_meta( const TABLE_VAL_MAP& );

  bool insert_insp_hdr( const TABLE_VAL_MAP& );
  bool insert_panel_results( const TABLE_VAL_MAP& );
  bool insert_group_results( const TABLE_VAL_MAP& );
  bool insert_item_results( const TABLE_VAL_MAP& );

  bool insert_cfg( const TABLE_VAL_MAP& );

  bool insert_repair_hdr( const TABLE_VAL_MAP& );
  bool insert_repair_group( const TABLE_VAL_MAP& );
  bool insert_repair_item( const TABLE_VAL_MAP& );

  bool update_insp_hdr( const TABLE_VAL_MAP& );
  
  virtual bool num_insp( const std::string&, u_int& ) = 0;
  virtual bool db_size( const std::string&, double& ) = 0; // In MB.

  virtual bool erase_all() = 0;
  virtual bool erase( const u_int retain, const std::string& machine ) = 0;

  const std::string dbname(); // Returns "anet", most likely.
  const std::string dbtype(); // Returns "mysql", "sqlite", etc...

  void debug( bool );
  
 protected:

  AutonetworkerLog* _log;

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Members common to both inspection and repair saving.
  //
  ////////////////////////////////////////////////////////////////////////////////

  bool _debug;

  // Id of last SQL insert.
  int _insert_id;

  // True if recipe meta exists for this inspection.
  bool _has_meta;
  
  int _curr_group_id;
  int _curr_item_id;
  int _curr_insp_id;
  int _curr_recipe_id;
  
  ////////////////////////////////////////////////////////////////////////////////
  //
  // Members for saving inspections.
  //
  ////////////////////////////////////////////////////////////////////////////////

  int _curr_algo_type_id;

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Members for saving repairs.
  //
  ///////////////////////////////////////////////////////////////////////////////

  int _curr_repair_id;

  // Current data store.
  std::string _dbname;
  std::string _dbtype;

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Functions common to both inspection and repair saving.
  //
  ////////////////////////////////////////////////////////////////////////////////

  bool do_error( const std::string& );
  bool delete_recipe( int );
  //bool delete_inspection( int );
  bool get_table_val( const std::string&, const TABLE_VAL_MAP&, std::string& );
  bool get_insert_stmt( const TABLE_VAL_MAP&, const std::string&, std::string& );
  bool get_update_stmt( const TABLE_VAL_MAP&, const std::string&, std::string& );
  bool exec_from_file( const std::string& fname );

  virtual bool sql_select( const std::string& ) = 0;
  virtual bool sql_select_single_val( const std::string&, std::string& ) = 0;
  virtual bool sql_query( const std::string& ) = 0;
  virtual bool sql_query_return_id( const std::string&, int& ) = 0;

  bool erase_val_map_key( const std::string& key, TABLE_VAL_MAP& val_map );
  bool escape_string( const std::string&, std::string& );

  bool find_recipe_id( const int insp_id, int& ret_val );
  bool find_insp_id( const std::string& serial, const std::string& centi_timestamp, int& ret_val );
  bool find_group_id( const int recipe_id, const std::string& ref_name, int& ret_val );
  bool find_item_id( const int group_id, const int pin, int& ret_val );
  bool find_algo_id( const u_int recipe_id, const u_int algo_number, eLevel level, int& retval );

  std::string int2str( int );

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Functions for saving inspections.
  //
  ////////////////////////////////////////////////////////////////////////////////
  
  virtual bool get_recipe_id( const std::string&, const std::string&, int& ) = 0;
  virtual bool get_algo_type_id( const std::string&, eLevel level, int& ) = 0;
  virtual bool get_next_id( eLevel, int& ) = 0;
  virtual bool load_ids( eLevel ) = 0;

  bool recipe_sql( TABLE_VAL_MAP, std::string& );
  bool insp_hdr_sql( TABLE_VAL_MAP, std::string& );
  bool panel_results_sql( TABLE_VAL_MAP, std::string& );
  bool group_meta_sql( TABLE_VAL_MAP, std::string& );
  bool group_results_sql( TABLE_VAL_MAP, std::string& );
  bool item_meta_sql( TABLE_VAL_MAP, std::string& );
  bool item_results_sql( TABLE_VAL_MAP, std::string& );
  bool algo_type_sql(  TABLE_VAL_MAP, std::string& );
  bool algo_iv_names_sql(  TABLE_VAL_MAP, std::string& );
  bool algo_meta_sql( TABLE_VAL_MAP, std::string& );
  bool cfg_desc_sql( TABLE_VAL_MAP, std::string& );
  bool cfg_value_sql( TABLE_VAL_MAP, std::string& );

  bool insert_algo_iv_names( const TABLE_VAL_MAP& );
  bool insert_cfg_desc( const TABLE_VAL_MAP& );
  bool insert_cfg_value( const TABLE_VAL_MAP& );

  virtual bool load_cfg_desc( std::map< std::string, u_int >& ret_map ) = 0;
  std::vector<std::string> table_names();

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Functions for saving repair data.
  //
  ////////////////////////////////////////////////////////////////////////////////
  
  bool repair_hdr_sql( const TABLE_VAL_MAP&, std::string& );
  bool group_repair_sql( TABLE_VAL_MAP, std::string& );
  bool item_repair_sql( TABLE_VAL_MAP, std::string& );
};

#endif
