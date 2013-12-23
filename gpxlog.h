/*
    X-Plane Plugin SDK
    Copyright (C) Sandy Barbour and Ben Supnik
    ---------------------------------------------------------------------------
    gpxlog.h
*/

#ifndef GPXLOG_H
#define GPXLOG_H

#include <inttypes.h>
#include "XPLMDataAccess.h"
#include "XPLMMenus.h"

// Number of seconds to delay before checking position
static const float GPXLOG_INTERVAL = 1.0;

// Status of logging. OFF or ON
enum gpxlog_status {GPXLOG_OFF, GPXLOG_ON};

// Data type for compatibility with GPXLOG.DLL.
typedef uint32_t DWORD;

// Data type for compatibility with GPXLOG.DLL.
typedef uint16_t WORD;

PLUGIN_API int XPluginStart(char *, char *, char *);
PLUGIN_API void	XPluginStop(void);
PLUGIN_API int XPluginEnable(void);
PLUGIN_API void XPluginDisable(void);
PLUGIN_API void XPluginReceiveMessage( XPLMPluginID, long, void *);
void MyMenuHandlerCallback(void *, void *);
float MyFlightLoopCallback(float, float, int,  void *);

// -------------------

struct geopos {
  time_t t;
  double lat;
  double lon;
  double alt;
  double hdg;
  double gsp;
  int    psd;
  int    spd;
};
void get_geopos(geopos&);

void gpxlog_stop(void);
void gpxlog_start(void);

#endif
