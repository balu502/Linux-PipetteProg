#include "commobject.h"

//#define _TTY_POSIX_
//#include <qextserialport.h>
//#include <ftdi.h>
#include <QThread>
#include <QDebug>
#include <QFile>
//#include "can_comm.h"

#define MSLEEP_VALUE    20
#define RC_TIMEOUT		500
class CommObject;
class Sleeper: public QThread
{
    public:
        static void msleep(int ms)
        {
            QThread::msleep(ms);
        }
};
namespace commobj
{
  CommObject *extComm;
}
void cmds(int chan, unsigned char *cmd)
{
/*    char str[80];
    int ret;
    printf("CMD received on can %s\n",cmd);
    if ((ret=CMDparse(cmd,str+3))>=0) {
      if (!ret) CAN_SendString(chan,"BB>Ok",1); // send ACK reply
      else {
        strncpy(str,"BB>",3);
        CAN_SendString(chan,str,1); // send data reply
      }
    }
    else {
      //sprintf(str,"BB>?%s",cmd);
      CAN_SendString(chan,str,1);
    }*/
}

int  CANEventHandler(unsigned char evid,unsigned char srcnode)
{
  using namespace commobj;
  //printf("Event %d from %d\n",evid,srcnode);
  extComm->eventCANHandler(evid,srcnode);
  return 0;
}

CommObject::CommObject(bool* abort_flag)
{
    using namespace commobj;
    extComm=this;
    abort = abort_flag;
    leds=false;
    uv=false;
    scrRun=false; // in idle state
    current_channel = -1;
    pauseCoverUPEn=0;
/*
    port = new QextSerialPort(devicePort);
    port->setBaudRate(BAUD9600);
    port->setFlowControl(FLOW_OFF);
    port->setParity(PAR_NONE);
    port->setDataBits(DATA_8);
    port->setStopBits(STOP_2);
    port->setTimeout(0, 1);*/
    CAN_init(cmds,NULL);
 /*   if (<0)
    {
        //exit(1);
    }*///http://rsdn.ru/article/cpp/fastdelegate.xml
   // pTest=&CommObject::CANEventHandler;
    //pComm=this;
    CAN_ServerEvent = (CANEventHandler);
    //CAN_ServerEvent=(CANEventHandler); //CANEventHandler; // setup events handler
}
void CommObject::canReInit(void)
{
  CAN_exit();
  CAN_init(cmds,NULL);
}

void CommObject::eventCANHandler(unsigned char evid,unsigned char srcnode)
{
    //printf("Event %d from %d\n",evid,srcnode);
    switch (evid) {
    case CEV_PICTEMP:
       break;
    case CEV_CAPT:  // get picture frame, continue to run HET  to generate CCD control signals
      break;
    case CEV_WHEELRDY:
      break;

    case CEV_TPROGCNT:
      break;

    case CEV_TPROGSTR:
      break;

    case CEV_SYNCREQ:
      //CAN_SendEvent(CEV_SYNCRPL);
      //CAN_SendEventN(CEV_SYNCRPL,2); //!!!
      //CAN_SendEventN(CEV_SYNCRPL,4); //!!!
      break;
    case CEV_LEDSWITCH:
        leds=!leds;
        if(leds)
          emit controlLight(LIGHT_LON);
        else
          emit controlLight(LIGHT_LOFF);
    break;
    case CEV_UVSWITCH:
        if(scrRun) break; // can't switch UV if run
        uv=!uv;
        if(uv)
          emit controlLight(LIGHT_UON);
        else
          emit controlLight(LIGHT_UOFF);
    break;
    case CEV_COVEROPEN:
      if(uv){
        uv=0;
        emit controlLight(LIGHT_UOFF);
      }
      if(pauseCoverUPEn) break;
      emit controlCoverOpen(1); //pause
    break;
    case CEV_SYNCRPL:
      break;

    }
}
void CommObject::setUVVal(bool val)
{
   uv=val;
}

void CommObject::setLedVal(bool val)
{
   leds=val;
}
CommObject::~CommObject(void)
{
//    delete port;
}

bool CommObject::open(void)
{
//    return port->open(QIODevice::ReadWrite);
    return true;
}

void CommObject::close(void)
{
//    if(port->isOpen()) port->close();
}

bool CommObject::setChannel(int chan)
{
    current_channel = chan;
    emit message("#" + QString::number(current_channel));
    return true;
}

int CommObject::getStartButton(void)
{
    return 0;
}

QString* CommObject::CommTR(QString* Message, bool emitMessage)
{
    static QString received;
    static char buffer[129];

    strcpy(buffer, Message->toAscii().data());

    int chan = CAN_OpenChan(current_channel);
    int ret = CAN_ClientChan(chan, buffer, 80, 5);
    if (ret>0) received = QString(buffer);
    else
    {
        received = "";
        emit(commError());
        CAN_ResetChan(chan);
        emit message("error in CAN transaction, node " + QString::number(current_channel)
                     + " ret " + QString::number(ret));
        emitMessage = false;
    }
    CAN_CloseChan(chan);

    if(emitMessage) emit message(Message->simplified() + " / " + received.simplified());
    return &received;
}

// test all CAN ch from 30 s and crate can_id.dat file with ID
void CommObject::CANtest(void)
{
  int chan,beginID=2,endID=15;
  //QString received;
  //char buffer[80];
  QFile data("can_id.dat");
  if(!data.open(QFile::WriteOnly)) {
    return;
  }
  QTextStream out(&data);
  qDebug()<<"Begin get CAN IDs";
  for(int i=beginID;i<=endID;i++){
    chan = CAN_OpenChan(i);
    //qDebug()<<"CAN IDs"<<i<<chan;
    if(chan<0) continue;
    //strcpy(buffer,"fver");
    //int ret = CAN_ClientChan(chan, buffer, 80, 5);
    //if (ret>0) received = QString(buffer);
    //else
    //  received="Unknown";
    out << "ID: " << i<<"\r\n";
    CAN_CloseChan(chan);
  }
  qDebug()<<"End get CAN IDs";
  data.close();

}
