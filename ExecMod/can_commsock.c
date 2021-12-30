/*
 *   can_comm.c   CAN communication functions
 *   Version for SOCKETCAN
 *   v2.00 17/03/12
 *   v2.10 24/02/16  CAN_exit with RX thread termination added    
 *
 */

#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
 
#include <linux/can.h>
#include <linux/can/raw.h>

#include <pthread.h>
#include <string.h>
#include <time.h>

#include <errno.h>

#include "can_comm.h"

/* At time of writing, these constants are not defined in the headers */
#ifndef PF_CAN
#define PF_CAN 29
#endif
 
#ifndef AF_CAN
#define AF_CAN PF_CAN
#endif

// "event" flag and ID
unsigned char evflag=0;
unsigned short evn;
unsigned char evbuf[8];
int evdlen=0;
int rxh_errno; // rx thread errno value

//#define CANDBG 1

#define MTYPE_STRING 0x400
#define MTYPE_EVENT  0x000

#define  CANBUFSZ 256

#ifndef UC_TYPE  // to define CAN node number in Makefile 
#define UC_TYPE 1
#endif

//#define UC_TYPE UCDISP_CANID


#define UCSELF_CANID UC_TYPE   // our uC CANID

#define MAXCANCHAN 10 // number of CAN channels

typedef struct can_chan {
	unsigned char state;	     	// channel state (OPEN,FREE,NEWDAT etc)
	unsigned char dstate;	     	// channel data stream state for data CC_TXRPL,CC_RXRPL
	unsigned char node_id; 			// destination node ID
	unsigned char tx_mbox; 			// hardware OUT channel
	unsigned char ctrl_req;			// last control request code
	unsigned char t_rptr,t_wptr; 	// read/write ptr for TX buffer
	unsigned char r_rptr,r_wptr; 	// read/write ptr for RX buffer
	unsigned char tbuf[CANBUFSZ],rbuf[CANBUFSZ]; // rx &tx buffers for this channel
} CANChannel;

#define CC_FREE		0x00  // CAN channel is available
#define CC_OPEN	   	0x01  // CAN channel is opened
#define CC_TXRPL	0x02  // CAN channel TX request/reply
#define CC_TXCTRL  	0x04  // control packet waits to be transmitted
#define CC_TXDAT  	0x08  // data packet waits to be transmitted
#define CC_WAIT	   	0x10  // CAN channel is waiting to be acknowleged (in opening or closing state)
#define CC_RXRPL	0x20  // CAN channel RX request/reply
#define CC_RXCTRL  	0x40  // control packet received for this channel
#define CC_RXDAT  	0x80  // data packet received for this channel

/*
 *  Types of packets for CAN channels
 */
#define CC_PID_DREQ	0x00  // data packet - request
#define CC_PID_DRPL	0x01  // data packet - reply
#define CC_PID_CREQ	0x02  // control packet - request
#define CC_PID_CRPL	0x03  // control packet - reply

#define CC_CTRL_OPEN	0x01  // request to open channel
#define CC_CTRL_CLOSE	0x02  // request to close channel
#define CC_CTRL_RESET	0x03  // request to reset channel

int (*CAN_ServerEvent)(unsigned char,unsigned char)=0;
void (*srv_reply)(int chan,unsigned char *buf)=0;
void (*srv_cmd)(int chan,unsigned char *buf)=0;

static CANChannel CANCh[MAXCANCHAN];
#define TXFIRSTCH 5

static int CANSend(char txmbox,short mid,unsigned char *dat, char len);
static int CANSendBuf(int chan);
static int doproc_chanctrl(int chan);

#define CANTxBusy(mbox) (0)
#define CANTxDI(mbox)
#define CANTxEI(mbox)
#define CANRxDI(mbox) //pthread_mutex_lock(&rx_mx)
#define CANRxEI(mbox) //pthread_mutex_unlock(&rx_mx)


int sk_rd,sk_wr; // can sockets (similar to CAN mailboxes) for in and out
void *scanrx_handler(void *);
void *scantx_handler(void *);
//pthread_mutex_t tx_mx     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mb_mx     = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  mb_rdy   = PTHREAD_COND_INITIALIZER;
pthread_t can_rxth,can_txth;
unsigned char msgbuf[256]; // buffer for complete messages - TO REDO!!!
int msgcnt=0; // complete message counter 
 

