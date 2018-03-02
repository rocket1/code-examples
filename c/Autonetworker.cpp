  #include <memory.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "AutonetworkerDBModel.h"
#include "AutonetworkerDBModel_SQLite.h"
#include "AutonetworkerDBModel_MySQL.h"
#include "Autonetworker.h"
#include "AnetTransaction.h"
#include "AnetException.h"

#include "libutil.h"
#include "cfg_class.h"
#include "fcntl.h"
#include "dbmacro.h"
#include "Hdr.H"
#include "Threshold.H"
#include "Results.H"
#include "Group.H"
#include "Item.H"
#include "Serial.H"
#include "Pparam.H"
#include "Inctrl.H"
#include "Netctrl.H"
#include "Alert.H"

extern create_l1_func Level1CreateFuncs[];
extern create_l2_func Level2CreateFuncs[];

extern long Level1CreateFuncsCount;
extern long Level2CreateFuncsCount;

static const std::string kCfgFile( "/etc/opt/mvp/syspar.d/defaults.res" );
static const char kIVNamesDelim = ' ';

bool
Autonetworker::connect()
{
  return connect( gCfg->an_hostname(),
		  gCfg->an_username(),
		  gCfg->an_password(),
		  gCfg->an_databasename() ); 
}

bool
Autonetworker::connect( const std::string& host, const std::string& user, const std::string& pass, const std::string& db)
{
    if ( ! _dbmodel ) {
        return false;
    }

    _log->write("AN enter connect");

    _host = host;
    _user = user;
    _pass = pass;
    _db   = db;
    
    eDatabaseTypes db_type = gCfg->an_database();

    if ( getenv("ANET_DRIVER") != NULL ) {
        
        std::string env_db_type = getenv("ANET_DRIVER");
        
        if ( env_db_type == "mysql" ) {
            db_type = kMySql;
        }
        else if ( env_db_type == "sqlite" ) {
            db_type = kSQLite;
        }
        else {
            return false;
        }
        
        if ( getenv("ANET_HOST") != NULL ) {
            _host = getenv("ANET_HOST");
        }
        
        if ( getenv("ANET_USER") != NULL ) {
            _user = getenv("ANET_USER");
        }
      
        if ( getenv("ANET_PASS") != NULL ) {
            _pass = getenv("ANET_PASS");
        }
        
        if ( getenv("ANET_DB") != NULL ) {
            _db = getenv("ANET_DB");
        }
    }    

    if ( db_type == kMySql ) {

        if ( _host.empty()) {
            err( "MySQL hostname was empty." );
        }
        if ( _user.empty()) {
            err( "MySQL username was empty." );
        }
        if ( _pass.empty()) {
            err( "MySQL password was empty."  );
        }
        if ( _db.empty()) {
            err( "MySQL database name was empty." );
        }
    }
    else if ( db_type == kSQLite ) {

      /*        if ( _db.empty()) {
            err( "SQLite database name was empty." );
	    }*/
    }

    bool ret =  _dbmodel->connect( _host, _user, _pass, _db );
    return ret;
}

bool
Autonetworker::trans( AnetMemFn func )
{
    _log->write("AN enter trans");
  try {
    
    AnetTransaction trans(this, _log);

    trans.begin();

    if ( !CALL_MEMBER_FN(*this, func)() ) {
      return false;
    }
    
    trans.commit();
    return true;
  }
  catch ( AnetExceptionConnFail& e ) {
    
    if (gAlert) {

      std::stringstream creds;
      
      creds << "Autonetworker Connect Failed\n\nCurrent Settings:\n\n"
	    << "  Host: " << _host << std::endl
	    << "  User: " << _user << std::endl
	    << "  Pass: <hidden>\n"
	    << "  DB:   " << _db;
      
      gAlert->do_alert( creds.str() );
    }

    return false;
  }
  catch ( AnetExceptionBrokenLink& e ) {
    
    err( e.what() );
    
    if ( _dbmodel ) {
        _dbmodel->reset_conn();
    }
    
    if (gAlert) {
    
      std::stringstream msg;
      msg << "The link to \"" << _host << "\" has gone away.\nMake sure this server is alive\nand your configuration settings are\ncorrect in Configuration->Autonetworker.\nWould you like to disable the Autonetworker?";
      //      gAlert->do_alert(msg.str());
    }

    return false;
  }
  catch ( std::exception& e ) {
  
    err( e.what() );
    
    if ( _dbmodel ) {
        _dbmodel->reset_conn();
    }
    
    return false;
  }
    return false;
}

bool
Autonetworker::save_inspection()
{ 
    _log->write("AN enter save_inspection");
  struct timeval time_begin;
  timer_start(&time_begin);
  
  if ( !do_preprocess() ) { //this will open the connection by calling trans()
    return false;
  }

  bool ok = trans( &Autonetworker::do_save );

  _save_duration = timer_stop(&time_begin);

  std::stringstream msg;
  double nsec = _save_duration / 1000000.0F;

  if (ok) {
    
    const Serial* s = Serial::get();
    const std::string serial = gCfg->syspar_panel_2d_bc_as_serial() ? s->serial_renamed() : s->serial();

    msg << "INSPECTION (" << nsec << " sec)"
	<< " Recipe: " << s->flavor()
	<< ", Serial: " << serial;
    
    std::string lot( Netctrl::get()->n_lot() );
    
    if (!lot.empty()) {
      msg << ", Lot: " << lot;
    }
    
    _log->write(msg.str());
  }

  return ok;
}

