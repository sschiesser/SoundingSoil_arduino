/*
 * ssUtilities.h
 */

#ifndef _AUDIOUTILS_H_
#define _AUDIOUTILS_H_

#include "main.h"

extern AudioMixer4						mixer;

/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void prepareRecording(void);
void setRecInfos(struct recInfo* rec, String path, unsigned int rec_cnt);
void startRecording(String path);
void continueRecording(void);
void stopRecording(String path);
void startMonitoring(void);
void stopMonitoring(void);
void initAudio(void);

#endif /* _AUDIOUTILS_H_ */