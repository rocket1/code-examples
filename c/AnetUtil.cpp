#include <sstream>
#include <fstream>
#include <sys/types.h>	 // u_int
#include "AnetUtil.h"

static const u_int LINE_LEN = 4096;

bool
AnetUtil::read_file( const std::string& fname, std::string& contents )
{
  if (fname.empty()) {
    return false;
  }

  std::ifstream in( fname.c_str(), std::ios::in );

  if (in.fail()) {
    return false;
  }

  std::stringstream contents_ss;
  char buf[LINE_LEN];

  while( !in.getline(buf, LINE_LEN).eof() ) {
    contents_ss << buf << "\n";
  }

  contents = contents_ss.str();
  return true;
}