bool
Autonetworker::do_preprocess()
{
    _log->write("AN enter do_preprocess");
  // If we want to keep the database size small then we
  // we delete all but a certain amount of records each inspection.

  const u_int max_records = gCfg->an_max_inspection_records();

  if ( max_records > 0 ) {
    if ( !erase(max_records) ) {
      return false;
    }
  }

  // Let's find out what panels we want to completely skip.

  Hdr* esp = Hdr::get();
  register Group* Dg;
  register Item* Di;

  if (!esp) {
    return false;
  }

  for (u_int brd = 0; brd < esp->es_number_of_boards(); brd++) {

    ALLBRDGROUPS(brd, Dg) {
      
      if (!keep_this_group(Dg)) {
	continue;
      }

      if (!Dg->visible()) {
        continue;
      }
      
      const Results* Dr = Dg->results();
      
      if (!Dr || Dr->r_nresults() == 0) {
	continue;
      }
      
      int x, y;
      pid_from_ref( Dg->ref_name(), x, y );
      
      if (y < 1) {
	continue;
      }
      
      if (brd == 0) {
	if ( is_skip_group(Dg) && Dr->r_decision() < 0 ) {
	  _skip_fail_list.push_back(y);
	}
	else if ( is_fid_group(Dg) && Dr->r_decision() < 0 ) {
	  _fid_fail_list.push_back(y);
	}
      }
      
      ALLGRPITEMS(Dg, Di) {
	
	if (!Di->visible()) {
	  continue;
	}
	
	const Results* Dr = Di->results();
	
	if (!Dr || Dr->r_nresults() == 0) {
	  continue;
	}
	
	if ( Dr->type() == AlgoTypes::Datamatrix() ) {
	  
	  std::map<u_int, std::string>::iterator i = _panel_bc_map.find(y);
	  
	  if (i == _panel_bc_map.end()) {
	    std::string barcode;
	    Dr->datamatrix_string(barcode);
	    _panel_bc_map.insert( make_pair(y, barcode) );
	  }
	}
      }
    }
  }

  return true;
}

bool
Autonetworker::do_save() 
{
    _log->write("AN enter do_save");
  if (!save_recipe_meta()) {
    err( "Failed saving recipe meta." );
    return false;
  }

  if (!save_algo_meta(eLevel1)) {
    err( "Failed saving level 1 algo meta." );
    return false;
  }
  
  if (!save_algo_meta(eLevel2)) {
    err( "Failed saving level 2 algo meta." );
    return false;
  }
  
  if (!save_component_meta()) {
    err( "Failed saving component meta." );
    return false;
  }
  
  if (!save_insp_hdr()) {
    err( "Failed saving inspection header." );
    return false;
  }

  if (!save_panel_results()) {
    err( "Failed saving component results." );
    return false;
  }
  
  if (!save_component_results()) {
        err( "Failed saving component results." );
        return false;
  }
  
  if (!save_cfg()) {
    err( "Failed saving configuration." );
    return false;
  }
  
  return true;
}

bool
Autonetworker::save_recipe_meta()
{
    _log->write("AN enter save_recipe_meta");
  TABLE_VAL_MAP recipe_meta_val_map;
  
  std::string recipe_name = Serial::get()->flavor();
  recipe_meta_val_map["recipe_name"] = recipe_name;
  recipe_name += ".es";
  database_path(recipe_name);
  struct stat statb;

  if ( stat(recipe_name.c_str(), &statb) != 0 ) {
    err( std::string("Could not find recipe file: " + recipe_name) );
    return false;
  }
  
  std::stringstream mod_date_ss;
  mod_date_ss << statb.st_mtime;

  std::string recipe_file;

  /*  if ( !AnetUtil::read_file(recipe_name, recipe_file) ) {
    err( std::string( "Failed saving recipe file \"" + recipe_name + "\"." ) );
    return false;
    }*/

  recipe_meta_val_map["recipe_mod"] = mod_date_ss.str();
  recipe_meta_val_map["recipe_file"] = "";//recipe_file;

  return _dbmodel->insert_recipe_meta(recipe_meta_val_map);
}

