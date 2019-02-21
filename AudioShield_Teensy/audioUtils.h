/*
 * ssUtilities.h
 */
#ifndef _AUDIOUTILS_H_
#define _AUDIOUTILS_H_

#include "main.h"

#define REC_READ_BUF_SIZE				256
#define REC_WRITE_BUF_SIZE			(2 * REC_READ_BUF_SIZE)

// Audio mixer channels
#define MIXER_CH_REC          0
#define MIXER_CH_SDC          1
// SGTL5000 headphone volume
#define SGTL5000_VOLUME_DEF		0.8
/* SGTL5000 line-in levels
 *	0: 3.12 Volts p-p
 *	1: 2.63 Volts p-p
 *	2: 2.22 Volts p-p
 *	3: 1.87 Volts p-p
 *	4: 1.58 Volts p-p
 *	5: 1.33 Volts p-p  (default)
 *	6: 1.11 Volts p-p
 *	7: 0.94 Volts p-p
 *	8: 0.79 Volts p-p
 *	9: 0.67 Volts p-p
 *	10: 0.56 Volts p-p
 *	11: 0.48 Volts p-p
 *	12: 0.40 Volts p-p
 *	13: 0.34 Volts p-p
 *	14: 0.29 Volts p-p
 *	15: 0.24 Volts p-p
 */
#define SGTL5000_INLEVEL_DEF	15
/* SGTL5000 line-out levels
 *	13: 3.16 Volts p-p
 *	14: 2.98 Volts p-p
 *	15: 2.83 Volts p-p
 *	16: 2.67 Volts p-p
 *	17: 2.53 Volts p-p
 *	18: 2.39 Volts p-p
 *	19: 2.26 Volts p-p
 *	20: 2.14 Volts p-p
 *	21: 2.02 Volts p-p
 *	22: 1.91 Volts p-p
 *	23: 1.80 Volts p-p
 *	24: 1.71 Volts p-p
 *	25: 1.62 Volts p-p
 *	26: 1.53 Volts p-p
 *	27: 1.44 Volts p-p
 *	28: 1.37 Volts p-p
 *	29: 1.29 Volts p-p  (default)
 *	30: 1.22 Volts p-p
 *	31: 1.16 Volts p-p
 */
#define GSTL5000_OUTLEVEL_DEF	29

#define ABS_VOL_MAX_VAL				120.0

extern String 								rec_path;
extern int										vol_ctrl;
extern float									vol_value;


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void prepareRecording(bool sync);
void setRecInfos(struct recInfo* rec, String path);
void startRecording(String path);
void continueRecording(void);
void stopRecording(String path);
void pauseRecording(void);
void resetRecInfo(struct recInfo* rec);
void finishRecording(void);
void startMonitoring(void);
void stopMonitoring(void);
void setHpGain(void);
void detectPeaks(void);
void initAudio(void);

#endif /* _AUDIOUTILS_H_ */