int CAN_init(void (*cmdfunc)(int,unsigned char*),
	     void (*cmdfunc_reply)(int,unsigned char*))
{
  long can_eflags;
  int i,ret;

  for (i=0;i<MAXCANCHAN;i++) {
    CANCh[i].state = 0;
    CANCh[i].node_id = 0;
  }

  /* Create the sockets */
  sk_rd = socket( PF_CAN, SOCK_RAW, CAN_RAW );
  struct ifreq ifr;
  strcpy(ifr.ifr_name, "can0");
  ioctl(sk_rd, SIOCGIFINDEX, &ifr); /* ifr.ifr_ifindex gets filled with that device's index */
   /* Select that CAN interface, and bind the socket to it. */
  struct sockaddr_can addr;
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  ret=bind( sk_rd, (struct sockaddr*)&addr, sizeof(addr) );
   if (ret<0) {
     perror("bind rd");
     return -1;
   }
#if 0
  sk_wr = socket( PF_CAN, SOCK_RAW, CAN_RAW );
  strcpy(ifr.ifr_name, "can0");
  ioctl(sk_wr, SIOCGIFINDEX, &ifr); /* ifr.ifr_ifindex gets filled 
                                        with that device's index */
   /* Select that CAN interface, and bind the socket to it. */
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  ret=bind( sk_wr, (struct sockaddr*)&addr, sizeof(addr) );
   if (ret<0) {
     perror("bind wr");
     return -1;
   }
#endif
   //  pthread_mutex_lock(&scantx_mx);
  pthread_mutex_lock(&mb_mx);
  srv_cmd = cmdfunc;
  srv_reply = cmdfunc_reply;
  if (pthread_create(&can_rxth,NULL,scanrx_handler,NULL)) {
      perror("thread create");
      ret=-2;
  }
  else ret = 0;

  pthread_mutex_unlock(&mb_mx);
  //pthread_mutex_unlock(&scantx_mx);

  return ret;
}

int CAN_exit(void)
{
  int *rx_error_ptr;
  if (close (sk_rd)) perror("socket close"); // close rx socket to break RX thr
  if (pthread_cancel(can_rxth)) perror("thread cancel");
  if (pthread_join(can_rxth,&rx_error_ptr)) perror("thread join");
  if (rx_error_ptr==PTHREAD_CANCELED) {
    //printf("RX thread cancelled\n");
    return 0;
  }
  else {
    if (*rx_error_ptr!=0) printf("RX thread eror %d\n",*rx_error_ptr);
    return *rx_error_ptr;
  }
}

