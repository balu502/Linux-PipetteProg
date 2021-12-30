#ifndef COMMTHREAD_H
#define COMMTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include "commobject.h"
#include "uithread.h"



class CommThread : public QThread
{
	Q_OBJECT
public:
    CommThread(void);
	~CommThread();

    bool commInit(void);
	bool isCommInitialized(void){return comm_initialized;}
	bool isReady(void){return (phase == READY);}
    void setScriptState(bool val);
    bool getScriptState(void);
    void setLightEn(bool);
    void setSigEn(bool);
    void setUVOffInterval(int val);
    void setPauseCoverUPEn(int val); // 1 - disable pause on cover up
    CommObject* comm_object;
public slots:
	void cInit(void){commInit();}
    void trError(void){is_launched = false; emit commError(ProgramFinishStatus::DEVICE_ERROR_COMM);}
    void cmdCompleate(void){ device_ready=true;}
    void setLightReq(int light);
    void setSigReq(CSigs sigState);
    void setSigReqFromScr(int sigSt);
    void timerUVtout(void);
signals:
    void commError(ProgramFinishStatus::Detail);
    void error(QString message);
    void onPosition(void);
    void statusMessage(QString message);
    void signal_logicMessage(QString message);
protected:
	void run();
private:
    void commInitExecute(void);
    bool setLightState(CLights light);
    void setLightStateExecute(void);
    bool setSigState(CSigs sigLed);
	void setSigStateExecute(void);
    QTimer *timerUVOff; // for automatical turn off UV light
    int uvOffInterval;

    CPhase phase;
	bool abort;
	bool is_launched;
    bool device_ready;
	QMutex mutex;
	QWaitCondition condition;


	bool comm_initialized;
    bool scriptRun;
// Signalisation control
    CSigs sigCtrl;

// light control
    CLights lightCtrl;
    bool lightEn,sigEn;
};

#endif // COMMTHREAD_H
