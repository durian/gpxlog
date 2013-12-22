#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>

#include <stdlib.h>

#include "gpxlog.h"
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

// Default values if no config file found.
Info::Info() {
  status = 1;
  os     = NULL; // output stream
  oo     = false; // output is open

  start_immediately = 0;
  delta_hdg    =      4.0;
  delta_dfp    =   2000.0;
  newtrack_dfp =   5000.0; 
  delta_alt    =    200.0;
  format       =      1;
}

Info::~Info() {
}

void Info::read_prefs( const std::string& filename ) {
  std::ifstream file( filename.c_str() );
  if ( ! file ) {
    //std::cerr << "ERROR: cannot load file." << std::endl;
    status = -1; //set_status(-1);
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
	std::string tmp = lhs +":"+rhs+"\n";
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

void Info::open_outfile( const std::string& fn ) {
  os = new std::ofstream( fn.c_str(), std::ios::out );

  if ( format == 1 ) {
    // write XML header
    write_outfile("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>");
    write_outfile("<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" creator=\"GPXLog for XPlane\" version=\"1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">");
    write_outfile("<trk>");
    write_outfile("<trkseg>");
  }
}
void Info::write_outfile( const std::string& l ) {
  if ( os && os->is_open() ) {
    (*os) << l << std::endl;
  }
}
void Info::close_outfile() {
  if ( os && os->is_open() ) {
    os->close();
  }
}
void Info::flush_outfile() {
  if ( os && os->is_open() ) {
    os->flush();
  }
}

void Info::write_geopos( const struct geopos& p, const std::string& t, double dfp, double tsd ) {
  std::ostringstream ostr;
  ostr << "<trkpt lat=\"";
  ostr << std::setiosflags(std::ios::fixed) << std::setprecision(5) << p.lat;
  ostr << "\" lon=\"";
  ostr << std::setiosflags(std::ios::fixed) << std::setprecision(5) << p.lon;
  ostr << "\">" << std::endl;
  ostr << "<time>" << t << "</time>" << std::endl;
  ostr << "<ele>";
  ostr << std::setiosflags(std::ios::fixed) << std::setprecision(1) << p.alt; // m
  ostr << "</ele>" << std::endl;
  ostr << "<hdg>";
  ostr << std::setiosflags(std::ios::fixed) << std::setprecision(1) << p.hdg;
  ostr << "</hdg>" << std::endl;
  ostr << "<gsp>";
  ostr << std::setiosflags(std::ios::fixed) << std::setprecision(1) << (p.gsp * 3.6); // m/s to km/h
  ostr << "</gsp>" << std::endl;
  ostr << "<dfp>";
  ostr << std::setiosflags(std::ios::fixed) << std::setprecision(1) << dfp; // m
  ostr << "</dfp>" << std::endl;
  ostr << "<tsd>";
  ostr << std::setiosflags(std::ios::fixed) << std::setprecision(1) << tsd; // m
  ostr << "</tsd>" << std::endl;
  ostr << "</trkpt>";// << std::endl;
  write_outfile( ostr.str() );
}