void *scanrx_handler(void *arg)
{
  int len,i,canid;
  unsigned char intbuf[8];
  char pktid,nodeid,chan,req_code,mbox;
  fd_set rd_set;
  int ret;
  struct can_frame frame;

  FD_ZERO(&rd_set);
 
  while(1) {
     FD_SET(sk_rd,&rd_set);
		  
     ret=select(sk_rd+1,&rd_set,NULL,NULL,NULL);
     if (ret<=0) {
       perror("irq select");
       break;
     }
     int bytes_read = read( sk_rd, &frame, sizeof(frame) );
#ifdef CANDBG
     printf("can pkt %x, len %d, rd %d, select ret %d\n",
            frame.can_id,frame.can_dlc,bytes_read,ret);
#endif
   /* Read a message back from the CAN bus */
     
     //pthread_mutex_lock(&rx_mx); // lock receive mutex

     switch(frame.can_id&0x400) {
     case 0x400: // strings
      canid = frame.can_id;
      if (((canid>>4)&0xf)!=UCSELF_CANID) break; // skip not ours pkt
      len = frame.can_dlc;
      if (len>8) len = 8;

      nodeid = canid&0xF;
      pktid = (canid>>8)&0x3;

      //printf("Msg from %d (0x%x)\n",canid&0xf,canid);
      req_code = frame.data[0];

      // Find channel for this nodeid if any
      for (chan=0;chan<MAXCANCHAN;chan++)
        if (CANCh[chan].state && (CANCh[chan].node_id==nodeid)) break;
      if (chan==MAXCANCHAN) { // if no channels found
        // request to open a new channel or reistablish lost connection
        if (pktid==CC_PID_CREQ && (req_code==CC_CTRL_OPEN || req_code==CC_CTRL_RESET)) {
	  for (chan=0;chan<MAXCANCHAN;chan++) if (!CANCh[chan].state) break;
          if (chan==MAXCANCHAN) {// no channels available;		
		;
	  }
	  else {  // Ok, setup a new channel
	    CANCh[chan].state = CC_OPEN|CC_RXCTRL;
	    CANCh[chan].ctrl_req = req_code;
            CANCh[chan].node_id = nodeid;
            CANCh[chan].tx_mbox = TXFIRSTCH+chan;
	    //printf("New channel %d created\n",chan);
	    // Setup a new channel ptrs and send ack
	    doproc_chanctrl(chan);
	  }
        }
        else  { // no channel for this packet. Lost connection?
	  ;	
        }
      }
      else if (pktid==CC_PID_CRPL || pktid==CC_PID_CREQ) { // found channel, control packet
	CANCh[chan].ctrl_req = req_code;
        if ((pktid&CC_PID_CRPL)==CC_PID_CREQ) { // request
	  CANCh[chan].state |= ((pktid<<5)&0x60);
	  doproc_chanctrl(chan);
        }
        else { // got a reply
	  pthread_mutex_lock(&mb_mx);
	  CANCh[chan].state |= ((pktid<<5)&0x60);
	  //if (req_code==CC_CTRL_OPEN) CANCh[chan].state|=CC_OPEN;
	  //else if (req_code==CC_CTRL_CLOSE) CANCh[chan].state=CC_CLOSE;
	  //CANCh[chan].state&=~CC_RXRPL; // todo use as a replyflag, should be guarded by mutex, clear in the waiting thread
	  // somebody may be waiting on getting this reply, wake him up 
	  pthread_cond_signal(&mb_rdy);
	  pthread_mutex_unlock(&mb_mx);
        }	
      }
      else { // data for this channel
        if (!(CANCh[chan].state&CC_WAIT)|| // not busy channel
            (CANCh[chan].state&CC_WAIT && ((pktid&CC_PID_DRPL)==CC_PID_DRPL)) || // channel is waiting for reply
            (CANCh[chan].state&CC_WAIT && ((CANCh[chan].state&CC_RXDAT) || (CANCh[chan].dstate&CC_RXDAT)) )) { // channel is accepting a new command
          CANCh[chan].state |= CC_RXDAT;
          if ((pktid&CC_PID_DRPL)==CC_PID_DRPL)
            CANCh[chan].dstate |= CC_RXRPL;
          else { // request
            CANCh[chan].dstate &= ~CC_RXRPL;
            CANCh[chan].state |= CC_WAIT;  // mark channel busy
          }

          *(long *)intbuf = *(long *)frame.data;
          if (len>4) *((long *)intbuf+1) = *(long *)&frame.data[4];;
          i=0;
          while(i<len) {
            CANCh[chan].rbuf[CANCh[chan].r_wptr++] = intbuf[i++];
            CANCh[chan].r_wptr &= (CANBUFSZ-1);
          }
	  // set mutex to guard msgflag
	  if (ret=(doproc_chandata(chan,msgbuf,256))) { // get msg data, if any
	    if (ret==2) {if(srv_reply!=NULL) srv_reply(chan,msgbuf);}// reply
	    else  if (srv_cmd!=NULL) srv_cmd(chan,msgbuf);// cmd request
	      // wakeup somebody who waits on this event
	    if (ret==2) {
		pthread_mutex_lock(&mb_mx);
		msgcnt++;
		pthread_cond_signal(&mb_rdy);
		pthread_mutex_unlock(&mb_mx);
	    }
	  }
        }
        else { // ignore request for busy channel
          ; // send control pkt as a reply?
        }
      }	
      break;

    case 0: // events
      evn=frame.can_id&0x7FF;
      evflag=1;
      //printf("event %x, len %d\n",frame.can_id,frame.can_dlc);
      if (CAN_ServerEvent) {
	if (frame.can_dlc) {
	  evdlen=frame.can_dlc;
	  for (i=0;i<evdlen;i++) evbuf[i]=frame.data[i];
	}
	else evdlen=0;
    CAN_ServerEvent((evn>>4)&0x3f,evn&0xf);
      }
      break;
     } // switch end 

      //pthread_mutex_unlock(&rx_mx); // release receive mutex

   } // while(1) end
  //return;
  rxh_errno=errno;
  pthread_exit(&rxh_errno);
}

#if 0
void *scantx_handler(void *arg)
{

  while(1) {
    pthread_mutex_lock(&scantx_mx);
    pthread_cond_wait(&tx_rdy,&scantx_mx); // wait for start of transmit

    for (chan=0;chan<MAXCANCHAN;chan++)
	if (CANCh[chan].state&(CC_TXCTRL|CC_TXDAT) && (CANCh[chan].tx_mbox==mbox)) break;

      if ((chan==MAXCANCHAN) || (HECC1CANTRS&(1<<mbox))) return; // no data to transfer or TX busy, exit

      canid = (CANCh[chan].node_id<<4)|UCSELF_CANID;
      /* Control transfer */
      if (CANCh[chan].state&CC_TXCTRL) {
  	if (CANCh[chan].state&CC_TXRPL) canid |= (MTYPE_STRING|(CC_PID_CRPL<<8)); // if reply
	else canid |= (MTYPE_STRING|(CC_PID_CRPL<<8));
	CANSend(mbox,canid,&CANCh[chan].ctrl_req,1);
	CANCh[chan].state &= ~(CC_TXCTRL|CC_TXRPL); // clear channel TX state			
	//if (CANCh[chan].ctrl_req==CC_CTRL_CLOSE) {  // closing channel
	//	CANCh[chan].state=0;
	//}
      }
      else { // data packet
	while((len=CANCh[chan].t_wptr-CANCh[chan].t_rptr)!=0) {
	  if (len<0) len += CANBUFSZ;
	  if (len>8) len=8;
	  if ((CANBUFSZ-CANCh[chan].t_rptr)<len) { // if less then needed bytes to read before the end of the buffer
	    len = CANBUFSZ-CANCh[chan].t_rptr;
	  }
	  if (CANCh[chan].dstate&CC_TXRPL) canid |= (MTYPE_STRING|(CC_PID_DRPL<<8)); // if reply
	  else canid |= (MTYPE_STRING|(CC_PID_DREQ<<8));
	  CANSend(mbox,canid,&CANCh[chan].tbuf[CANCh[chan].t_rptr],len);
	  CANCh[chan].t_rptr +=len;
	  CANCh[chan].t_rptr &= (CANBUFSZ-1);
	}
	if (!(CANCh[chan].t_wptr-CANCh[chan].t_rptr)) { // al data sent
	  CANCh[chan].state &= ~CC_TXDAT;	
	  if (CANCh[chan].dstate&CC_TXRPL) {
	    CANCh[chan].state &= ~CC_WAIT; // release channel after acknowledge
	    CANCh[chan].dstate &= ~CC_TXRPL;
	  }
	}							
      }
      
      pthread_mutex_unlock(&scantx_mx);
  } // main tx loop 
}
#endif

