#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>

#include <stdlib.h>

#include "Info.h"

// ----------------------------------------------------------------------------
// Code
// ----------------------------------------------------------------------------

std::string trim(const std::string &t, const std::string &ws) {
  std::string res = t;
  size_t found;
  found = res.find_last_not_of(ws);
  if (found != std::string::npos) {
    res.erase(found+1);
  } else {
    res.clear();
  }
  return res;
}

Info::Info() {
  status = 1;
  start_immediately = 0;
  delta_hdg    =    4.0;
  delta_dfp    = 2000.0;
  newtrack_dfp = 5000.0; 
  delta_alt    =  200.0;
}

Info::~Info() {
}

void Info::read_file( const std::string& filename ) {
  std::ifstream file( filename.c_str() );
  if ( ! file ) {
    //std::cerr << "ERROR: cannot load file." << std::endl;
    set_status(-1);
    return;
  }

  std::string a_line;
  while( std::getline( file, a_line )) {
    if ( a_line.length() == 0 ) {
      continue;
    }
    if ( a_line.at(0) == '#' ) {
      continue;
    }

    size_t pos = a_line.find( ':', 0 );
    if ( pos != std::string::npos ) {
      std::string lhs = trim(a_line.substr( 0, pos ), " \t\r\n");
      std::string rhs = trim(a_line.substr( pos+1 ), " \t\r\n");
      if ( (lhs != "") && (rhs != "") ) {
	if ( lhs == "start_immediately" ) {
	  if ( rhs == "1" ) {
	    start_immediately = 1;
	  }
	} else if ( lhs == "delta_hdg" ) {
	  delta_hdg = strtod( rhs.c_str(), NULL );
	} else if ( lhs == "delta_dfp" ) {
	  delta_dfp = strtod( rhs.c_str(), NULL );
	} else if ( lhs == "delta_alt" ) {
	  delta_alt = strtod( rhs.c_str(), NULL );
	} else if ( lhs == "newtrack_dfp" ) {
	  newtrack_dfp = strtod( rhs.c_str(), NULL );
	} 
      }
    }
  }
  file.close();
  set_status(2);
}

