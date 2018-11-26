/*
 * ssUtilities.h
 */

#ifndef _AUDIOUTILS_H_
#define _AUDIOUTILS_H_

#include "main.h"

// Audio mixer channels
#define MIXER_CH_REC          0
#define MIXER_CH_SDC          1

extern AudioMixer4						mixer;

/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void prepareRecording(void);
void setRecInfos(struct recInfo* rec, String path, unsigned int rec_cnt);
void startRecording(String path);
void continueRecording(void);
void stopRecording(String path);
void pauseRecording(void);
void finishRecording(void);
void startMonitoring(void);
void stopMonitoring(void);
void initAudio(void);

#endif /* _AUDIOUTILS_H_ */