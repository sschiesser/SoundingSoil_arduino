/*
* BC127.h
*/
#ifndef _BC127_H_
#define _BC127_H_

#include "main.h"

// Max amount of BT devices to list on inquiry
#define DEVLIST_MAXLEN				6
// Time offset to adjust from GPS UTC
#define GPS_TIME_OFFSET				1 // Central European Time
// BC127 serial port
#define BLUEPORT							Serial4
// BC127 reset pin
#define BC127_RST_PIN					30

extern struct btDev 					dev_list[DEVLIST_MAXLEN];
extern unsigned int 					found_dev;
extern String									BT_peer_address;
extern String									BT_peer_name;
extern int										BT_id_a2dp;
extern int										BT_id_avrcp;
extern int										BLE_conn_id;
extern bool                                     BC127_ready;

// Serial command messages
enum serialMsg {
    BCCMD__NOTHING = 0,
    BCCMD_ADV_ON,
    BCCMD_ADV_OFF,
    BCCMD_BLUE_OFF,
    BCCMD_BLUE_ON,
    BCCMD_BT_NAME,
    BCCMD_DEV_CONNECT,
    BCCMD_DEV_DISCONNECT1,
    BCCMD_DEV_DISCONNECT2,
    BCCMD_INQUIRY,
    BCCMD_MON_PAUSE,
    BCCMD_MON_START,
    BCCMD_MON_STOP,
    BCCMD_REC_START,
    BCCMD_REC_STOP,
    BCCMD_RESET,
    BCCMD_STATUS,
    BCCMD_VOL_A2DP,
    BCCMD_VOL_DOWN,
    BCCMD_VOL_UP,
    // ----------
    BCNOT_BT_STATE,
    BCNOT_FILEPATH,
    BCNOT_INQ_DONE,
    BCNOT_INQ_START,
    BCNOT_INQ_STATE,
    BCNOT_LATLONG,
    BCNOT_MON_STATE,
    BCNOT_REC_STATE,
    BCNOT_REC_NEXT,
    BCNOT_REC_NB,
    BCNOT_REC_TS,
    BCNOT_RWIN_OK,
    BCNOT_RWIN_VALS,
    BCNOT_VOL_LEVEL,
    // ----------
    BCERR_RWIN_BAD_REQ,
    BCERR_RWIN_WRONG_PARAMS,
    BCERR_VOL_BT_DIS,
    MAX_OUTPUTS
};



/* ======================
* FUNCTIONS DECLARATIONS
* ====================== */
void initBc127(void);
void bc127Reset(void);
void bc127BlueOn(void);
void bc127BlueOff(void);
void bc127AdvStart(void);
void bc127AdvStop(void);
void bc127Inquiry(void);
enum serialMsg parseSerialIn(String input);
bool sendCmdOut(int msg);
void populateDevlist(String addr, String name, String caps, String stren);
bool searchDevlist(String addr);

unsigned int countParams(String input);


enum serialMsg msgAvrcpPlay(void);
enum serialMsg msgAvrcpPause(void);
enum serialMsg msgAbsVol(String p1, String p2);
enum serialMsg msgLinkLoss(String p1, String p2);
enum serialMsg msgName1(String p1, String p2);
enum serialMsg msgCloseOk(String p1, String p2, String p3);
enum serialMsg msgName2(String p1, String p2, String p3);
enum serialMsg msgOpenOk(String p1, String p2, String p3);
enum serialMsg msgRecv1(String p1, String p2, String p3);
enum serialMsg msgInquiry1(String p1, String p2, String p3, String p4);
enum serialMsg msgName3(String p1, String p2, String p3, String p4);
enum serialMsg msgState(String p1, String p2, String p3, String p4);
enum serialMsg msgRecv2(String p1, String p2, String p3, String p4);
enum serialMsg msgInquiry2(String p1, String p2, String p3, String p4, String p5);
enum serialMsg msgLink1(String p1, String p2, String p3, String p4, String p5);
enum serialMsg msgName4(String p1, String p2, String p3, String p4, String p5);
enum serialMsg msgRecv3(String p1, String p2, String p3, String p4, String p5);
enum serialMsg msgInquiry3(String p1, String p2, String p3, String p4, String p5, String p6);
enum serialMsg msgRecv4(String p1, String p2, String p3, String p4, String p5, String p6);
enum serialMsg msgLink2(String p1, String p2, String p3, String p4, String p5, String p6);
enum serialMsg msgInquiry4(String p1, String p2, String p3, String p4, String p5, String p6, String p7);
enum serialMsg msgLink3(String p1, String p2, String p3, String p4, String p5, String p6, String p7);
enum serialMsg msgInquiry5(String p1, String p2, String p3, String p4, String p5, String p6, String p7, String p8);
enum serialMsg msgLink4(String p1, String p2, String p3, String p4, String p5, String p6, String p7, String p8);
enum serialMsg msgLink5(String p1, String p2, String p3, String p4, String p5, String p6, String p7, String p8, String p9);

String cmdDevConnect(void);
String cmdInquiry(void);
String cmdMonPause(void);
String cmdMonStart(void);
String cmdMonStop(void);

String notBtState(void);
String notFilepath(void);
String notInqDone(void);
String notInqStart(void);
String notLatlong(void);
String notMonState(void);
String notRecNb(void);
String notRecNext(void);
String notRecState(void);
String notRecTs(void);
String notRwinOk(void);
String notRwinVals(void);
String notVolLevel(void);

String errRwinBadReq(void);
String errRwinWrongParams(void);
String errVolBtDis(void);
#endif /* _BC127_H_ */
