/*
  001 Initial for 2.1.3
      Removed MACK APL ConvertPath ifdef/function
      Reinstated...
  002 check against prev pos
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

// XPLM_MSG_PLANE_LOADED  102
// XPLM_MSG_PLANE_CRASHED 101

#define ERAD 6378145.f /* radius of alien planet "earth" */

/* distance between points */
static double distanceto(double lat0, double lon0, double lat1, double lon1) {
  double slat = sinf((lat1-lat0) * (double)(M_PI/360));
  double slon = sinf((lon1-lon0) * (double)(M_PI/360));
  double aa   = slat*slat + cosf(lat0 * (double)(M_PI/180)) * cosf(lat1 * (double)(M_PI/180)) * slon*slon;
  return ERAD * 2 * atan2f(sqrtf(aa), sqrtf(1-aa));
}


#if APL && __MACH__
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen);
#endif

/* Global data file */
FILE *gOutputFile;

/* Global System path to data file */
char gOutputPath[255];

#if APL && __MACH__
char outputPath2[255];
int Result = 0;
#endif

/* Global Status of recording function GPXLOG_OFF, GPXLOG_ON */
enum gpxlog_status gGPXStatus;

/* Global time struct */
time_t t;
struct tm *plugin_t;
char timeoutstr[32];

/* XML header */
std::string version = "1.0";
char xml1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>";
char xml2[] = "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" creator=\"GPXLog for XPlane\" version=\"1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">";
char xml3[] = "<trk>";
char xml4[] = "<trkseg>";

char  filebase[255];
const char* sepchar = XPLMGetDirectorySeparator();
int   result = 0;

double prev_lat = 1000;
double prev_lon = 1000;
double prev_alt = -1;
double prev_hdg = -1;
int    prev_psd = -1;
int    prev_spd = -1;

double t_dist = 0.0;