bool
Autonetworker::save_algo_meta( eLevel algo_level )
{
    _log->write("AN enter save_algo_meta");
  Pparam* pparam = Pparam::get();
  const Threshold* Th;
  const threshold_data_t* Th_d;
  std::string algo_type;

  TABLE_VAL_MAP algo_meta_val_map;
  TABLE_VAL_MAP algo_type_meta_val_map;
  
  u_int algo_number = 0;
  u_int max_algos = kMaxAlgosPerLevel - 1; //don't include 1024 "Always Pass"
  
  while ( algo_number <= max_algos ) {
    
    switch (algo_level) {
      
    case eLevel2:
      Th = &(pparam->p_l2th(algo_number));
      break;
      
    case eLevel1:
      Th = &(pparam->p_l1th(algo_number));
      break;
      
    default:
      return false;
    }
    
    if (!Th) {
      err( "Threshold was NULL in Autonetworker::save_algo_meta()" );
      return false;
    }
    
    algo_type = Th->algo_name();
    
    if (algo_type.empty()) {
      algo_number++;
      continue;
    }

    std::vector<std::string> iv_names_vec;
    
    if ( !iv_names(algo_type, algo_level, iv_names_vec) ) {
      //err( std::string("Failed getting iv_names vector in Autonetworker::save_algo_meta() for algo type \"" + algo_type +"\".") );
      //return false;
    }
    
    algo_type_meta_val_map["algo_type"] = algo_type;
    algo_type_meta_val_map["algo_level"] = itoa(algo_level);
    
    if ( !_dbmodel->insert_algo_type(algo_type_meta_val_map, iv_names_vec) ) {
      err( "Failed inserting algo type in Autonetworker::save_algo_meta()." );
      return false;
    }
    
    if ( (Th_d = Th->thresh()) == NULL ) {
      err( "Threshold value vector was NULL in Autonetworker::save_algo_meta()" );
      return false;
    }
    
    std::stringstream thresh_values;
    u_int thresh_ndx = 0;
    u_short num_thresholds = Th->th_count();

    //    std::cerr << algo_type; 

    while (num_thresholds > 0) {

      const ThresholdListEntry* entry = Th->get_th(thresh_ndx);
      const char* n = entry->threshold_entry_name(); 
      //      std::cerr << " " << n;

      thresh_values << Th_d[thresh_ndx];
      if ( ++thresh_ndx >= num_thresholds )
	break;
      thresh_values << " ";
    }
    
    //    std::cerr << "\n";

    std::string algo_label = Th->user_name();
    std::string chain_to = itoa(Th->chain_to());

    algo_meta_val_map["algo_number"] = itoa(algo_number);
    algo_meta_val_map["algo_label"] = algo_label;
    algo_meta_val_map["chain_to"] = chain_to;
    algo_meta_val_map["threshold"] = thresh_values.str();
    
    if ( !_dbmodel->insert_algo_meta(algo_meta_val_map) ) {
      err( "Failed inserting algo meta in Autonetworker::save_algo_meta()" );
      return false;
    }
    
    algo_number++;
  }

  return true;
}

bool
Autonetworker::keep_this_group( const Group* Dg )
{
  if (!Dg) {
    return false;
  }

  // Filter out certain algo names, e.g. "linkskip" algo we don't care about.
  u_int algo_number = Dg->algorithm();
  const Pparam* pparam = Pparam::get();

  if (!pparam) {
    return false;
  }

  const Threshold* Th = &(pparam->p_l2th(algo_number));
  
  if (!Th) {
    return false;
  }

  const std::string algo_name(Th->algo_name());
  
  if (algo_name == "linkskip") {
    return false;
  }
  
  return true;
}

bool
Autonetworker::save_component_meta()
{
  Hdr* esp = Hdr::get();
  Wind win;
  register Group* Dg;
  register Item* Di;
  
    _log->write("AN enter save_component_meta");
  for (u_int brd = 0; brd < esp->es_number_of_boards(); brd++) {
    
    ALLBRDGROUPS(brd, Dg) {

      if (!keep_this_group(Dg)) {
	continue;
      }

      // NOTE: Here we DO NOT care if the component is invisible, like the
      // the other saving functions. We want ALL parts in the component meta.

      Dg->_wind.s2c(&win);
      
      std::string ref_name = Dg->ref_name();
      std::string type_name = Dg->type_name();
//      std::string stock_name = Dg->stock_name();
 //     std::string pipepette_name = Dg->pipepette_name;
 //     std::string package_type = Dg->package_type;
//      std::string place_nozzle = Dg->place_nozzle;
 //     std::string place_feeder = Dg->place_feeder;
      
      std::string x_loc = itoa(win.Wind_X);
      std::string y_loc = itoa(win.Wind_Y);
      std::string dx_loc = "";
      std::string dy_loc = "";
      std::string orient = itoa(Dg->orientation());
      std::string algo_number = itoa(Dg->algorithm());
      std::string psr_panel = itoa(Dg->psr_panel());
      int panel_x;
      int panel_y;
      pid_from_ref( ref_name, panel_x, panel_y );
      std::string pass = itoa(brd);
      std::string is_visible = Dg->visible() ? "1" : "0";
      std::string part_family = itoa(Dg->ptClass()); 

      TABLE_VAL_MAP grp_meta_val_map;

      grp_meta_val_map["ref_name"] = ref_name;
      grp_meta_val_map["part_type"] = type_name;
//      grp_meta_val_map["stock_name"] = stock_name;
//grp_meta_val_map["pipepette_name"] = pipepette_name;
//grp_meta_val_map["package_type"] = package_type;
//grp_meta_val_map["place_nozzle"] = place_nozzle;
//grp_meta_val_map["place_feeder"] = place_feeder;
      grp_meta_val_map["x_loc"] = x_loc;
      grp_meta_val_map["y_loc"] = y_loc;
      grp_meta_val_map["dx_loc"] = dx_loc;
      grp_meta_val_map["dy_loc"] = dy_loc;
      grp_meta_val_map["algo_number"] = algo_number;
      grp_meta_val_map["orient"] = orient;
      grp_meta_val_map["psr_panel"] = psr_panel;
      grp_meta_val_map["panel_x"] = itoa(panel_x);
      grp_meta_val_map["panel_y"] = itoa(panel_y);
      grp_meta_val_map["pass"] = pass;
      grp_meta_val_map["visible"] = is_visible;
      grp_meta_val_map["part_family"] = part_family;

      if ( !_dbmodel->insert_group_meta(grp_meta_val_map) ) {
	return false;
      }
      
      ALLGRPITEMS(Dg, Di) {

	Di->_wind.s2c(&win);

	std::string pin = itoa(Di->pinnum());
	std::string x_loc = itoa(win.Wind_X);
	std::string y_loc = itoa(win.Wind_Y);
	std::string dx_loc = "";
	std::string dy_loc = "";
	std::string algo_number = itoa(Di->algorithm());
	std::string orient = itoa(Di->orientation());
	std::string is_fiducial = itoa(Di->fiducial());
	std::string is_visible = Di->visible() ? "1" : "0";
	std::string item_type = itoa(Di->item_type());

	TABLE_VAL_MAP item_meta_val_map;

	item_meta_val_map["pin"] = pin;
	item_meta_val_map["x_loc"] = x_loc;
	item_meta_val_map["y_loc"] = y_loc;
	item_meta_val_map["dx_loc"] = dx_loc;
	item_meta_val_map["dy_loc"] = dy_loc;
	item_meta_val_map["algo_number"] = algo_number;
	item_meta_val_map["orient"] = orient;
	item_meta_val_map["is_fiducial"] = is_fiducial;
	item_meta_val_map["visible"] = is_visible;
	item_meta_val_map["item_type"] = item_type;

	if ( !_dbmodel->insert_item_meta(item_meta_val_map) ) {
	  return false;
	}
      }
    }      
  }

  return true;
}

