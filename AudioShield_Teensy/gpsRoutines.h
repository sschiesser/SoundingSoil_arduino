/*
 * gpsRoutines.h
 */

#ifndef _GPSROUTINES_H_
#define _GPSROUTINES_H_

#define GPS_NMEA_START_CHAR		0x24 // $
#define GPS_NMEA_STOP_CHAR		0x0A // LF
#define GPS_CONV_KNOT_TO_KMH	(1.852)
#define GPS_CONV_KNOT_TO_MPH	(1.15078)

struct gpsTime {
  byte h;
  byte min;
  byte sec;
  byte msec;
};
struct gpsDate {
  byte day;
  byte month;
  unsigned int year;
};
struct gpsLat {
  byte deg;
  byte min;
  unsigned int sec;
  bool north;
};
struct gpsLong {
  byte deg;
  byte min;
  unsigned int sec;
  bool east;
};
struct gpsAlt {
  float number;
  bool m_unit;
};
struct gpsGeoid {
  float number;
  bool m_unit;
};
struct gpsSpeed {
  float knots;
  float mph;
  float kmh;
};
struct gpsVar {
  float angle;
  bool east;
};
struct gpsRMCtag {
  struct gpsTime time;
  bool status_active;
  struct gpsLat latitude;
  struct gpsLong longitude;
  struct gpsSpeed speed;
  float track_angle;
  struct gpsDate date;
  struct gpsVar mvar;
  char sig_int;
  String raw_tag;
  byte length;
};


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */

struct gpsRMCtag fetchGPS(void);
struct gpsRMCtag  sliceTag(String tag);
int tagGetNextPos(String tag, byte pos, char delim);

#endif /* _GPSROUTINES_H_ */