XPLMMenuID	myMenu;
int		mySubMenuItem;

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {
  strcpy(outName, "GXPLog");
  strcpy(outSig,  "durian.gpxlog");
  strcpy(outDesc, "A plugin that outputs a GPX log.");

  XPLMDebugString( "Starting gpxlog.\n" );

  /* Locate the X-System directory. */
  XPLMGetSystemPath(filebase);

  /* Data refs. */
  gZuluTime     = XPLMFindDataRef("sim/time/zulu_time_sec");
  gPlaneLat     = XPLMFindDataRef("sim/flightmodel/position/latitude");
  gPlaneLon     = XPLMFindDataRef("sim/flightmodel/position/longitude");
  gPlaneAlt     = XPLMFindDataRef("sim/flightmodel/position/elevation"); // y_agl ?
  gPlaneHeading = XPLMFindDataRef("sim/flightmodel/position/hpath");
  gPlaneIAS     = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");//groundspeed ?
  gPlaneTAS     = XPLMFindDataRef("sim/flightmodel/position/true_airspeed");
  gSimPaused    = XPLMFindDataRef("sim/time/paused");
  gSimSpeed     = XPLMFindDataRef("sim/time/sim_speed");


  /* Register the callback for once a second.  */
  XPLMRegisterFlightLoopCallback(
			 MyFlightLoopCallback,	/* Callback */
			 GPXLOG_INTERVAL,	/* Interval */
			 NULL);			/* Not used */



  /* First we put a new menu item into the plugin menu.
   * This menu item will contain a submenu for us. */
  mySubMenuItem = XPLMAppendMenuItem(
		     XPLMFindPluginsMenu(),	/* Put in plugins menu */
		     "GPX Log",  	        /* Item title */
		     0,				/* Item ref */
		     1);			/* English charset */

  /* Now create a submenu attached to our menu item. */
  myMenu = XPLMCreateMenu(
		  "gpxlog",
		  XPLMFindPluginsMenu(),
		  mySubMenuItem, 		/* Menu Item to attach to. */
		  MyMenuHandlerCallback,	/* The handler */
		  0);				/* Handler Ref */

  /* Append menu items to our submenu. */
  XPLMAppendMenuItem( myMenu,
		      "GPX log - ON",
		      (void *)GPXLOG_ON,
		      1);
  XPLMAppendMenuItem( myMenu,
		      "GPX log - OFF",
		      (void *)GPXLOG_OFF,
		      1);

  /* Init */
  gOutputFile = NULL;
  gGPXStatus  = GPXLOG_OFF;

  //this seems backwards, check
  XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 1);
  XPLMEnableMenuItem(myMenu, GPXLOG_ON, 0);

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

  std::cerr << "XPluginReceiveMessage " << inMessage << std::endl;

  if ( inMessage == XPLM_MSG_PLANE_LOADED ) {
    if( gGPXStatus == GPXLOG_ON ) {
      gpxlog_stop();
      gpxlog_start();
    }
  }
  /*if (inMessage == XPLM_MSG_AIRPORT_LOADED) {
    if( gGPXStatus == GPXLOG_ON ) {
      gpxlog_stop();
      gpxlog_start();
    }
    }*/
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
    int    psd = XPLMGetDatai(gSimPaused);
    int    spd = XPLMGetDatai(gSimSpeed);
    
    double dfp = 0.0; //distance from previous
    /*  Write GPX trkpt */
    if ( (psd == 0) && gOutputFile ) {
      t += spd;
      // check if we want to log (distance, time, heading)
      // First check if turning
      //
      int do_log = 0;

      if ( (prev_lat < 999) && (prev_lon < 999) ) {
	// check hdg first, if change, log, than change 

	if ( fabs(hdg - prev_hdg) > 4.0 ) {
	  // If we turn, we log
	  do_log = 1;
	}
	dfp = distanceto(prev_lat, prev_lon, lat, lon); //abs?
	if ( dfp > 5000.0 ) {
	  do_log = 1;
	  gpxlog_stop();
	  gpxlog_start();
	}
	if ( dfp > 2000.0 ) {
	  // if we don't turn, but move more than 2000 m, we log
	  do_log = 1;
	}
	if ( fabs(alt - prev_alt) > 200.0 ) {
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
	fprintf( gOutputFile, "<trkpt lat=\"%f\" lon=\"%f\">\n", lat, lon );
	fprintf( gOutputFile, "<time>%s</time>\n", timeoutstr );
	fprintf( gOutputFile, "<ele>%.1f</ele>\n", alt );
	fprintf( gOutputFile, "<hdg>%.1f</hdg>\n", hdg ); /* non standard gpx field */
	fprintf( gOutputFile, "<dfp>%.1f</dfp>\n", dfp ); /* distance from previous */
	fprintf( gOutputFile, "<tsd>%.1f</tsd>\n", t_dist ); /* total segment distance */
	fprintf( gOutputFile, "</trkpt>\n");
	fflush(gOutputFile); //maybe a config item
	prev_lat = lat;
	prev_lon = lon;
	prev_alt = alt;
	prev_hdg = hdg;
	prev_spd = spd;
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

  XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 1);
  XPLMEnableMenuItem(myMenu, GPXLOG_ON, 0);

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
    }
    if ( psd == 0 ) {
      t += spd;
      plugin_t = gmtime(&t);
      t_dist += dfp;
      strftime(timeoutstr, 32, "%Y-%m-%dT%H:%M:%SZ", plugin_t);
      fprintf( gOutputFile, "<trkpt lat=\"%f\" lon=\"%f\">\n", lat, lon );
      fprintf( gOutputFile, "<time>%s</time>\n", timeoutstr );
      fprintf( gOutputFile, "<ele>%.1f</ele>\n", alt );
      fprintf( gOutputFile, "<hdg>%.1f</hdg>\n", hdg ); /* non standard gpx field */
      fprintf( gOutputFile, "<dfp>%.1f</dfp>\n", dfp ); /* distance from previous */
      fprintf( gOutputFile, "<tsd>%.1f</tsd>\n", t_dist ); /* total segment distance */
      fprintf( gOutputFile, "</trkpt>\n");
    }
    fprintf( gOutputFile, "</trkseg></trk></gpx>\n" );
  }
  if (gOutputFile) {
    fclose(gOutputFile);
  }
  gOutputFile = NULL;
  gGPXStatus = GPXLOG_OFF;
  prev_lat = 1000;
  prev_lon = 1000;
  prev_alt = -1;
  prev_hdg = -1;
  prev_psd = -1;
  prev_spd = -1;
  t_dist = 0.0;
}

void gpxlog_start() {
  if ( gGPXStatus == GPXLOG_ON ) {
    return;
  }

  XPLMEnableMenuItem(myMenu, GPXLOG_OFF, 0);
  XPLMEnableMenuItem(myMenu, GPXLOG_ON, 1);

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

  gOutputFile = fopen( gOutputPath, "w+" );
  
  if ( gOutputFile ) {
    fprintf( gOutputFile, "%s\n%s\n%s\n%s\n", xml1, xml2, xml3, xml4 );
    gGPXStatus = GPXLOG_ON;
  }
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

