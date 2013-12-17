/*
    X-Plane Plugin SDK
    Copyright (C) Sandy Barbour and Ben Supnik
    ---------------------------------------------------------------------------
    gpxlog.h
*/

#ifndef GPXLOG_H_INCLUDED
#define GPXLOG_H_INCLUDED

#include <inttypes.h>
#include "XPLMDataAccess.h"
#include "XPLMMenus.h"

/* Number of seconds to delay before writing next entry */
static const float GPXLOG_INTERVAL = 1.0;

/* Status of flight recorder. OFF or ON */
enum gpxlog_status {GPXLOG_OFF, GPXLOG_ON};

/* Data type for compatibility with GPXLOG.DLL. */
typedef uint32_t DWORD;

/* Data type for compatibility with GPXLOG.DLL. */
typedef uint16_t WORD;


/**
 * Required: This is a forced API (must be included) by the SDK.
 * It is called once when the plugin is loaded. (constructor)
 *
 * Allocate any permanent resources, do initialization, and register any other
 * callbacks. This is where the user interface is initialized.
 * Also return the plugin's name, signature, and a description to the XPLM.
 *
 * @param outName Name of the plugin
 * @param outSig Plugin signature
 * @param outDesc Plugin Description
 * @return 1 on successful init
 */
PLUGIN_API int XPluginStart(char *, char *, char *);

/**
 * Required: This is a forced API (must be included) by the SDK.
 * It is called once when the plugin is unloaded. (destructor)
 *
 * De-allocate any callbacks, free memory and close any open files.
 */
PLUGIN_API void	XPluginStop(void);

/**
 * Required: This is a forced API (must be included) by the SDK.
 * This is called when the plugin is enabled, either right after it is loaded
 * or when a user decides to enable it via the Plugin Manager.
 *
 * This is called when the plugin is enabled.
 * @return  1 if enable was successful
 */
PLUGIN_API int XPluginEnable(void);

/**
 * Required: This is a forced API (must be included) by the SDK.
 * This is called when the plugin is disabled, either just before it is
 * unloaded or when a user decides to disable it via the Plugin Manager.
 */
PLUGIN_API void XPluginDisable(void);

/**
 * Required: This is a forced API (must be included) by the SDK.
 * Used or not, we must make provision to receive messages from keyboard,
 * mouse, X-Plane and other plugins.
 *
 * Not used in this plugin
 *
 * @param inFromWho
 * @param inMessage
 * @param inParam
 */
PLUGIN_API void XPluginReceiveMessage( XPLMPluginID, long, void *);

/**
 * Determine which XFlightRec menu item was selected and Start or
 * Stop the recorder.
 *
 * @param inMenuRef Void pointer to the XFlightRec menu
 * @param inItemRef Void pointer to the XFlightRec menu item selected
 */
void MyMenuHandlerCallback(void *, void *);

/**
 * Determine which XFlightRec menu item was selected and Start or
 * Stop the recorder.
 *
 * @param inElapsedSinceLastCall
 * @param inElapsedTimeSinceLastFlightLoop
 * @param inCounter
 * @param inRefcon
 *
 * @return Return Time interval to be called again.
 */
float MyFlightLoopCallback(float, float, int,  void *);

/**
 * Stop the Flight Data Recorder. Close any open file
 */
void gpxlog_stop(void);

/**
 * Start the Flight Data Recorder. Open and overwrite a previous data file.
 */
void gpxlog_start(void);

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


#endif




