#ifndef COMMOBJECT_H
#define COMMOBJECT_H

#include <QObject>
//"C"
//extern  {
#include "can_comm.h"
//}

#define COMM_CHAN_DRIVEX    2
#define COMM_CHAN_DRIVEY    3
#define COMM_CHAN_DRIVEZ    4
#define COMM_CHAN_PIPETTE   5
#define COMM_CHAN_LEDS      6
int  CANEventHandler(unsigned char,unsigned char);

typedef enum
{
    READY,
    COMM_INIT,
    SET_LIGHT_STATE,
    SET_SIG_STATE
}CPhase;
typedef enum
{
    LIGHT_LON,
    LIGHT_LOFF,
    LIGHT_UON,
    LIGHT_UOFF,
    LIGHT_UNKNOWN
}CLights;
typedef enum
{
    SIG_OFF,
    SIG_ERROR,
    SIG_READY,
    SIG_BUSY,
    SIG_UNKNOWN
}CSigs;

class CommObject : public QObject
{
    Q_OBJECT
public:
    CommObject(bool *abort_flag);
    ~CommObject(void);

    bool open(void);
    void close(void);
    bool setChannel(int chan);
    void eventCANHandler(unsigned char evid,unsigned char srcnode);
    int getStartButton(void);
    void canReInit(void);
    QString* CommTR(QString* Message, bool emitMessage = true);
    void setUVVal(bool);
    void setLedVal(bool);
    void setPauseCoverUPEn(int val) {pauseCoverUPEn=val;}
    void setScrRun(bool val) {scrRun=val;}
    void CANtest(void);
//    typedef int (CommObject::*PTR_FUN)(unsigned char,unsigned char);
//    PTR_FUN pTest;

signals:
    void commError(void);
    void message(QString);
    void controlLight(int);
    void controlCoverOpen(int);
public slots:

private:
        bool* abort;
        bool leds,uv;
        int current_channel;
        int pauseCoverUPEn;
        bool scrRun;
};

#endif // COMMOBJECT_H