/* Send event, eid -  event id, in the 9-4 bits of CAN standard MID */
int CAN_SendEvent(char evid)
{
  short mid=evid;
 
  mid<<=4;
  mid |= (UCSELF_CANID|MTYPE_EVENT);
  if (!CANTxBusy(1)) { // if mbox transmitter is not busy	
    CANSend(1,mid,0,0);
    return 0;
  }
  else return -1;
}
/* Send event, eid -  event id, in the 9-4 bits of CAN standard MID */
int CAN_SendEventEx(char evid, unsigned char *buf,int len)
{
  short mid=evid;
 
  mid<<=4;
  mid |= (UCSELF_CANID|MTYPE_EVENT);
  if (!CANTxBusy(1)) { // if mbox transmitter is not busy	
    CANSend(1,mid,buf,len);
    return 0;
  }
  else return -1;
}

/* Send event with other node address */
int CAN_SendEventN(char evid, unsigned char node)
{
  short mid=evid;
 
  mid<<=4;
  mid |= (node|MTYPE_EVENT);
  if (!CANTxBusy(1)) { // if mbox transmitter is not busy	
    CANSend(1,mid,0,0);
    return 0;
  }
  else return -1;
}

int CAN_RecvString(char chan, unsigned char *msg, int blen)
{
  int len,to=100000,mptr=0;

  if (!(CANCh[chan].state&CC_RXDAT)) return 0; // no data for this channel
  // check if data is available
  len=CANCh[chan].r_wptr-CANCh[chan].r_rptr;
  if (!len) {
    return 0; // somehow buffer is empty
  }

  while(mptr<blen && to) {
    len=CANCh[chan].r_wptr-CANCh[chan].r_rptr; // next packet from the buffer
//    if (!len) {
//      to--;
//      continue;
//    }
    if (len<0) len += CANBUFSZ;
    while(len--) {
      msg[mptr] = CANCh[chan].rbuf[CANCh[chan].r_rptr++];
      CANCh[chan].r_rptr &= (CANBUFSZ-1);
      if (!msg[mptr]) return (mptr+1); // end of string, ret len
      mptr++;
    }
  }
  if (mptr==blen) return -2; // buffer overflow
  if (!to) return -1;       // time out accepting string
}

