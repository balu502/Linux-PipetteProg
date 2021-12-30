#ifndef PROGRAMEXECUTETHREAD_H
#define PROGRAMEXECUTETHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QtGui/QSound>
#include <QtScript>
#include <QList>
#include <QDateTime>
#include <QDebug>
#include "PythonQt.h"
#include "logobject.h"
#include "settings.h"
#include "codescanobject.h"
#include "CommThread.h"
#include "chreader.h"
#include "uithread.h"
#include "ioqtscr.h"
#include "photoqtscr.h"
#include "qsiface.h"


//#define WITH_QT_SCR

class CommThread;
class UIThread;


class ProgramExecuteThread : public QThread
{
	Q_OBJECT
public:
    ProgramExecuteThread(void);
	~ProgramExecuteThread();

public slots:
//    void deviceReady(void){qDebug()<<"On pos";device_ready = true;}
    void deviceError(ProgramFinishStatus::Detail detail);
    void stdOut(QString str) {printf("%s",str.toAscii().data());}//qDebug()<<"python<"<<str<<"python>";}
    void pressResumeBtn(void){ device_error = false; pressResumeButton=true;} // set state ready after press btn in message menu
//    void pressToolsMenu(int btn);
//    void pressExternalButtons(int light);
private slots:
    void storeLastError(QString error_text){  emit logMessage(error_text); }
    void barscannerFound(QString name);
    void barcodeReady(QString code);
    void slotTerminateScript(void);
    void slotPauseScript(void);
    void slotContinueScript(void);
signals:
    void startLog(void);
    void stopLog(void);
	void complete(void);
    void error(QString message);
	void logMessage(QString message);
	void procChanged(int order);
	void programFinished(bool successfully);
	void warningBox(QString message);

	void waitForUserActions(QString message);
    void userActionsDone(void);
protected:
	void run();
private:
    PythonQtObjectPtr   mainModule;
    QScriptEngine engine;
    QString contentsQtScript;
    QString userErrorMsg;
    TIoQtScr *ioQs,*ioPs ;
    TPhotoQtScr *photoPs;
    QString barcodeValue;
#ifdef WITH_QT_SCR
    qsIface qs;
#endif
    PythonQtObjectPtr tag;
    void programExecute(void);
//	void executeSetSigState(TSigState sigState);
//    void executeSetLightState(int light);
    bool readControl(void);
    int processScr( QString fName );


	bool abort;
	bool is_launched;
	bool pause;
    int scrType; // unknown - 0, python - 1, JS -2  type of script

	QMutex mutex;
	QWaitCondition condition;

    Settings settings;

    CommThread* device;
    CodeScanObject* bar_scanner;
    UIThread* ui_thread;

    LogObject log;

    int current_program_id;
//	bool device_ready;
    bool device_error;


 //   int lightControlReq;
	bool pressResumeButton;
    bool barcode_ready;
};

#endif // PROGRAMEXECUTETHREAD_H
