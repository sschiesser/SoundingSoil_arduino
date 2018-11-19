/*
 * SD utils
 * 
 * Miscellaneous functions to handle with the SD card.
 * 
 */
 
#include "SDutils.h"

String createSDpath(struct gps_rmc_tag tag) {
	Serial.println("Creating new folder/file on the SD card");
	String dirName = "";
	String fileName = "";
	String path = "/";
	char buf[12];
	sprintf(buf, "%02d", tag.date.year);
	dirName.concat(buf);
	sprintf(buf, "%02d", tag.date.month);
	dirName.concat(buf);
	sprintf(buf, "%02d", tag.date.day);
	dirName.concat(buf);

	sprintf(buf, "%02d", tag.time.h);
	fileName.concat(buf);
	sprintf(buf, "%02d", tag.time.min);
	fileName.concat(buf);
	sprintf(buf, "%02d", tag.time.sec);
	fileName.concat(buf);

	path.concat(dirName);
	path.concat("/");
	path.concat(fileName);
	path.concat(".wav");

	if(SD.exists(dirName)) {
		if(SD.exists(path)) {
			SD.remove(path);
		}
	}
	else {
		SD.mkdir(dirName);
	}
	return path;
}