int CAN_SendString(char chan,unsigned char *msg, char mode)
{
  short mid;
  unsigned char tout=0;
  int len=strlen(msg)+1;
  char txmbox,llen = ((8<len)?8:len);
  unsigned char wptr=CANCh[chan].t_wptr;

  if (chan>=MAXCANCHAN || !(CANCh[chan].state&(CC_OPEN))) return -1; // not used channel
  CANRxDI(0);
  if ( !mode && CANCh[chan].state&CC_WAIT) {
    CANRxEI(0);
    return -2; // chan is waiting to be acknowleged
  }
  CANCh[chan].state |= CC_WAIT; // mark chan busy
  CANRxEI(0);

  txmbox = CANCh[chan].tx_mbox;
	
  // fill in data buffer
  while(len--) {
	CANCh[chan].tbuf[CANCh[chan].t_wptr] = *(msg++);
	while (((CANCh[chan].t_wptr+1)%CANBUFSZ)==CANCh[chan].t_rptr) {
				//Delay(1);  // wait for buffer free space
		if (!(++tout)) {
			CANTxDI(txmbox);
			CANCh[chan].t_wptr=wptr; // restore previous ptr value
			CANTxEI(txmbox);
			return -2;
		}
	}
	CANTxDI(txmbox);
	CANCh[chan].t_wptr++;
	CANCh[chan].t_wptr &= (CANBUFSZ-1);
	CANTxEI(txmbox);
  }

  CANCh[chan].state |= CC_TXDAT;
  if (mode) {
  	CANCh[chan].dstate |= CC_TXRPL; // mode=1 means reply
	mid = CC_PID_DRPL<<8;
  }
  else {
  	CANCh[chan].dstate &= ~CC_TXRPL; // request
  	mid = CC_PID_DREQ<<8;	
  }

  // try to send first data pkt
  CANTxDI(txmbox); // disable TX interrupt of the mbox
  if (!CANTxBusy(txmbox)) { // if mbox transmitter is not busy	

    	if ((CANBUFSZ-CANCh[chan].t_rptr)<llen) { // if less then needed bytes to read before the end of the buffer
		llen = CANBUFSZ-CANCh[chan].t_rptr;
        }
	mid |= (CANCh[chan].node_id<<4)|UCSELF_CANID|MTYPE_STRING;
	CANSend(txmbox,mid,&CANCh[chan].tbuf[CANCh[chan].t_rptr],llen);
	CANCh[chan].t_rptr +=llen;
	CANCh[chan].t_rptr &= (CANBUFSZ-1);
	if (!(CANCh[chan].t_wptr-CANCh[chan].t_rptr)) {
	  CANCh[chan].state &= ~CC_TXDAT;	
          if (CANCh[chan].dstate&CC_TXRPL) {
            CANCh[chan].state &= ~CC_WAIT; // release channel after acknowledge
            CANCh[chan].dstate &= ~CC_TXRPL;
          }
	}	
  }
  CANTxEI(txmbox); // enable TX interrupt of the mbox
  
  CANSendBuf(chan);

  return 0;
}

static int CANSend(char txmbox,short mid,unsigned char *dat, char len)
{
  int i;
  long ldata;
  struct can_frame frame;

  frame.can_id= mid;  // message ID
  frame.can_dlc= len;
  memcpy(frame.data,dat,len);
  //int bytes_sent = write( sk_wr, &frame, sizeof(frame) );
  int bytes_sent = write( sk_rd, &frame, sizeof(frame) );
  //printf("Sent %d bytes with canid %x\n",bytes_sent,frame.can_id);
  return 0;
}

static int CANSendBuf(int chan)
{
  int len,mbox=0;
  int canid = (CANCh[chan].node_id<<4)|UCSELF_CANID;

  while((len=CANCh[chan].t_wptr-CANCh[chan].t_rptr)!=0) {
    if (len<0) len += CANBUFSZ;
    if (len>8) len=8;
    if ((CANBUFSZ-CANCh[chan].t_rptr)<len) { // if less then needed bytes to read before the end of the buffer
      len = CANBUFSZ-CANCh[chan].t_rptr;
    }
    if (CANCh[chan].dstate&CC_TXRPL) canid |= (MTYPE_STRING|(CC_PID_DRPL<<8)); // if reply
    else canid |= (MTYPE_STRING|(CC_PID_DREQ<<8));
    CANSend(mbox,canid,&CANCh[chan].tbuf[CANCh[chan].t_rptr],len);
    CANCh[chan].t_rptr +=len;
    CANCh[chan].t_rptr &= (CANBUFSZ-1);
  }
  if (!(CANCh[chan].t_wptr-CANCh[chan].t_rptr)) { // all data is sent
    CANCh[chan].state &= ~CC_TXDAT;	
    if (CANCh[chan].dstate&CC_TXRPL) {
      CANCh[chan].state &= ~CC_WAIT; // release channel after acknowledge
      CANCh[chan].dstate &= ~CC_TXRPL;
    }
  }

  return 0;
}
 
/*
 *    Open communication channel - send a request, receive a reply, setup TX and RX buffers
 */
signed char CAN_OpenChan(char nodeid)
{
  char i,ckchan,reply,type;
  int txmbox;
  short mid;
  struct timeval tv;
  struct timespec ts;
  int ret;

	// check if resources for a new channel are available

  for (i=0;i<MAXCANCHAN;i++)
	if (CANCh[i].state && CANCh[i].node_id==nodeid) return i; // only one channel for each node

  for (i=0;i<MAXCANCHAN;i++)
	if (!CANCh[i].state) break; // if free channel is found
  if (i==MAXCANCHAN) return -1; // error - no more free channels

	// setup a request structure
  CANCh[i].node_id = nodeid;
  CANCh[i].state = CC_WAIT|CC_TXCTRL;
  CANCh[i].ctrl_req = CC_CTRL_OPEN;
  CANCh[i].r_wptr = 0;
  CANCh[i].r_rptr = 0;
  CANCh[i].t_wptr = 0;
  CANCh[i].t_rptr = 0;
	
  // send a request
  txmbox = i+TXFIRSTCH;
  CANCh[i].tx_mbox = txmbox;
  CANTxDI(txmbox); // disable TX interrupt of the mbox
  if (!CANTxBusy(txmbox)) { // if mbox transmitter is not busy
	mid = (nodeid<<4)|UCSELF_CANID|MTYPE_STRING|(CC_PID_CREQ<<8);
  	CANSend(txmbox,mid,&CANCh[i].ctrl_req,1);
  	CANCh[i].state &= ~(CC_TXCTRL|CC_TXRPL); // clear channel TX state
  }
  CANTxEI(txmbox); // enable TX interrupt of the mbox

  gettimeofday(&tv,NULL);
  ts.tv_sec = tv.tv_sec;
  ts.tv_nsec = tv.tv_usec*1000;
  ts.tv_sec += 1; // timeout of 1 second

  pthread_mutex_lock(&mb_mx);
  while (!(CANCh[i].state&CC_RXRPL)) {
    ret=pthread_cond_timedwait(&mb_rdy,&mb_mx,&ts);
    //printf("got cond (open), stat %x, ret %d \n",CANCh[i].state,ret);
    if (ret==ETIMEDOUT) {
	pthread_mutex_unlock(&mb_mx);
  	CAN_ResetChan(i);
	CAN_CloseChan(i);
	return -2;
    }
  }
  CANCh[i].state=CC_OPEN; // clear "got reply" flag, mark chan opened
  pthread_mutex_unlock(&mb_mx);
  return i;
}

