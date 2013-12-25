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

// distance between points
static double distanceto(double lat0, double lon0, double lat1, double lon1) {
  double slat = sin((lat1-lat0) * (double)(M_PI/360));
  double slon = sin((lon1-lon0) * (double)(M_PI/360));
  double aa   = slat*slat + cos(lat0 * (double)(M_PI/180)) * cos(lat1 * (double)(M_PI/180)) * slon * slon;
  return 6378145.0 * 2 * atan2(sqrtf(aa), sqrt(1-aa));
}
static double distanceto(geopos& p0, geopos& p1) {
  return distanceto(p0.lat, p0.lon, p1.lat, p1.lon);
}

#if APL && __MACH__
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen);
char outputPath2[255];
int  Result = 0;
#endif

/* X-Plane Dataref:  sim/time/zulu_time_sec */
XPLMDataRef gZuluTime;

/* X-Plane Dataref:  sim/flightmodel/position/latitude */
XPLMDataRef gPlaneLat;

/* X-Plane Dataref: sim/flightmodel/position/longitude */
XPLMDataRef gPlaneLon;

/* X-Plane Dataref: sim/flightmodel/position/elevation */
XPLMDataRef gPlaneAlt;

/* X-Plane Dataref: sim/flightmodel/position/psi */
XPLMDataRef gPlaneHeading;

/* X-Plane Dataref: sim/flightmodel/position/indicated_airspeed */
XPLMDataRef gPlaneIAS;

/* X-Plane Dataref: sim/flightmodel/position/groundspeed */
XPLMDataRef gPlaneGSP;

/* X-Plane Dataref: sim/flightmodel/position/true_airspeed */
XPLMDataRef gPlaneTAS;

/* X-Plane Dataref: sim/time/paused */
XPLMDataRef gSimPaused;

/* X-Plane Dataref: sim/time/sim_speed */
XPLMDataRef gSimSpeed;

Info* info = new Info(); // config settings, TODO: read from file. Globals here, better name, ...

char gOutputPath[255]; // Global System path to gpx file 
enum gpxlog_status gGPXStatus; // Status GPXLOG_OFF, GPXLOG_ON 
time_t t;            // Time
struct tm *plugin_t; // time in xplane
char timeoutstr[32];

std::string version = "1.0";

char  filebase[255];
int   result = 0;

// "Previous" logged point, and "null" point
struct geopos np = {
  .t   =     0,
  .lat =  1000,
  .lon =  1000,
  .alt = -1,
  .hdg = -1,
  .gsp = -1,
  .psd = -1,
  .spd = -1
};
struct geopos pp = np;

double t_dist = 0.0; // Total track distance

XPLMMenuID	myMenu;
int		mySubMenuItem;

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {
  strcpy(outName, "GXPLog");
  strcpy(outSig,  "durian.gpxlog");
  strcpy(outDesc, "A plugin that outputs a GPX log.");

  XPLMDebugString( "Starting gpxlog.\n" );
  
  XPLMGetSystemPath(filebase); // Locate the X-System directory
  //XPLMDebugString( filebase );
  //XPLMDebugString( "\n" );

  std::string sep = std::string(XPLMGetDirectorySeparator());

  strcpy( gOutputPath, filebase );
  strcat( gOutputPath, "Resources" );
  strcat( gOutputPath, XPLMGetDirectorySeparator() );
  strcat( gOutputPath, "plugins" );
  strcat( gOutputPath, XPLMGetDirectorySeparator() );
  strcat( gOutputPath, "gpxlog" ); // our directory
  strcat( gOutputPath, XPLMGetDirectorySeparator() );
  strcat( gOutputPath, "gpxlog.ini" );

  std::string prefsfile = std::string(filebase) + "Resources" + sep + "plugins" + sep+ "gpxlog" + sep + "gpxlog.ini";
  
#if APL && __MACH__
  Result = ConvertPath(gOutputPath, outputPath2, sizeof(gOutputPath));
  if (Result == 0) {
    prefsfile = std::string( outputPath2 );
  } else {
    XPLMDebugString("Unable to convert path\n");
  }
#endif
  XPLMDebugString( prefsfile.c_str() );
  XPLMDebugString( "\n" );

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
  gGPXStatus  = GPXLOG_OFF; // start OFF
  XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 0);
  XPLMEnableMenuItem(myMenu, GPXLOG_ON, 1);

  if ( info->start_immediately == 1 ) {
    XPLMDebugString( "Will auto start gpxlog.\n" );
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
  info->flush_outfile();
}

