#ifndef _INFO_H
#define _INFO_H

#include <string>
#include <map>

// ----------------------------------------------------------------------------
// Class
// ----------------------------------------------------------------------------

std::string trim(const std::string&, const std::string&);

class Info {
 private:
  int                     status;
  std::map<std::string, std::string> kv; 

 public:
  int                     start_immediately;
  double                  delta_hdg;
  double                  delta_dfp;
  double                  newtrack_dfp; // not useful?
  double                  delta_alt;

  // Constructor.
  Info();

  // Destructor.
  ~Info();

  int  get_status() { return status; }
  void set_status(int s) { status = s; }
  void read_file( const std::string& );
  void add_kv( const std::string&, const std::string& );
  const std::string& get_value( const std::string& );
  const std::string& get_value( const std::string&, const std::string& );
};

#endif