bool
Autonetworker::save_insp_hdr()
{
    _log->write("AN enter save_insp_hdr");
  const Serial* s = Serial::get();
  const std::string serial = gCfg->syspar_panel_2d_bc_as_serial() ? s->serial_renamed() : s->serial();

  struct timeval tp;
  gettimeofday(&tp, NULL);
  std::string time_of_insp = itoa(tp.tv_sec);
  std::string cycle_time = itoa(Inctrl::get()->i_cycle_time);

  std::string oper;

  if ( !get_op_name(oper) ) {
    err( "Failed getting operator name in Autonetworker::save_insp_hdr()" );
    return false;
  }

  char mach_c_str[255];
  gethostname(mach_c_str, 255);
  std::string machine = mach_c_str[0] == '\0' ? std::string() : std::string(mach_c_str);

  std::string line(gCfg->line_name());
  std::string lot_id = Netctrl::get()->n_lot();
  
  if (lot_id.empty()) {
    lot_id = "0";
  }

  std::string cfg_file;

  if ( !AnetUtil::read_file(kCfgFile, cfg_file) ) {
    err( std::string( "Failed saving cfg file \"" + kCfgFile + "\"." ) );
    return false;
  }

  TABLE_VAL_MAP insp_hdr_val_map;

  insp_hdr_val_map["serial"] = serial;
  insp_hdr_val_map["time_of_insp"] = time_of_insp;
  insp_hdr_val_map["cycle_time"] = cycle_time;
  insp_hdr_val_map["operator"] = oper;
  insp_hdr_val_map["machine"] = machine;
  insp_hdr_val_map["line"] = line;
  insp_hdr_val_map["lot_id"] = lot_id;
  insp_hdr_val_map["lane"] = lane();
  insp_hdr_val_map["centi_timestamp"] = _centi_timestamp;
  insp_hdr_val_map["cfg_file"] = "";//cfg_file;

  // We will update thesee after the groups are tallied.
  insp_hdr_val_map["passed"] = "";
  insp_hdr_val_map["ngroup_fails"] = "";

  return _dbmodel->insert_insp_hdr(insp_hdr_val_map);
}

bool
Autonetworker::save_component_results()
{
    _log->write("AN enter save_component_results");
  Hdr* esp = Hdr::get();
  Results* Dr;
  register Group* Dg;
  register Item* Di;
  
  bool passed = true;
  u_int ngroup_fails = 0;

  for (u_int brd = 0; brd < esp->es_number_of_boards(); brd++) {
    
    ALLBRDGROUPS(brd, Dg) {

      if (!keep_this_group(Dg)) {
	continue;
      } 

      if (!Dg->visible()) {
	continue;
      }

      Dr = Dg->results();

      if (!Dr || Dr->r_nresults() == 0) {
	std::stringstream msg;
	msg << "Results were NULL for group: " << Dg->ref_name() << std::endl;
	err( msg.str() );
	return false;
      }

      std::string result_vector;

      if ( !res2str(Dr, result_vector) ) {
	err( "Failed converting group results to string in Autonetworker::save_component_results()" );
	return false;
      }

      const std::string ref_name( Dg->ref_name() );
      const std::string decision( itoa(Dr->r_decision()) );
      const std::string condition( itoa(Dr->component_condition()) );
      
      TABLE_VAL_MAP group_results_val_map;

      group_results_val_map["ref_name"] = ref_name;
      group_results_val_map["decision"] = decision;
      group_results_val_map["condition"] = condition;
      group_results_val_map["result_vector"] = result_vector; 

      if ( !_dbmodel->insert_group_results(group_results_val_map) ) {
	return false;
      }

      if ( Dr->r_decision() < 0 ) {
	passed = false;
	++ngroup_fails;
      }
      //No need to save the item's variable data when this flag is 
      //turned off.
      if ( !gCfg->an_save_variable_data()) {
          continue;
      }

      ALLGRPITEMS(Dg, Di) {

	if (!Di->visible()) {
	  continue;
	}

	Dr = Di->results();

	if (!Dr) {
	  std::stringstream msg;
	  msg << "Results were NULL for item pin: " << Di->pinnum() << " of group: " << Dg->ref_name() << std::endl;
	  err( msg.str() );
	  return false;
	}
	
	std::string result_vector;

	if ( !res2str(Dr, result_vector) ) {
	  return false;
	}
	
	std::string decision = itoa(Dr->r_decision());
	std::string datamatrix;
	Dr->datamatrix_string(datamatrix);
	std::string ocr;
	Dr->ocr_string(ocr);

	TABLE_VAL_MAP item_results_val_map;

	item_results_val_map["result_vector"] = result_vector;
	item_results_val_map["decision"] = decision;
	item_results_val_map["datamatrix"] = datamatrix;
	item_results_val_map["ocr"] = ocr;

	if ( !_dbmodel->insert_item_results(item_results_val_map) ) {
	  return false;
	}
      }
    }
  }

  TABLE_VAL_MAP update_val_map;
  update_val_map["passed"] = itoa(passed);
  update_val_map["ngroup_fails"] = itoa(ngroup_fails);

  if ( !_dbmodel->update_insp_hdr(update_val_map) ) {
    return false;
  }
  
  return true;
}