/*
 *    Close communication channel - wait for end of comm, send a request to close
 *
 */
signed char CAN_CloseChan(char chan)
{
  int mbox;
  char i,ckchan,reply,type;
  short mid;
  struct timeval tv;
  struct timespec ts;
  int ret;


  if (chan>=MAXCANCHAN || !(CANCh[chan].state&(CC_OPEN|CC_WAIT))) return -1; // not used channel
  if (CANCh[chan].state&CC_WAIT) return -2; // chan is waiting to be acknowleged

  mbox = CANCh[chan].tx_mbox;

  CANCh[chan].state |= (CC_TXCTRL|CC_WAIT);
  CANCh[chan].state &= ~CC_OPEN;
  CANCh[chan].ctrl_req = CC_CTRL_CLOSE; // request type

  CANTxDI(mbox); // disable TX interrupt of the mbox
  if (!CANTxBusy(mbox)) { // if mbox transmitter is not busy
  	CANSend(mbox,(MTYPE_STRING|(CC_PID_CREQ<<8)|UCSELF_CANID|(CANCh[chan].node_id<<4)),
                                                     &CANCh[chan].ctrl_req,1);
  	CANCh[chan].state &= ~(CC_TXCTRL|CC_TXRPL); // clear channel TX state		
  }
  CANTxEI(mbox); // enable TX interrupt of the mbox

  gettimeofday(&tv,NULL);
  ts.tv_sec = tv.tv_sec;
  ts.tv_nsec = tv.tv_usec*1000;
  ts.tv_sec += 1; // 1 second timeout

  pthread_mutex_lock(&mb_mx);
  while (!(CANCh[i].state&CC_RXRPL)) {
    ret=pthread_cond_timedwait(&mb_rdy,&mb_mx,&ts);
    //printf("got cond (close), stat %x, ret %d \n",CANCh[i].state,ret);
    if (ret==ETIMEDOUT) {
	CANCh[chan].state = 0;
	CANCh[chan].node_id = 0;
 	pthread_mutex_unlock(&mb_mx);
	return -2;
    }
  }
  CANCh[i].state=0; // clear "got reply" flag, close channel
  pthread_mutex_unlock(&mb_mx);

  return 0;
}


/*
 *    Reset communication channel
 */
signed char CAN_ResetChan(char chan)
{
  char txmbox;

  if (chan>=MAXCANCHAN || !(CANCh[chan].state&(CC_OPEN|CC_WAIT))) return -1; // not used channel
  // setup a request structure
  txmbox = CANCh[chan].tx_mbox;
  CANCh[chan].ctrl_req = CC_CTRL_RESET;
  CANCh[chan].state  = CC_OPEN; // clear any active flags
  CANCh[chan].r_wptr = 0;
  CANCh[chan].r_rptr = 0;
  CANCh[chan].t_wptr = 0;
  CANCh[chan].t_rptr = 0;
	
  CANTxDI(txmbox); // disable TX interrupt of the mbox
  if (!CANTxBusy(txmbox)) { // if mbox transmitter is not busy
  	CANSend(txmbox,(CANCh[chan].node_id<<4)|UCSELF_CANID|MTYPE_STRING|(CC_PID_CREQ<<8),
                                                     &CANCh[chan].ctrl_req,1);
  	CANCh[chan].state &= ~(CC_TXCTRL); // clear channel TX state		
  }
  CANTxEI(txmbox); // enable TX interrupt of the mbox

  return chan;
}

