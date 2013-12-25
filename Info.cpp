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

std::string to_str(long i) {
  std::ostringstream ostr;
  ostr << i;
  return ostr.str();
}

std::string to_str2(double i, int p) {
  std::ostringstream ostr;
  ostr << std::setiosflags(std::ios::fixed) << std::setprecision(p) << i;
  return ostr.str();
}

int parse_int(const std::string& s) { 
  int n;
  std::istringstream(s) >> n;
  return n;
}

// Default values if no config file found.
Info::Info() {
  status = 1;
  os     = NULL; // output stream

  start_immediately = 1;
  delta_hdg    =      4.0;
  delta_dfp    =   2000.0;
  newtrack_dfp =   5000.0; 
  delta_alt    =    200.0;
  format       =      1; //1=extended, non-standard, 2=standard GPX
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
	if ( lhs == "start_immediately" ) { // or auto_start
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
	} else if ( lhs == "format" ) {
	  format = parse_int( rhs.c_str() );
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
    write_outfile("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>");
    write_outfile( "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" xmlns:gpxx=\"http://www.garmin.com/xmlschemas/GpxExtensions/v3\" xmlns:gpxtpx=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\" creator=\"GPXLog for XPlane\" version=\"1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd http://www.garmin.com/xmlschemas/GpxExtensions/v3 http://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd http://www.garmin.com/xmlschemas/TrackPointExtension/v1 http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd\">");
    write_outfile("<trk>");
    write_outfile("<trkseg>");
  }
  if ( format == 2 ) {
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

  if ( (format == 1) || (format == 2) ) {
    ostr << "<trkpt lat=\"";
    ostr << std::setiosflags(std::ios::fixed) << std::setprecision(5) << p.lat;
    ostr << "\" lon=\"";
    ostr << std::setiosflags(std::ios::fixed) << std::setprecision(5) << p.lon;
    ostr << "\">" << std::endl;
    ostr << "<time>" << t << "</time>" << std::endl;
    ostr << "<ele>";
    ostr << std::setiosflags(std::ios::fixed) << std::setprecision(1) << p.alt; // m
    ostr << "</ele>" << std::endl;
  }
  if ( format == 1 ) {
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
  }
  if ( (format == 1) || (format == 2) ) {
    ostr << "</trkpt>";// << std::endl;
  }
    write_outfile( ostr.str() );
  }

void Info::close_track() {
  if ( (format == 1) || (format == 2) ) {
    write_outfile( "</trkseg></trk></gpx>" );
  }
}