bool
Autonetworker::save_panel_results()
{
    _log->write("AN enter save_panel_results");
  bool gOcrAsBarcode = false;

  Hdr* esp = Hdr::get();
  Results* Dr;
  register Group* Dg;
  register Item* Di;

  typedef boost::tuple<int, int> Tuple;
  std::vector< Tuple > found_panels;

  for (u_int brd = 0; brd < esp->es_number_of_boards(); brd++) {

    Tuple last_panel( -999, -999 );
    
    ALLBRDGROUPS(brd, Dg) {

      if (!keep_this_group(Dg)) {
        continue;
      }

      if (!Dg->visible()) {
        continue;
      }

      int px, py;

      if ( !pid_from_ref( Dg->ref_name(), px, py ) ) {
	err( "Failed getting pid from ref in Autonetworker::save_panel_results()" );
	return false;
      }

      // By here px and py will be equal to an integer => 0 or -1, not -999 (invalid).
      // Should force at least one panel results to be saved, even without panels specified.

      Tuple this_panel( px, py );
      
      if (this_panel != last_panel) {

	std::vector< Tuple >::iterator tuple_i = std::find( found_panels.begin(), found_panels.end(), this_panel );

	if (tuple_i != found_panels.end()) {
	  // If we found the panel, don't bother inserting it again.
	  //continue;
	}

	found_panels.push_back(this_panel);

	std::string psr_panel = itoa(Dg->psr_panel());
	std::string condition = itoa(Pass::get_pass(brd)->panel_cond(brd));
	std::string panel_x = itoa(px);
	std::string panel_y = itoa(py);
	std::string pass = itoa(brd);

	std::vector<u_int>::iterator skip_i = std::find( _skip_fail_list.begin(), _skip_fail_list.end(), static_cast<u_int>(py) );
	std::vector<u_int>::iterator fid_i = std::find( _fid_fail_list.begin(), _fid_fail_list.end(), static_cast<u_int>(py) );

	bool skip_fail = skip_i != _skip_fail_list.end();
	bool fid_fail = fid_i != _fid_fail_list.end();
	
	std::string barcode;
	std::map<u_int, std::string>::iterator bc_i = _panel_bc_map.find(py);

	if (bc_i != _panel_bc_map.end()) {
	  barcode = bc_i->second;
	}

	TABLE_VAL_MAP panel_results_val_map;
	 
	panel_results_val_map["psr_panel"] = psr_panel;
	panel_results_val_map["barcode"] = barcode;
	panel_results_val_map["condition"] = condition;
	panel_results_val_map["panel_x"] = panel_x;
	panel_results_val_map["panel_y"] = panel_y;
	panel_results_val_map["pass"] = pass;
	panel_results_val_map["skip_fail"] = skip_fail ? "1" : "0";
	panel_results_val_map["fid_fail"] = fid_fail ? "1" : "0";
    /*  // We will update these after the groups are tallied.
  panel_results_val_map["passed"] = "";
  panel_results_val_map["ngroup_fails"] = "";*/

	if ( !_dbmodel->insert_panel_results(panel_results_val_map) ) {
	  err( "Failed inserting panel results in Autonetworker::save_panel_results()" );
	  return false;
	}

	barcode.clear();
      }

      last_panel = this_panel;
    }
  }
  return true;
}

bool
Autonetworker::save_cfg()
{
  std::ifstream cfg_in( kCfgFile.c_str(), std::ios::in );

  if (cfg_in.fail()) {
    std::stringstream msg;
    msg << "Failed opening \"" << kCfgFile << "\" for reading.";
    err(msg.str());
  }

    _log->write("AN enter save_cfg");
  TABLE_VAL_MAP cfg_val_map;
  static const u_int LINE_LEN = 4096;
  char buf[LINE_LEN];

  while( !cfg_in.getline(buf, LINE_LEN).eof() ) {

    std::string bufstr(buf);
    size_t colon_pos = bufstr.find(":");
    static const size_t cfg_name_begin = 2; // skip *. in *.CfgBarcode2_has_dual_readers: False 

    std::string cfg_name = bufstr.substr( cfg_name_begin, colon_pos - 2 );
    std::string cfg_value = bufstr.substr( colon_pos + 1 );
    btrim(cfg_name);
    btrim(cfg_value);

    cfg_val_map[cfg_name] = cfg_value;
  }

  if ( !_dbmodel->insert_cfg(cfg_val_map) ) {
    err("Failed saving cfg.");
    return false;
  }
  
  return true;
}