signed char CAN_CkChan(char *chan)
{
  char i;

  for (i=0;i<MAXCANCHAN;i++) {
    if (!CANCh[i].state) continue; // skip free channel

#ifdef CANDBG
    if (CANCh[i].state&(CC_RXCTRL|CC_RXDAT)){
      char str[50];
      sprintf(str,"CkChn %d, st %x dst %x, nid %d \n",(int)i,(int)CANCh[i].state,(int)CANCh[i].dstate,(int)CANCh[i].node_id);
      printf(str);
    }
#endif

    if (CANCh[i].state&CC_RXCTRL) // new control packet received - high priority
    break;
  }

  if (i!=MAXCANCHAN) {
    *chan=i;
    return 1;
  }

  for (i=*chan;i<MAXCANCHAN;i++) {
    if (!CANCh[i].state) continue; // skip free channel
    if (CANCh[i].state&CC_RXDAT) { // new data packet received, check end-of-msg
		int ridx;
  		int len=CANCh[i].r_wptr-CANCh[i].r_rptr;
  		if (!len) continue; // somehow buffer is empty
    	if (len<0) len += CANBUFSZ;
    	for (ridx=0;ridx<len;ridx++) {
    		int rptr = CANCh[i].r_rptr+ridx;
    		rptr &= (CANBUFSZ-1);        // modulo CANBUFSZ
#ifdef CANDBG
		{
			char str[50];
			sprintf(str,"Ret,Ck r:%d w:%d\r",rptr,CANCh[i].r_wptr);
			printf(str);
		}
#endif
      		if (!CANCh[i].rbuf[rptr]) {  // end-of-msg (byte 0x00)
      			CANCh[i].dstate |= CC_RXDAT;
      			*chan = i;
      			return 2;
      		}
    	}
  	}		
	// skip msg if not completed, go to next channel
  }
  return 0;
}

// Process new data arrived to channels
signed char CAN_ProcessChan(char chan,char *buf,int blen)
{
  char state = CANCh[chan].state;

#ifdef CANDBG
    {
      char str[50];
      sprintf(str,"ProcCh %d, state %x, dstate %x, req %d\n",(int)chan,(int)CANCh[chan].state,(int)CANCh[chan].dstate,(int)CANCh[chan].ctrl_req);
      printf(str);
    }
#endif

  if (state&CC_RXCTRL) { // control pkt
    if (state&CC_RXRPL) { // reply for our request
      switch (CANCh[chan].ctrl_req) {
      case CC_CTRL_OPEN: // ack of the req to open a new channel	
        CANCh[chan].state = CC_OPEN;
        break;

      case CC_CTRL_CLOSE: // ack of the channel close operation
        CANCh[chan].state = 0; // release channel	
        break;
      }
      CANCh[chan].state &= ~CC_RXRPL;	
      return 1; // reply
    }
    else { //  new request for us
      char txmbox;
      switch (CANCh[chan].ctrl_req) {
      case CC_CTRL_OPEN: // request to open a new channel, send ack if possible 	
        CANCh[chan].state = CC_OPEN;
        CANCh[chan].r_rptr = 0;
        CANCh[chan].r_wptr = 0;
        CANCh[chan].t_rptr = 0;
        CANCh[chan].t_wptr = 0;
      case CC_CTRL_CLOSE:
        txmbox = CANCh[chan].tx_mbox;
        CANTxDI(txmbox); // disable TX interrupt of the mbox
        CANCh[chan].state |= (CC_TXCTRL|CC_TXRPL);
        if (!CANTxBusy(txmbox)) { // if mbox transmitter is not busy
  	  CANSend(txmbox,(CANCh[chan].node_id<<4)|UCSELF_CANID|MTYPE_STRING|(CC_PID_CRPL<<8),
                                                     &CANCh[chan].ctrl_req,1);
  	  CANCh[chan].state &= ~(CC_TXCTRL|CC_TXRPL); // clear channel TX state	
          if (CANCh[chan].ctrl_req==CC_CTRL_CLOSE) CANCh[chan].state=0; // release channel
        }
        CANTxEI(txmbox); // enable TX interrupt of the mbox

          break;

      case CC_CTRL_RESET:
        CANCh[chan].state = CC_OPEN;
        CANCh[chan].r_rptr = 0;
        CANCh[chan].r_wptr = 0;
        CANCh[chan].t_rptr = 0;
        CANCh[chan].t_wptr = 0;
        // no acknowlege
        break;

        default:
	  break;
      }
    }
    CANCh[chan].state &= ~CC_RXCTRL; // clear new  flag
    return 0; // request
  }
  else if (state&CC_RXDAT) { // data pkt
    if (CANCh[chan].dstate&CC_RXDAT) { // msg completed
      if (CAN_RecvString(chan,buf,blen)<0) return -2;
      CANCh[chan].dstate &= ~CC_RXDAT; // clear msg completed flag	
      if (!(CANCh[chan].r_wptr-CANCh[chan].r_rptr))
        CANCh[chan].state &= ~CC_RXDAT;
      if (CANCh[chan].dstate&CC_RXRPL) {
        CANCh[chan].state &= ~CC_WAIT; //clear wait flag
        CANCh[chan].dstate &= ~CC_RXRPL;
        return 1;
      }
      else return 0;
    }
  }

  return -1; // false processing
}


