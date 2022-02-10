#include "uithread.h"
#include <QDebug>


QString ProgramFinishStatus::statusHeader(void)
{
    if(general == SUCCESS) return "Program";
    else if(general == ABORT)
    {
        if(detail == ABORTED_BY_USER) return "Warning:";
        else return "Error:";
    }
    else return "?";
}

QString ProgramFinishStatus::statusText(void)
{
    if(general == SUCCESS) return "completed";
    else if(general == ABORT)
    {
        if(detail == ABORTED_BY_USER) return "aborted by user";
        else if(detail == DATABASE_ERROR) return "program data";
        else if(detail == DEVICE_ERROR_COMM) return "device comm";
        else if(detail == DEVICE_ERROR_X) return "drive X";
        else if(detail == DEVICE_ERROR_Y) return "drive Y";
        else if(detail == DEVICE_ERROR_Z) return "drive Z";
        else if(detail == DEVICE_ERROR_PIP) return "pipette module";
        else if(detail == DRIVE_NOT_INITIALIZED)  return "drive init";
        else if(detail == PIPETTE_NOT_INITIALIZED)  return "pipette init";
        else if(detail == DTMAG_COMM)  return "dtMag comm";
        else return "?";
    }
    else return "?";
}

UIThread::UIThread(Settings *settings) :
    QThread()
{
    abort = false;
    user_interface = new UserInterface(this);

    connect(user_interface, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
    user_interface->init(settings->scriptsDirName, settings->testsScriptsDirName,(QStringList()<<"*."+settings->pythonExMask<<"*."+settings->jsExMask));
    start(QThread::NormalPriority);
}

UIThread::~UIThread(void)
{
    mutex.lock();
    abort = true;
    //condition.wakeOne();
    mutex.unlock();
    wait();

    user_interface->deinit();
    delete user_interface;
}

void UIThread::run()
{
    long n=0;
    while(!abort)
    {
        msleep(10);
        mutex.lock();
        user_interface->scanButtons();
        user_interface->displayUpdate();
        mutex.unlock();
        if(n++ > 3000*60) n = 0;    // ~1 hour
    }
}

void UIThread::setStatus(int value)
{
    mutex.lock();
    user_interface->status = value;
    mutex.unlock();
}

void UIThread::setProgramName(QString name)
{
    mutex.lock();
    user_interface->current_program_name = name;
    mutex.unlock();
}

void UIThread::setProcedureName(QString name)
{
    mutex.lock();
    user_interface->current_procedure_name = name;
    mutex.unlock();
}

void UIThread::setProgress(int value)
{
    mutex.lock();
    user_interface->execution_progress = value;
    mutex.unlock();
}
// set status from script (0 - run, 1 - pause)
void UIThread::setExec(int value)
{
    mutex.lock();qDebug()<<"setExec (Run/pause)"<<value;
    user_interface->statusExecute = value;
    mutex.unlock();
}

void UIThread::setFinishStatusGeneral(ProgramFinishStatus::General value)
{
    mutex.lock();
    finish_status.general = value;
    mutex.unlock();
}

void UIThread::setFinishStatusDetail(ProgramFinishStatus::Detail value)
{
    mutex.lock();
    finish_status.detail = value;
    mutex.unlock();
}

void UIThread::showMessageScreen(void)
{
    mutex.lock();
    user_interface->showMessage(finish_status.statusHeader(), finish_status.statusText());
    mutex.unlock();
}
void UIThread::showUserMessageScreen(QString &str)
{
    mutex.lock();
    user_interface->showMessage(str,"");
    mutex.unlock();
}
// Slot call from ioqtscr.cpp by Signal setOptionOnDisplay
void UIThread::slotSetOptions(QString list)
{
    mutex.lock();
    user_interface->setOptionsList(list);
    mutex.unlock();
}