bool
Autonetworker::res2str(Results* Dr, std::string& ret_val)
{
  ret_val = std::string();

  if (!Dr) {
    return false;
  }

  if (Dr->r_nresults() == 0) {
    return true;
  }

  std::stringstream res_ss;
  size_t res = 0;

  while (1) {
    
    ResultsData& rd = (*Dr)[res];
    size_t upper_limit = static_cast<size_t>( MIN(rd.rd_nresults(), kMaxAlgoThresholds) );
    size_t n = 0;
    
    while (upper_limit > 0) {

      if (_rand_fudge) {
	int fudge = static_cast<int>(((double)rand()/(double)RAND_MAX)*100);
	if (time(NULL)%2) {
	  fudge = -fudge;
	}
	int val = rd[n] + fudge;
	res_ss << val;
      }
      else {
	res_ss << rd[n];
      }

      if ( ++n == upper_limit )
	break;
      res_ss << " ";
    }
    if (++res >= Dr->r_nresults())
      break;
    res_ss << " & "; // chain delimiter
  }

  ret_val = res_ss.str();
  return true;
}

bool
Autonetworker::erase_all()
{
  _log->write("Erasing all data...");
   
  if ( !trans( &Autonetworker::do_erase_all ) ) {
    err( "Failed erasing data." );
    return false;
  }

  _log->write("All data has been erased.");
  return true;
}

bool
Autonetworker::do_erase_all()
{
  return _dbmodel->erase_all();
}

bool
Autonetworker::do_erase()
{
  char mach_c_str[255];
  gethostname(mach_c_str, 255);
  std::string machine = mach_c_str[0] == '\0' ? std::string() : std::string(mach_c_str);

  return _dbmodel->erase( gCfg->an_max_inspection_records(), machine );
}

bool
Autonetworker::erase( const u_int retain )
{
  if ( !trans( &Autonetworker::do_erase ) ) {
    err( "Failed erasing data." );
    return false;
  }

  // no log message.
  return true;
}

void
Autonetworker::debug( bool d )
{
  _debug = d;
}

bool
Autonetworker::debug()
{
  return _debug;
}

bool
Autonetworker::err( const std::string& msg )
{
  if (_debug) {
    std::cerr << msg << std::endl;
  }

  _log->write(msg);
  return true;
}

Autonetworker::Autonetworker() :
    _debug(getenv("ANET_DEBUG") != NULL),
    _dbmodel(NULL),
    _log(AutonetworkerLog::get())
{
    eDatabaseTypes db_type = gCfg->an_database();

    if ( getenv("ANET_DRIVER") != NULL ) {
        
        std::string env_db_type = getenv("ANET_DRIVER");
        
        if ( env_db_type == "mysql" ) {
            db_type = kMySql;
        }
        else if ( env_db_type == "sqlite" ) {
            db_type = kSQLite;
        } 
    }

    switch ( db_type ) {

        case kNoDatabase:
            break;

        case kMySql:
            _dbmodel = new AutonetworkerDBModel_MySQL();
            break;

        case kSQLite:
            _dbmodel = new AutonetworkerDBModel_SQLite();
            break;

        default:
            break;
    }

    if ( ! _dbmodel ) {
        err( "Error initializing database model." );
        return;
    }

    _dbmodel->debug(_debug);
    _centi_timestamp.clear();
    _rand_fudge = false;
    _save_duration = 0;
    _repair_file.clear();
    srand( time(NULL) );
}

Autonetworker::~Autonetworker()
{
  if (_dbmodel) {
    delete _dbmodel;
    _dbmodel = NULL;
  }
}

bool
Autonetworker::get_op_name( std::string& ret_val )
{
  ret_val.clear();

  if (gCfg->fc_user_prompt_enabled()) {
    ret_val = gCfg->fc_UID();
    return true;
  }
  
  struct passwd *pw = getpwuid(getuid());
  
  if (!pw) {

    std::string errmsg( "Unknown Error Code" );

    switch (errno) {
    case EINTR:
      errmsg = "A signal was caught.";
      break;
    case EIO:
      errmsg = "I/O error.";
      break;
    case EMFILE:
      errmsg = "The maximum number (OPEN_MAX) of files was open already in the calling process.";
      break;
    case ENFILE:
      errmsg = "The maximum number of files was open already in the system.";
      break;
    case ENOMEM:
      errmsg = "Insufficient memory to allocate passwd structure.";
      break;
    case ERANGE:
      errmsg = "Insufficient buffer space supplied.";
      break;
    default:
      break;
    }

    std::stringstream msg;
    msg << "getpwuid failed. errno: " << errno << " " << errmsg;
    err( msg.str() );

    return false;
  }

  std::stringstream op_name;
  op_name << pw->pw_name;
  ret_val = op_name.str();
  return true;
}

bool
Autonetworker::iv_names( const std::string& algo_type, eLevel level, std::vector<std::string>& ret_vec )
{
  ret_vec.clear();

  if (algo_type.empty()) {
    err( "Algo type was empty." );
    return false;
  }

  switch (level) {
    
  case eLevel1:
    if ( !iv_names_l1(algo_type, ret_vec) ) {
      return false;
    }
    return true;
    
  case eLevel2:
    if ( !iv_names_l2(algo_type, ret_vec) ) {
      break;
    }
    return true;
    
  default:
    break;
  }

  return false;
}

