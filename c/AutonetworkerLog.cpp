#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "libutil.h"
#include "AutonetworkerLog.h"

AutonetworkerLog*
AutonetworkerLog::get()
{
  static AutonetworkerLog instance;
  return &instance;
}

AutonetworkerLog::AutonetworkerLog()
{
  _debug_level = ANET_LOG;
  char* debug_level = getenv("ANETDEBUG");
  
  if (debug_level != NULL) {
    _debug_level = ANET_VERBOSE;
  }

  _anet_logfile = "/opt/mvp/elog/anet.log";
  _out.open( _anet_logfile.c_str(), std::ios::app );
  
  if (_out.fail()) {
    std::cerr << "Failed to open Autonetworker log file \"" << _anet_logfile << "\"." << std::endl;
  }
}

AutonetworkerLog::~AutonetworkerLog()
{
  if (_out.is_open()) {
    _out.close();
  }
}

std::string
AutonetworkerLog::get_timestamp()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);
  time_t time_sec = tp.tv_sec;
  time_t time_usec = tp.tv_usec;
  struct tm *clock = localtime(&time_sec);
  char buf[128];
  strftime(buf, sizeof(buf), "%F %T", clock);
  std::string date_string(buf);
  date_string += ".";
  date_string += itoa(time_usec);
  while (date_string.length() < 26) {
    date_string += "0";
  }
  return date_string;
}

void
AutonetworkerLog::write( const std::string& msg ) 
{
  write( msg, ANET_VERBOSE );
}

void
AutonetworkerLog::write( const std::string& msg, ANET_DEBUG_LEVEL elevel )
{
  if (msg.empty()) {
    std::cerr << "Message was empty in AutonetworkerLog::do_error()" << std::endl;
    return;
  }
  
  if (_out.fail()) {
    std::cerr << "Output stream was not valid in AutonetworkerLog::do_error()" << std::endl;
    return;
  }

  std::stringstream fullmsg;
  fullmsg << get_timestamp() << " " << msg << std::endl;

  // Write to log file.
  if ( (_debug_level & ANET_LOG) && (elevel & ANET_LOG) ) {
    _out << fullmsg.str();
    _out.flush();
  }

  // Write to console.
  if ( (_debug_level & ANET_DEBUG) && (elevel & ANET_DEBUG) ) {
    std::cerr << fullmsg.str();
  }
}