int CAN_ClientChan(char chan, char *buf, char len, int tout)
{
  int type,ret;
  struct timespec to;

  if (chan>=MAXCANCHAN || chan<0 || !(CANCh[chan].state&CC_OPEN)) return -1;

  if (CAN_SendString(chan,buf,0)<0) return -2 ;
  if (!tout) return 0; // no wait for reply

  time(&to.tv_sec);
  to.tv_nsec=0;
  to.tv_sec+=tout;

  ret=0;
  pthread_mutex_lock(&mb_mx);
  while (!msgcnt) { // wait for msg ready condition variable 
    if (tout<0) ret=pthread_cond_wait(&mb_rdy,&mb_mx);
    else ret=pthread_cond_timedwait(&mb_rdy,&mb_mx,&to);
    if (ret==ETIMEDOUT) {
      pthread_mutex_unlock(&mb_mx);
      return -4; // timeout
    }
  }
  if (!ret) {
    strncpy(buf,msgbuf,len);
    msgcnt--;
  }
  pthread_mutex_unlock(&mb_mx);

  if (ret) return -3; // other errors
  return strlen(buf)+1;
}

int CAN_ServerChan(void (*cmdfunc)(char,char*),void (*cmdfunc_reply)(char,char*))
{
  int type,reply;
  char chan,msg[256];

  chan = 0;
  if (type=CAN_CkChan(&chan)) {
#ifdef CANDBG
    sprintf(msg,"Service of CAN channel %d, type %d\r",chan,type);
    printf(msg);
#endif
    if ((reply=CAN_ProcessChan(chan,msg,sizeof(msg)))<0) return -1;
    if (cmdfunc!=0 && type==2 && !reply) cmdfunc(chan,msg);
    if (cmdfunc_reply!=0 && type==2 && reply) cmdfunc_reply(chan,msg);
  }

  return 0;
}

int CAN_GetCID(char chan)
{
  if (chan>=MAXCANCHAN || chan<0 || !(CANCh[chan].state&CC_OPEN)) return -1;
  return CANCh[chan].node_id;
}

static int doproc_chanctrl(int chan)
{
  int txmbox;
  
  if (CANCh[chan].ctrl_req!=CC_CTRL_CLOSE) CANCh[chan].state = CC_OPEN;
  CANCh[chan].r_rptr = 0;
  CANCh[chan].r_wptr = 0;
  CANCh[chan].t_rptr = 0;
  CANCh[chan].t_wptr = 0;
  if (CANCh[chan].ctrl_req!=CC_CTRL_RESET) {
    txmbox = CANCh[chan].tx_mbox;
    CANTxDI(txmbox); // disable TX interrupt of the mbox
    CANCh[chan].state |= (CC_TXCTRL|CC_TXRPL);
    if (!CANTxBusy(txmbox)) { // if mbox transmitter is not busy
      CANSend(txmbox,(CANCh[chan].node_id<<4)|UCSELF_CANID|MTYPE_STRING|(CC_PID_CRPL<<8),
            &CANCh[chan].ctrl_req,1);
      CANCh[chan].state &= ~(CC_TXCTRL|CC_TXRPL); // clear channel TX state	
      if (CANCh[chan].ctrl_req==CC_CTRL_CLOSE) CANCh[chan].state=0; // release channel
    }
    CANTxEI(txmbox); // enable TX interrupt of the mbox
  }
  CANCh[chan].state&=~CC_RXCTRL;
  return 0;
}

int doproc_chandata(int chan, char *buf, int blen)
{
  int ridx;
  int len=CANCh[chan].r_wptr-CANCh[chan].r_rptr;
  if (!len) return 0; // somehow buffer is empty
  if (len<0) len += CANBUFSZ;
  for (ridx=0;ridx<len;ridx++) {
    int rptr = CANCh[chan].r_rptr+ridx;
    rptr &= (CANBUFSZ-1);        // modulo CANBUFSZ
    if (!CANCh[chan].rbuf[rptr]) {  // end-of-msg (byte 0x00)
      CANCh[chan].dstate |= CC_RXDAT;
      if (CAN_RecvString(chan,buf,blen)<0) return -2;
      CANCh[chan].dstate &= ~CC_RXDAT; // clear msg completed flag	
      if (!(CANCh[chan].r_wptr-CANCh[chan].r_rptr))
        CANCh[chan].state &= ~CC_RXDAT;
      if (CANCh[chan].dstate&CC_RXRPL) {
        CANCh[chan].state &= ~CC_WAIT; //clear wait flag
        CANCh[chan].dstate &= ~CC_RXRPL;
        return 2;
      }
      return 1;
    }
  }
  return 0;
}