bool
Autonetworker::iv_names_l1( const std::string& algo_name, std::vector<std::string>& ret_vec )
{
  for ( int i = 0; i < Level1CreateFuncsCount; ++i ) {

    AlgoLevel1* algo = Level1CreateFuncs[i]();

    if (!algo) {
      err( "AlgoLevel1 was NULL in Autonetworker::iv_names_l1()." );
      return false;
    }

    if ( strncmp(algo->algo_name(), algo_name.c_str(), algo_name.length()) == 0 ) {

      size_t n = algo->result_names_count();

      if (n < 1) {
	return false;
      }
      
      u_int iv_ndx = 0;

      while (n--) {
	ret_vec.push_back(algo->result_names(iv_ndx));
	iv_ndx++;
      }
      return true;
    }
  }

  const std::string msg("The algo \"" + algo_name + "\" was not found in Autonetworker::iv_names_l1()");
  err(msg);
  return false;
}

bool
Autonetworker::iv_names_l2( const std::string& algo_name, std::vector<std::string>& ret_vec )
{
  for ( int i = 0; i < Level2CreateFuncsCount; ++i ) {

    AlgoLevel2* algo = Level2CreateFuncs[i]();

    if (!algo) {
      err( "AlgoLevel2 was NULL." );
      return false;
    }

    if ( strncmp(algo->algo_name(), algo_name.c_str(), algo_name.length()) == 0 ) {

      size_t n = algo->result_names_count();

      if (n < 1) {
        err( std::string("The algo \"" + algo_name + "\" had no IV Names.") );
        return false;
      }
      
      u_int iv_ndx = 0;

      while (n--) {
	ret_vec.push_back(algo->result_names(iv_ndx));
	iv_ndx++;
      }

      return true;
    }
  }

  const std::string msg("The algo \"" + algo_name + "\" was not found in Autonetworker::iv_names_l2()");
  err(msg);
  return false;
}

void
Autonetworker::lane(lane_type_t lane)
{
  _lane = lane;
}

std::string
Autonetworker::lane()
{
  return _lane == lane_type_t::FrontLane() ? "Front Lane" : "Back Lane";
}

void
Autonetworker::centi_timestamp( const std::string& ts )
{
  _centi_timestamp = ts;
}

bool
Autonetworker::rand_fudge( bool rf ) 
{
  _rand_fudge = rf;
  return true;
}

void
Autonetworker::timer_start( struct timeval* start_time )
{
  gettimeofday(start_time, NULL);
}

long
Autonetworker::timer_stop( struct timeval* start_time )
{
  struct timeval end_time;
  gettimeofday(&end_time, NULL);

  long secs = end_time.tv_sec - start_time->tv_sec; // not reported
  long usec = end_time.tv_usec - start_time->tv_usec;

  if (usec < 0) {
    secs--;
    usec += 1000000;
  }

  return usec;
}

long
Autonetworker::save_duration()
{
  return _save_duration;
}

bool
Autonetworker::is_gonogo( const Group* Dg )
{
  if (!Dg) {
    return false;
  }

  const Results* Dr = Dg->results();

  if (!Dr) {
    return false;
  }

  return Dr->type() == AlgoTypes::GoNoGo();
}

bool
Autonetworker::substr_match( const std::string& needle, const std::string& haystack, bool ignore_case )
{
  if ( needle.empty() || haystack.empty() ) {
    return false;
  }

  std::string needle_copy(needle);
  std::string haystack_copy(haystack);

  if (ignore_case) {
    std::transform(needle_copy.begin(), needle_copy.end(), needle_copy.begin(), ::toupper);
    std::transform(haystack_copy.begin(), haystack_copy.end(), haystack_copy.begin(), ::toupper);
  }

  size_t i = haystack_copy.find(needle_copy);
  return i != std::string::npos;
}

bool
Autonetworker::is_skip_group( const Group* Dg )
{
  return Dg && is_gonogo(Dg) && substr_match( "skip", Dg->ref_name(), true );
}

bool
Autonetworker::is_fid_group( const Group* Dg )
{
  return Dg && is_gonogo(Dg) && substr_match( "fid", Dg->ref_name(), true );
}


////////////////////////////////////////////////////////////////////////////////
//
// R E P A I R
//
////////////////////////////////////////////////////////////////////////////////
void
Autonetworker::repair_file( const std::string& fname )
{
  _repair_file = fname;
}
  
bool
Autonetworker::save_repair()
{
  _log->write("AN enter save_repair");
  struct timeval time_begin;
  timer_start(&time_begin);

  bool ok = trans( &Autonetworker::do_save_repair );

  _save_duration = timer_stop(&time_begin);
  std::stringstream msg;
  double nsec = _save_duration / 1000000.0F;
  msg << "REPAIR";

  if (!ok) {
    msg << " [FAILED]";
  }

  msg << " (" << nsec << " sec)";
  _log->write(msg.str());
  return ok;
}

bool
Autonetworker::do_save_repair()
{
  if (!load_repair_file()) {
    err( "Failed loading repair file." );
    return false;
  }

  if (!save_repair_hdr()) {
    err( "Failed saving repair header." );
    return false;
  }
  
  if (!save_component_repair()) {
    err( "Failed saving component repair data." );
    return false;
  }
  
  return true;
}

