/*
 * can_comm.h
 * interface to CAN communication functions
 */
#ifndef CANCOMM_HH
#define CANCOMM_HH

#include "can_ids.h"
/*
#define CAN_init COMMSOCK_CAN_init
#define CAN_SendString COMMSOCK_CAN_SendString
#define CAN_RecvString COMMSOCK_CAN_RecvString
#define CAN_OpenChan COMMSOCK_CAN_OpenChan
#define CAN_CloseChan COMMSOCK_CAN_CloseChan
#define CAN_ResetChan COMMSOCK_CAN_ResetChan
#define CAN_CkChan COMMSOCK_CAN_CkChan
#define CAN_ProcessChan COMMSOCK_CAN_ProcessChan
#define CAN_ClientChan COMMSOCK_CAN_ClientChan
#define CAN_ServerChan COMMSOCK_CAN_ServerChan
#define CAN_SendEvent COMMSOCK_CAN_SendEvent
#define CAN_SendEventN COMMSOCK_CAN_SendEventN
*/
//#define CAN_ServerEvent COMMSOCK_CAN_ServerEvent
/*
#define CAN_GetCID COMMSOCK_CAN_GetCID
#define evflag COMMSOCK_evflag
#define evn COMMSOCK_evn
#define evdlen COMMSOCK_evdlen
#define evbuf COMMSOCK_evbuf
#define msgbuf COMMSOCK_msgbuf
#define msgcnt COMMSOCK_msgcnt
*/
//extern "C" int CAN_init(void);
#ifdef __cplusplus
extern "C" {
#endif
int CAN_init(void (*cmdfunc)(int,unsigned char*),
         void (*cmdfunc_reply)(int,unsigned char*));
int CAN_exit(void);
int CAN_SendString(char chan,unsigned char *msg, char mode);
int CAN_RecvString(char chan, unsigned char *msg, int blen);
signed char CAN_OpenChan(char nodeid);
signed char CAN_CloseChan(char chan);
signed char CAN_ResetChan(char chan);
signed char CAN_CkChan(char *chan);
signed char CAN_ProcessChan(char chan,char *buf,int blen);
int CAN_ClientChan(char chan, char *buf, char len, int tout);
int CAN_ServerChan(void (*cmdfunc)(char,char*),void (*cmdfunc_reply)(char,char*));
int CAN_SendEvent(char evid);
int CAN_SendEventN(char evid, unsigned char node);
extern int (*CAN_ServerEvent)(unsigned char evid, unsigned char src); //extern
int CAN_GetCID(char chan);
//http://docs.oracle.com/cd/E19059-01/wrkshp50/805-4956/bajdcjch/index.html
//http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0203j/Cacjfjei.html
#ifdef __cplusplus
}
#endif


//unsigned char evflag;
//unsigned short evn;
//int evdlen;
//unsigned char evbuf[8];
//unsigned char msgbuf[256]; // buffer for complete messages - TO REDO!!!
//int msgcnt; // complete message counter
#endif
