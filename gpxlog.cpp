/*
  (c) pjb 2013
*/

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#if APL
#if defined(__MACH__)
#include <Carbon/Carbon.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFURL.h>
#endif
#endif

#include "XPLMProcessing.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "XPLMPlugin.h"
#include "XPLMMenus.h"

#include "gpxlog.h"
#include "Info.h"

std::string to_str(long i) {
  std::ostringstream ostr;
  ostr << i;
  return ostr.str();
}
std::string to_str(double i, int p) {
  std::ostringstream ostr;
  ostr << std::setiosflags(std::ios::fixed) << std::setprecision(p) << i;
  //ostr << i;
  return ostr.str();
}

/* distance between points */
static double distanceto(double lat0, double lon0, double lat1, double lon1) {
  double slat = sin((lat1-lat0) * (double)(M_PI/360));
  double slon = sin((lon1-lon0) * (double)(M_PI/360));
  double aa   = slat*slat + cos(lat0 * (double)(M_PI/180)) * cos(lat1 * (double)(M_PI/180)) * slon * slon;
  return 6378145.0 * 2 * atan2(sqrtf(aa), sqrt(1-aa));
}

#if APL && __MACH__
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen);
char outputPath2[255];
int Result = 0;
#endif

Info* info = new Info(); // config settings, TODO: read from file. Globals here

FILE *gOutputFile; // Global gpx file 
char gOutputPath[255]; // Global System path to gpx file 
enum gpxlog_status gGPXStatus; // Status GPXLOG_OFF, GPXLOG_ON 
time_t t;            // Time
struct tm *plugin_t; // time in xplane
char timeoutstr[32];

std::string version = "1.0";

char  filebase[255];
int   result = 0;

// "Previous" logged point
double prev_lat =  1000;
double prev_lon =  1000;
double prev_alt = -1;
double prev_hdg = -1;
double prev_gsp = -1;
int    prev_psd = -1;
int    prev_spd = -1;

double t_dist = 0.0; // Total track distance

XPLMMenuID	myMenu;
int		mySubMenuItem;

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {
  strcpy(outName, "GXPLog");
  strcpy(outSig,  "durian.gpxlog");
  strcpy(outDesc, "A plugin that outputs a GPX log.");

  XPLMDebugString( "Starting gpxlog.\n" );
  
  XPLMGetSystemPath(filebase); // Locate the X-System directory
  XPLMDebugString( filebase );
  XPLMDebugString( "\n" );

  std::string sep = std::string(XPLMGetDirectorySeparator());
  std::string prefsfile = std::string(filebase) + "Resources" + sep + "plugins" + sep + "gpxlog.ini";

  info->read_prefs( prefsfile );
  if ( info->get_status() == -1 ) {
    XPLMDebugString( "Could not read prefs file.\n" );
    XPLMDebugString( prefsfile.c_str() );
    XPLMDebugString( "\n" );
  } else {
    XPLMDebugString( "Read prefs file.\n" );
  }

  // Data refs.
  gZuluTime     = XPLMFindDataRef("sim/time/zulu_time_sec");
  gPlaneLat     = XPLMFindDataRef("sim/flightmodel/position/latitude");
  gPlaneLon     = XPLMFindDataRef("sim/flightmodel/position/longitude");
  gPlaneAlt     = XPLMFindDataRef("sim/flightmodel/position/elevation"); // y_agl ?
  gPlaneHeading = XPLMFindDataRef("sim/flightmodel/position/hpath");
  gPlaneIAS     = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");
  gPlaneGSP     = XPLMFindDataRef("sim/flightmodel/position/groundspeed");
  gPlaneTAS     = XPLMFindDataRef("sim/flightmodel/position/true_airspeed");
  gSimPaused    = XPLMFindDataRef("sim/time/paused");
  gSimSpeed     = XPLMFindDataRef("sim/time/sim_speed");


  // Register the callback 
  XPLMRegisterFlightLoopCallback(
			 MyFlightLoopCallback,
			 GPXLOG_INTERVAL,
			 NULL);


  /*
SDK210TestsMenuItem = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "SDK210Tests", NULL, 1);
SDK210TestsMenuId = XPLMCreateMenu("SDK210Tests", XPLMFindPluginsMenu(), SDK210TestsMenuItem, SDK210TestsMenuHandler, NULL);
SDK210TestsMenuItem2 = XPLMAppendMenuItem(SDK210TestsMenuId, "SDK210Tests", (void *)"SDK210Tests", 1)
  */
  // First we put a new menu item into the plugin menu
  mySubMenuItem = XPLMAppendMenuItem(
				     XPLMFindPluginsMenu(), // Plugins menu 
				     "GPX Log", // Menu title
				     0, // Item ref
				     1); // English 

  // Now create a submenu attached to our menu items
  myMenu = XPLMCreateMenu(
			  "GPX Log",
			  XPLMFindPluginsMenu(),
			  mySubMenuItem, /* Menu Item to attach to. */
			  MyMenuHandlerCallback,/* The handler */
			  0);			/* Handler Ref */

  // Append menu items Off and ON to our submenu
  XPLMAppendMenuItem( myMenu,
		      "Log - OFF",
		      (void*)GPXLOG_OFF,
		      1);
  XPLMAppendMenuItem( myMenu,
		      "Log - ON",
		      (void*)GPXLOG_ON,
		      1);

  // Init (read config file with params?)
  gOutputFile = NULL;
  if ( info->start_immediately == 0 ) {
    gGPXStatus  = GPXLOG_OFF; // start OFF
    XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 0);
    XPLMEnableMenuItem(myMenu, GPXLOG_ON, 1);
  } else {
    gGPXStatus  = GPXLOG_ON; // start ON
    XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 1);
    XPLMEnableMenuItem(myMenu, GPXLOG_ON, 0);
  }
  XPLMDebugString( "Initialised gpxlog.\n" );

  return 1;
}