bool
Autonetworker::load_repair_file()
{
  if ( _repair_file.empty() ) {
    err( "Repair file name was empty." );
    return false;
  }

  bool read_failed = _insp.read(_repair_file);

  if (read_failed) {
    err( std::string("Failed reading repair file \"" + _repair_file + "\".") );
    return false;
  }

  _repair_data = _insp.get_repair_data();
  _hdr_data = _insp.get_header(0);

  if ( _repair_data.num_groups() < 1 ) {
    err( std::string( "No repair data found in error file \"" + _repair_file + "\"." ));
    return false;
  }

  return true;
}

bool
Autonetworker::save_repair_hdr()
{
  TABLE_VAL_MAP val_map;

  const std::string serial = _hdr_data.serial_number();
  const std::string machine = _repair_data.ag_name();
  const std::string oper = _repair_data.ag_operator();
  const std::string time_of_repair = _repair_data.time_of_repair();
  const std::string centi_timestamp = _hdr_data.centi_timestamp();

  val_map["serial"] = serial;
  val_map["machine"] = machine;
  val_map["operator"] = oper;
  val_map["time_of_repair"] = time_of_repair;
  val_map["centi_timestamp"] = centi_timestamp;

  if ( !_dbmodel->insert_repair_hdr(val_map) ) {
    return false;
  }

  return true;
}

bool
Autonetworker::save_component_repair()
{
  const GroupRepairData* grp;
  const ItemRepairData*  itm;

  int ngrp = _repair_data.num_groups();

  for (int grp_ndx = 0; grp_ndx < ngrp; ++grp_ndx) {
    
    grp = &( _repair_data.get_repair_group(grp_ndx) );

    std::string ref_name = grp->name();
    std::string rcode;
    std::string scode;
    std::string rstring;
    std::string sstring;

    if ( !repair_codes(*grp, eRepairCodes, rcode, rstring) ) {
      err( "Failed getting group repair code." );
      return false;
    }

    if ( !repair_codes(*grp, eSuggestedCodes, scode, sstring) ) {
      err( "Failed getting group suggested code." );
      return false;
    }

    TABLE_VAL_MAP val_map;

    val_map["ref_name"] = ref_name;
    val_map["repair_code"] = rcode;
    val_map["suggested_code"] = scode;
    val_map["repair_string"] = rstring;

    if ( !_dbmodel->insert_repair_group(val_map) ) {
      err( "Failed inserting group repair." );
      return false;
    }
    
    int nitm = grp->num_items();

    for (int itm_ndx = 0; itm_ndx < nitm; ++itm_ndx) {

      itm = &( grp->get_item(itm_ndx) );

      TABLE_VAL_MAP val_map;
      
      std::string pin_num = itoa(itm->pin_number());
      std::string bpin = "";
      std::string rcode;
      std::string scode = "";
      std::string rstring("");
      std::string sstring("");

      if ( !repair_codes(*itm, eRepairCodes, rcode, rstring) ) {
	err( "Failed getting item repair code." );
	return false;
      }

      if ( !repair_codes(*itm, eSuggestedCodes, scode, sstring) ) {
	err( "Failed getting item suggested code." );
	return false;
      }

      val_map["pin_num"] = pin_num;
      val_map["bridging_pin"] = bpin;
      val_map["repair_code"] = rcode;
      val_map["suggested_code"] = scode;
      val_map["repair_string"] = rstring;

      if ( !_dbmodel->insert_repair_item(val_map) ) {
	err( "Failed inserting item repair." );
	return false;
      }
    }
  }

  return true;
}

bool
Autonetworker::repair_codes( const GroupRepairData& ird, eRepairCodeType rc_type, std::string& ret_val , std::string& repairString)
{
    ret_val.clear();
    std::stringstream repair_codes;
    std::stringstream repair_string;

    switch (rc_type)
    {
	case eRepairCodes:
	    repair_codes << ird.get_repair_code();
	    repair_string << ird.get_repair_string();
	    break;
	case eSuggestedCodes:
	    // Nothing yet.
	    break;
	default:
	    err( "Invalid result code type." );
	    return false;
	    break;
    }

    ret_val = repair_codes.str();
    repairString = repair_string.str();
    return true;
}

bool
Autonetworker::repair_codes( const ItemRepairData& ird, eRepairCodeType rc_type, std::string& ret_val , std::string& repairString)
{
    ret_val.clear();
    std::stringstream repair_codes;
    std::stringstream repair_string;

    switch (rc_type)
    {
	case eRepairCodes:
	  repair_codes << ird.get_repair_code();
	  repair_string << ird.get_repair_string();
	  break;
	case eSuggestedCodes:
	  // Nothing yet.
	  break;
	default:
	  err( "Invalid result code type." );
	  return false;
	  break;
    }

    ret_val = repair_codes.str();
    repairString = repair_string.str();
    return true;
}

const std::string
Autonetworker::db_enum_to_str( eDatabaseTypes dbtype ) 
{
  switch(dbtype) {

  case kNoDatabase:
    return "None";

  case kMySql:
    return "MySQL";

  case kSQLite:
    return "SQLite";

  case kOracle:
    return "Oracle";

  default:
    break;
  }

  return "Bad database type.";
}

AutonetworkerDBModel*
Autonetworker::get_dbmodel()
{
  return _dbmodel;
}

double
Autonetworker::db_size()
{
  double ret;
  
  if ( _dbmodel->db_size(_db, ret) ) {
    return ret;
  }

  return -1;
}
