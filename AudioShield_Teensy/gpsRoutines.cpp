/*
 * GPS routines
 * 
 * Miscellaneous functions to read GPS NMEA tags,
 * find a specific format (RMC), slice the read string
 * and store the information in a specifict struct
 * 
 */

#include <Arduino.h>
#include "gpsRoutines.h"

// struct gps_rmc_tag gps_tag;

/* fetchGPS
 * --------
 * Capture NMEA strings on serial1 and slice them in tags
 * IN:	- none
 * OUT:	- none
 */
struct gps_rmc_tag fetchGPS(void)
{
  String gpsResp = "";
  boolean stringFound = false;
  while(!stringFound) {
//    gpsResp = Serial1.readStringUntil(GPS_NMEA_STOP_CHAR);
    gpsResp = "$GPRMC,162614,A,5230.5900,N,01322.3900,E,10.0,90.0,131006,1.2,E,A*13";
    if(gpsResp.startsWith("$GPRMC")) {
//      Serial.println(gpsResp);
      stringFound = true;
    }
  }
  return sliceTag(gpsResp);
}

/* sliceTag(rawTag)
 * ----------------
 * Slice raw NMEA string into single tags
 * IN: 	- raw NMEA tag (string)
 * OUT:	- none
 */
struct gps_rmc_tag sliceTag(String rawTag)
{
  int pos = 0;
  int len;
  char printbuf[256];
  struct gps_rmc_tag gps_tag;
  gps_tag.raw_tag = rawTag;
  
  // Time
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    pos = pos + len;
    gps_tag.time.h = (byte)rawTag.substring(pos+1, pos+3).toInt();
    gps_tag.time.min = (byte)rawTag.substring(pos+3, pos+5).toInt();
    gps_tag.time.sec = (byte)rawTag.substring(pos+5, pos+7).toInt();
    sprintf(printbuf, "Time: %dh%dm%ds", gps_tag.time.h, gps_tag.time.min, 
            gps_tag.time.sec);
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }

  // Status
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    pos = pos + len;   
    gps_tag.status_active = (rawTag.substring(pos+1,pos+2) == "A") ? true : false;
    sprintf(printbuf, "Status: %s", gps_tag.status_active ? "Active" : "Warning!");
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }
   
  // Latitude
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    pos = pos + len;
    gps_tag.latitude.deg = (byte)rawTag.substring(pos+1, pos+3).toInt();
    gps_tag.latitude.min = (byte)rawTag.substring(pos+3, pos+5).toInt();
    int flen = tagGetNextPos(rawTag, pos, '.');
    if(flen) {
      int fpos = pos + flen;
      gps_tag.latitude.sec = (unsigned int)rawTag.substring(fpos+1, fpos+3).toInt(); 
    }
    else {
      gps_tag.latitude.sec = 0;
    }
    sprintf(printbuf, "Latitude: %d:%d'%d''", gps_tag.latitude.deg, 
            gps_tag.latitude.min, gps_tag.latitude.sec);
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    pos = pos+len;  
    gps_tag.latitude.north = (rawTag.substring(pos+1,pos+2) == "N") ? true : false;
    sprintf(printbuf, "Orientation: %c", gps_tag.latitude.north ? 'N' : 'S');
//    Serial.println(printbuf); 
  }
  else if(len == 0) {
    pos++;
  }

  // Longitude
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    pos = pos+len;   
    gps_tag.longitude.deg = (byte)rawTag.substring(pos+1, pos+4).toInt();
    gps_tag.longitude.min = (byte)rawTag.substring(pos+4, pos+6).toInt();
    int flen = tagGetNextPos(rawTag, pos, '.');
    if(flen) {
      int fpos = pos + flen;
      gps_tag.longitude.sec = (unsigned int)rawTag.substring(fpos+1, fpos+3).toInt();
    }
    else {
      gps_tag.longitude.sec = 0;
    }
    sprintf(printbuf, "Latitude: %d:%d'%d''", gps_tag.longitude.deg, 
            gps_tag.longitude.min, gps_tag.longitude.sec);
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    gps_tag.longitude.east = (rawTag.substring(pos+len+1,pos+len+2) == "E") ? true : false;
    pos = pos+len;
    sprintf(printbuf, "Orientation: %c", gps_tag.longitude.east ? 'E' : 'W');
//    Serial.println(printbuf); 
  }
  else if(len == 0) {
    pos++;
  }
   
  // Speed
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    gps_tag.speed.knots = (float)rawTag.substring(pos+len+1, pos+len+6).toFloat();
    gps_tag.speed.mph = gps_tag.speed.knots * GPS_CONV_KNOT_TO_MPH;
    gps_tag.speed.kmh = gps_tag.speed.knots * GPS_CONV_KNOT_TO_KMH;
    pos = pos+len;   
    sprintf(printbuf, "Speed: %.02f km/h", gps_tag.speed.kmh);
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }

  // Track angle
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    gps_tag.track_angle = (float)rawTag.substring(pos+len+1, pos+len+6).toFloat();
    pos = pos+len;   
    sprintf(printbuf, "Track angle: %.02fdeg", gps_tag.track_angle);
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }

  // Date
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    gps_tag.date.day = (byte)rawTag.substring(pos+len+1, pos+len+3).toInt();
    gps_tag.date.month = (byte)rawTag.substring(pos+len+3, pos+len+5).toInt();
    gps_tag.date.year = (unsigned int)rawTag.substring(pos+len+5, pos+len+7).toInt();
    pos = pos+len;   
    sprintf(printbuf, "Date: %02d.%02d.%02d", gps_tag.date.day, gps_tag.date.month, 
            gps_tag.date.year);
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }

  // Magnetic variation
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    gps_tag.mvar.angle = (float)rawTag.substring(pos+len+1,pos+len+6).toFloat();
    pos = pos+len;   
    sprintf(printbuf, "Magnetic variation: %.02fdeg", gps_tag.mvar.angle);
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    gps_tag.mvar.east = (rawTag.substring(pos+len+1,pos+len+2) == "E") ? true : false;
    pos = pos+len;   
    sprintf(printbuf, "Orientation: %c", gps_tag.mvar.east ? 'E' : 'W');
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }

  // Signal integrity
  len = tagGetNextPos(rawTag, pos, ',');
  if(len) {
    char sigint = rawTag.charAt(pos+len+1);
    sprintf(printbuf, "Signal integrity: %c", sigint);
//    Serial.println(printbuf);
  }
  else if(len == 0) {
    pos++;
  }
  
  return gps_tag;
}

/* tagGetNextPos(tag, pos, delim)
 * ------------------------------
 * Get the next position to a defined delimiter within a string
 * IN:	- feed string (string)
 *		- starting position (byte)
 *		- tag delimiter (char)
 * OUT:	- length of the found tag (int)
 */
int tagGetNextPos(String tag, byte pos, char delim)
{
  int nextPos = tag.indexOf(delim, pos+1);
  // Delimiter found --> return length between old & new delimiter
  if(nextPos >= 0) {
    return (nextPos - pos);
  }
  // Delimiter not found --> return -1
  return nextPos;
}