PLUGIN_API void	XPluginStop(void) {
  gpxlog_stop( );
  XPLMUnregisterFlightLoopCallback(MyFlightLoopCallback, NULL);
}

PLUGIN_API int XPluginEnable(void) {
  return 1;
}

PLUGIN_API void XPluginDisable(void) {
  /* Flush the file when we are disabled.  This is convenient; you
   * can disable the plugin and then look at the output on disk. */
  if ( gOutputFile ) {
    fflush(gOutputFile);
  }
}

PLUGIN_API void XPluginReceiveMessage(
	      XPLMPluginID	inFromWho,
	      long		inMessage,
	      void	       *inParam ) {

  (void)inFromWho;
  (void)inParam;

  std::string s = "XPluginReceiveMessage "+to_str(inMessage)+"\n";
  XPLMDebugString( s.c_str() );

  if ( inMessage == XPLM_MSG_PLANE_LOADED ) {
    if( gGPXStatus == GPXLOG_ON ) {
      // start a new track when changing plane
      gpxlog_stop();
      gpxlog_start();
    }
  }
  if (inMessage == XPLM_MSG_PLANE_CRASHED) { //101
    if( gGPXStatus == GPXLOG_ON ) {
      gpxlog_stop();
      gpxlog_start();
    }
    }
  //chngelver, 108
}


/**********************************************************************/

void MyMenuHandlerCallback( void *inMenuRef, void *inItemRef) {
  (void)inMenuRef;

  if ( (long)inItemRef == GPXLOG_OFF ) {
    gpxlog_stop();
  }
  
  if ( (long)inItemRef == GPXLOG_ON ) {
    gpxlog_start();
  }
}

float MyFlightLoopCallback( float inElapsedSinceLastCall,
                            float inElapsedTimeSinceLastFlightLoop,
                            int inCounter,
                            void *inRefcon) {

  (void)inElapsedSinceLastCall;
  (void)inElapsedTimeSinceLastFlightLoop;
  (void)inCounter;
  (void)inRefcon;

  if( gGPXStatus == GPXLOG_ON ) {
    double lat = XPLMGetDataf(gPlaneLat);
    double lon = XPLMGetDataf(gPlaneLon);
    double alt = XPLMGetDataf(gPlaneAlt); // is already meters?
    double hdg = XPLMGetDataf(gPlaneHeading);
    double gsp = XPLMGetDataf(gPlaneGSP);
    int    psd = XPLMGetDatai(gSimPaused);
    int    spd = XPLMGetDatai(gSimSpeed);
    
    double dfp = 0.0; //distance from previous

    // save psd, new segment if starting after pause again?

    //  Write GPX trkpt, if not paused
    if ( (psd == 0) && gOutputFile ) {
      t += spd;

      // check if we want to log (distance, time, heading)
      // First check if turning
      //
      int do_log = 0;
      //int do_segment = 0; // new segment, track

      if ( (prev_lat < 999) && (prev_lon < 999) ) {

	// check hdg first, if change, log, than change 
	if ( fabs(hdg - prev_hdg) > info->delta_hdg ) {
	  // If we turn, we log
	  do_log = 1;
	}
	dfp = distanceto(prev_lat, prev_lon, lat, lon);
	if ( dfp > info->newtrack_dfp ) {
	  do_log = 1;
	  gpxlog_stop();
	  gpxlog_start();
	}
	if ( dfp > info->delta_dfp ) {
	  // if we don't turn, but move more than 2000 m, we log
	  do_log = 1;
	}
	if ( fabs(alt - prev_alt) > info->delta_alt ) {
	  // log fast ascend/descend as well, for 3D plots
	  do_log = 1;
	}

      } else { //999
	do_log = 1; // log if first one
      }

      if ( do_log > 0 ) {
	plugin_t = gmtime(&t);
	strftime(timeoutstr, 32, "%Y-%m-%dT%H:%M:%SZ", plugin_t);
	t_dist += dfp;

	// maybe a format option (WordTraffic, GPX, etc)
	info->write_outfile( "<trkpt lat=\""+to_str(lat,5)+"\" lon=\""+to_str(lon,5)+"\">" );
	info->write_outfile( "<time>"+std::string(timeoutstr)+"</time>" );
	info->write_outfile( "<ele>"+to_str(alt, 1)+"</ele>" );
	info->write_outfile( "<hdg>"+to_str(hdg, 1)+"</hdg>" );
	info->write_outfile( "<dfp>"+to_str(hdg, 1)+"</dfp>" );
	info->write_outfile( "<tsd>"+to_str(t_dist, 1)+"</tsd>" );
	info->write_outfile( "</trkpt>" );

	info->flush_outfile(); //maybe a config item?

	prev_lat = lat;
	prev_lon = lon;
	prev_alt = alt;
	prev_hdg = hdg;
	prev_gsp = gsp;
      }
    }
  }
  
  /*  Return time interval till next call */
  return GPXLOG_INTERVAL;
}

