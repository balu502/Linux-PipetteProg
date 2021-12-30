#ifndef UITHREAD_H
#define UITHREAD_H

#include <QThread>
#include "settings.h"
#include "userinterface.h"

class ProgramFinishStatus
{
public:
    enum General
    {
        GENERAL_NONE = 0,
        SUCCESS = 1,
        ABORT = 2
    }general;
    enum Detail
    {
        DETAIL_NONE = 0,
        ABORTED_BY_USER = 1,
        DATABASE_ERROR = 2,
        DEVICE_ERROR_COMM = 3,
        DEVICE_ERROR_X = 4,
        DEVICE_ERROR_Y = 5,
        DEVICE_ERROR_Z = 6,
        DEVICE_ERROR_PIP = 7,
        DRIVE_NOT_INITIALIZED = 8,
        PIPETTE_NOT_INITIALIZED = 9,
        DTMAG_COMM = 10
    }detail;

    QString statusHeader(void);
    QString statusText(void);
};

class UIThread : public QThread
{
    Q_OBJECT
public:
    explicit UIThread(Settings* settings);
    ~UIThread(void);

    void setStatus(int value);
    void setProgramName(QString name);
    void setFinishStatusGeneral(ProgramFinishStatus::General value);
    void setFinishStatusDetail(ProgramFinishStatus::Detail value);
    void showMessageScreen(void);
    void showUserMessageScreen(QString&);
	UserInterface* user_interface;
signals:
    void message(QString);
public slots:
    void setProcedureName(QString name);
    void setProgress(int value);
    void setExec(int value); // set execution status from script
    void slotSetOptions(QString);
protected:
    void run();
private:
    bool abort;
    QMutex mutex;
    
    ProgramFinishStatus finish_status;
};

#endif // UITHREAD_H
