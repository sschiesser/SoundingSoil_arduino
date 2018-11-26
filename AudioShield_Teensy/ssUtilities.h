/*
 * ssUtilities.h
 */

#ifndef _SSUTILITIES_H_
#define _SSUTILITIES_H_

#include "main.h"



/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void prepareRecording(void);
void setRecInfos(struct recInfo* rec, String path, unsigned int rec_cnt);
void alarmRecDone(void);


#endif /* _SSUTILITIES_H_ */