void gpxlog_stop() {
  // force print last point?
  if ( gGPXStatus == GPXLOG_OFF ) {
    return;
  }

  XPLMDebugString( "gpxlog_stop() called.\n" );

  XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 0);
  XPLMEnableMenuItem(myMenu, GPXLOG_ON, 1);

  if (gOutputFile) {
    // last point (clean up logic/order)
    double lat = XPLMGetDataf(gPlaneLat);
    double lon = XPLMGetDataf(gPlaneLon);
    double alt = XPLMGetDataf(gPlaneAlt);
    double hdg = XPLMGetDataf(gPlaneHeading);
    int    psd = XPLMGetDatai(gSimPaused);
    int    spd = XPLMGetDatai(gSimSpeed);
    double dfp = 0.0;
    if ( (prev_lat < 999) && (prev_lon < 999) ) {
      dfp = distanceto(prev_lat, prev_lon, lat, lon); //abs?
      // large distance here could mean crash and back at airport...
    }
    if ( psd == 0 ) {
      t += spd;
      plugin_t = gmtime(&t);
      t_dist += dfp;
      strftime(timeoutstr, 32, "%Y-%m-%dT%H:%M:%SZ", plugin_t);
      info->write_outfile( "<trkpt lat=\""+to_str(lat,5)+"\" lon=\""+to_str(lon,5)+"\">" );
      info->write_outfile( "<time>"+std::string(timeoutstr)+"</time>" );
      info->write_outfile( "<ele>"+to_str(alt, 1)+"</ele>" );
      info->write_outfile( "<hdg>"+to_str(hdg, 1)+"</hdg>" );
      info->write_outfile( "<dfp>"+to_str(hdg, 1)+"</dfp>" );
      info->write_outfile( "<tsd>"+to_str(t_dist, 1)+"</tsd>" );
      info->write_outfile( "</trkpt>" );
    }
    info->write_outfile( "</trkseg></trk></gpx>" );
  }
  info->close_outfile();
  gOutputFile = NULL;
  gGPXStatus = GPXLOG_OFF;
  prev_lat = 1000;
  prev_lon = 1000;
  prev_alt =   -1;
  prev_hdg =   -1;
  prev_psd =   -1;
  prev_spd =   -1;
  t_dist   =    0.0;
}

void gpxlog_start() {
  if ( gGPXStatus == GPXLOG_ON ) {
    return;
  }

  XPLMDebugString( "gpxlog_start() called.\n" );

  XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 1);
  XPLMEnableMenuItem(myMenu, GPXLOG_ON, 0);

  t        = time(NULL);
  plugin_t = gmtime(&t);
  strftime( timeoutstr, 32, "%Y%m%d%H%M%S.gpx", plugin_t );
  strcpy( gOutputPath, filebase );
  strcat( gOutputPath, timeoutstr );

#if APL && __MACH__
  Result = ConvertPath(gOutputPath, outputPath2, sizeof(gOutputPath));
  if (Result == 0)
    strcpy(gOutputPath, outputPath2);
  else
    XPLMDebugString("TimedProccessing - Unable to convert path\n");
#endif

  info->open_outfile( gOutputPath );
  gGPXStatus = GPXLOG_ON; // TODO error checking
}

#if APL && __MACH__
#include <Carbon/Carbon.h>
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen)
{
  CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, inPath ,kCFStringEncodingMacRoman);
  if (inStr == NULL)
    return -1;
  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle,0);
  CFStringRef outStr = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
  if (!CFStringGetCString(outStr, outPath, outPathMaxLen, kCFURLPOSIXPathStyle))
    return -1;
  CFRelease(outStr);
  CFRelease(url);
  CFRelease(inStr); 	
  return 0;
}
#endif

