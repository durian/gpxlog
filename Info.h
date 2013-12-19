#ifndef _INFO_H
#define _INFO_H

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

std::string trim(const std::string&, const std::string&);

class Info {
 private:
  int                     status;
  std::ofstream          *os;
  bool                    oo;

 public:
  int                     start_immediately;
  double                  delta_hdg;
  double                  delta_dfp;
  double                  newtrack_dfp; // not useful?
  double                  delta_alt;
  int                     format;
  
  std::string             outfilename;
  std::string             prefsfilename;
    
  // Constructor.
 Info();

  // Destructor.
  ~Info();

  int  get_status() { return status; }
  void set_status(int s) { status = s; }
  void read_prefs( const std::string& );
  void open_outfile( const std::string& fn );
  void write_outfile( const std::string& );
  void close_outfile();
  void flush_outfile();
};

#endif
