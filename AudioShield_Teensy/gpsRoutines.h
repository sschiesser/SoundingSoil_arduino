/*
 * gpsRoutines.h
 */

#ifndef _GPSROUTINES_H_
#define _GPSROUTINES_H_

#define GPS_NMEA_START_CHAR       0x24 // $
#define GPS_NMEA_STOP_CHAR        0x0A // LF
#define GPS_CONV_KNOT_TO_KMH      (1.852)
#define GPS_CONV_KNOT_TO_MPH      (1.15078)

struct gps_time {
  byte h;
  byte min;
  byte sec;
  byte msec;
};
struct gps_date {
  byte day;
  byte month;
  unsigned int year;
};
struct gps_lat {
  byte deg;
  byte min;
  unsigned int sec;
  bool north;
};
struct gps_long {
  byte deg;
  byte min;
  unsigned int sec;
  bool east;
};
struct gps_altitude {
  float number;
  bool m_unit;
};
struct gps_geoid {
  float number;
  bool m_unit;
};
struct gps_speed {
  float knots;
  float mph;
  float kmh;
};
struct gps_variation {
  float angle;
  bool east;
};
struct gps_rmc_tag {
  struct gps_time time;
  bool status_active;
  struct gps_lat latitude;
  struct gps_long longitude;
  struct gps_speed speed;
  float track_angle;
  struct gps_date date;
  struct gps_variation mvar;
  char sig_int;
  String raw_tag;
  byte length;
};

struct gps_rmc_tag fetchGPS(void);
struct gps_rmc_tag  sliceTag(String tag);
int tagGetNextPos(String tag, byte pos, char delim);

#endif /* _GPSROUTINES_H_ */