PLUGIN_API void XPluginReceiveMessage(
	      XPLMPluginID	inFromWho,
	      long		inMessage,
	      void	       *inParam ) {

  (void)inFromWho;
  (void)inParam;

  //std::string s = "XPluginReceiveMessage "+to_str(inMessage)+"\n";
  //XPLMDebugString( s.c_str() );

  if ( inMessage == XPLM_MSG_PLANE_LOADED ) {
    if ( info->start_immediately == 1 ) {
      // this will always trigger, reset after first use?
      gpxlog_start();
    } 
    if ( gGPXStatus == GPXLOG_ON ) {
      // start a new track when changing plane
      gpxlog_stop();
      gpxlog_start();
    }
  }
  if (inMessage == XPLM_MSG_PLANE_CRASHED) {
    if( gGPXStatus == GPXLOG_ON ) {
      gpxlog_stop();
      gpxlog_start();
    }
  }
  //XPLM_MSG_AIRPORT_LOADED, XPLM_MSG_SCENERY_LOADED ?
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

void get_geopos(geopos& the_pos) {
  the_pos.lat = XPLMGetDataf(gPlaneLat);
  the_pos.lon = XPLMGetDataf(gPlaneLon);
  the_pos.alt = XPLMGetDataf(gPlaneAlt);
  the_pos.hdg = XPLMGetDataf(gPlaneHeading);
  the_pos.gsp = XPLMGetDataf(gPlaneGSP);
  the_pos.psd = XPLMGetDatai(gSimPaused);
  the_pos.spd = XPLMGetDatai(gSimSpeed);
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

    struct geopos cp; //current pos, pp, previous pos
    get_geopos(cp);

    double dfp = 0.0; //distance from previous

    // save psd, new segment if starting after pause again?

    //  Write GPX trkpt, if not paused
    if ( cp.psd == 0 ) {
      t += cp.spd;

      // check if we want to log (distance, time, heading)
      // First check if turning
      //
      int do_log = 0;
      //int do_segment = 0; // new segment, track

      if ( (pp.lat < 999) && (pp.lon < 999) ) {

	// check hdg first, if change, log, then change 
	if ( fabs(cp.hdg - pp.hdg) > info->delta_hdg ) {
	  // If we turn, we log
	  do_log = 1;
	}
	//dfp = distanceto(pp.lat, pp.lon, cp.lat, cp.lon);
	dfp = distanceto(pp, cp);
	if ( dfp > info->newtrack_dfp ) { // hmm...is this good?
	  do_log = 1;
	  gpxlog_stop();
	  gpxlog_start();
	}
	if ( dfp > info->delta_dfp ) {
	  // if we don't turn, but move more than 2000 m, we log
	  do_log = 1;
	}
	if ( fabs(cp.alt - pp.alt) > info->delta_alt ) {
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

	// maybe a format option (WorldTraffic, GPX, etc)
	info->write_geopos( cp, std::string(timeoutstr), dfp, t_dist ); 
	info->flush_outfile(); //maybe a config item?

	pp = cp;
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

  //XPLMDebugString( "gpxlog_stop() called.\n" );

  XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 0);
  XPLMEnableMenuItem(myMenu, GPXLOG_ON, 1);

  // last point (clean up logic/order)
  struct geopos cp; //current pos, pp, previous pos
  get_geopos(cp);

  double dfp = 0.0;
  if ( (pp.lat < 999) && (pp.lon < 999) ) {
    //dfp = distanceto(pp.lat, pp.lon, cp.lat, cp.lon); //abs?
    dfp = distanceto(pp, cp);
  }
  if ( cp.psd == 0 ) {
    t += cp.spd;
    plugin_t = gmtime(&t);
    t_dist += dfp;
    strftime(timeoutstr, 32, "%Y-%m-%dT%H:%M:%SZ", plugin_t);
    info->write_geopos( cp, std::string(timeoutstr), dfp, t_dist ); 
  }
  info->close_track();
  info->close_outfile();

  gGPXStatus = GPXLOG_OFF;

  pp     = np;
  t_dist = 0.0;
}

void gpxlog_start() {
  if ( gGPXStatus == GPXLOG_ON ) {
    return;
  }

  //XPLMDebugString( "gpxlog_start() called.\n" );

  XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 1);
  XPLMEnableMenuItem(myMenu, GPXLOG_ON, 0);

  t        = time(NULL);
  plugin_t = gmtime(&t);
  strftime( timeoutstr, 32, "%Y%m%d%H%M%S.gpx", plugin_t );
  strcpy( gOutputPath, filebase );
  strcat( gOutputPath, timeoutstr );
  //XPLMDebugString( gOutputPath  );
  //XPLMDebugString( "\n" );

#if APL && __MACH__
  Result = ConvertPath(gOutputPath, outputPath2, sizeof(gOutputPath));
  if ( Result == 0 ) {
    strcpy(gOutputPath, outputPath2);
  } else {
    XPLMDebugString("Unable to convert path\n");
  }
#endif
  info->open_outfile( gOutputPath );
  gGPXStatus = GPXLOG_ON; // TODO error checking
}

#if APL && __MACH__
#include <Carbon/Carbon.h>
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen) {
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

