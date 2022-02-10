#include <QCoreApplication>
#include <QTextCodec>
#include <QStringList>
#include <QDebug>
#include <QProcess>
#include <sys/mman.h>

#include "ProgramExecuteThread.h"



#define DBS_STATUS_STOPPED	0
#define DBS_STATUS_RUNNING	1
#define DBS_STATUS_PAUSE	2



//~~~~~ CONSTRUCTOR ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ProgramExecuteThread::ProgramExecuteThread(void)
{
    //QCoreApplication::setLibraryPaths(QStringList(QString("/home/user/ExecModule")));
    settings.load();
  //  QTextCodec::setCodecForTr(QTextCodec::codecForName("Windows-1251"));       // encoding for this source c++ file
  //  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("Windows-1251")); // encoding for byte array conversions
  //  QTextCodec::setCodecForLocale(QTextCodec::codecForName("Windows-1251"));  // encoding for output

    QTextCodec::setCodecForTr(QTextCodec::codecForName("Windows-1251"));       // encoding for this source c++ file
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("Windows-1251")); // encoding for byte array conversions
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));  // encoding for output
    QString compilationTime = QString("%1 %2").arg(__DATE__).arg(__TIME__);

    qDebug()<<"Application SW Version:"<<APP_VERSION<<compilationTime<<" For exit press Ctrl+\\";

    device = new CommThread();

    bar_scanner = new CodeScanObject(this,&settings);
    ui_thread = new UIThread(&settings);
    if(settings.enableCANtest) device->comm_object->CANtest();
	abort = false;
	pause = false;
	is_launched = false;
    scrType=0; // unknown script type
    device->setLightEn(settings.enableLight);
    device->setSigEn(settings.enableSig);
    device->setUVOffInterval(settings.uvOffInterval);
    device->setPauseCoverUPEn(settings.disablePauseOnCoverUP);
    barcode_ready = false;

	pressResumeButton=false;

    connect(device, SIGNAL(commError(ProgramFinishStatus::Detail)), this, SLOT(deviceError(ProgramFinishStatus::Detail)));


    connect(bar_scanner, SIGNAL(deviceFound(QString)), this, SLOT(barscannerFound(QString)));
    connect(bar_scanner, SIGNAL(barcodeReady(QString)), this, SLOT(barcodeReady(QString)));
	
    connect(ui_thread->user_interface, SIGNAL(pressToolMenuBtn(int)), device, SLOT(setLightReq(int)));
    //connect(ui_thread->user_interface, SIGNAL(pressOptionMenuBtn(int)), ioPs, SLOT(setOptionsChoice(int)));
    connect(ui_thread->user_interface, SIGNAL(pressMessageBtn()), this, SLOT(pressResumeBtn()));

    connect(ui_thread->user_interface, SIGNAL(terminateScript()), this, SLOT(slotTerminateScript()));
    connect(ui_thread->user_interface, SIGNAL(pauseScript()), this, SLOT(slotPauseScript()));
    connect(ui_thread->user_interface, SIGNAL(continueScript()), this, SLOT(slotContinueScript()));

    connect(device->comm_object,SIGNAL(controlCoverOpen(int)),ui_thread,SLOT(setExec(int)));

    ioPs = new TIoQtScr(this);

// initialize script signals/slots processing python script
    connect(ui_thread->user_interface, SIGNAL(pressOptionMenuBtn(int)), ioPs, SLOT(setOptionsChoice(int)));
    connect(ioPs,SIGNAL(disableOptions()),ui_thread->user_interface,SLOT(optionListDisable()));
    connect(ioPs,SIGNAL(signalSetOptionsExtraText(QString)),ui_thread->user_interface,SLOT(slotSetOptionExtraText(QString)));
    connect(ioPs,SIGNAL( setProcOnDisplay(QString)),ui_thread,SLOT(setProcedureName(QString)));
    connect(ioPs,SIGNAL( setProgressOnDisplay(int)),ui_thread,SLOT(setProgress(int)));
    connect(ioPs,SIGNAL( setExecutionOnDisplay(int)),ui_thread,SLOT(setExec(int)));
    connect(ioPs,SIGNAL( setOptionOnDisplay(QString)),ui_thread,SLOT(slotSetOptions(QString)));
    connect(ioPs,SIGNAL( sReInitDisplay()),ui_thread->user_interface,SLOT(slotReInitDisplay()));
    connect(ioPs,SIGNAL(setSignalisation(int)),device,SLOT(setSigReqFromScr(int)));

// script var configure
#ifdef WITH_QT_SCR
    ioQs = new TIoQtScr(this);
    // initialize script signals/slots processing java script
    connect(ioQs,SIGNAL( setProcOnDisplay(QString)),ui_thread,SLOT(setProcedureName(QString)));
    connect(ioQs,SIGNAL( setProgressOnDisplay(int)),ui_thread,SLOT(setProgress(int)));
    connect(ioQs,SIGNAL( setExecutionOnDisplay(int)),ui_thread,SLOT(setExec(int)));

    QScriptValue iface = engine.newQObject(&qs);
    QScriptValue global = engine.globalObject();
    global.setProperty("apl", iface); // apl.   call hw c function from qt script file
    QScriptValue myIface = engine.newQObject(ioQs);
    QScriptValue mo = engine.globalObject();
    mo.setProperty("sets", myIface); // sets.   call function for visual vidgets on display from qt script file
#endif

// python
    PythonQt::init(PythonQt::IgnoreSiteModule|PythonQt::RedirectStdOut);
    mainModule =  PythonQt::self()->getMainModule();
    PythonQt::self()->registerClass(&TIoQtScr::staticMetaObject, "dnaExec");
    PythonQt::self()->registerClass(&TPhotoQtScr::staticMetaObject, "dnaExec");
    mainModule.addObject("sets",ioPs );
#ifdef WITH_QT_SCR
    mainModule.addObject("apl",&qs);
#endif
    PythonQt::self()->setRedirectStdInCallbackEnabled(false);
    connect(PythonQt::self(), SIGNAL( pythonStdOut(QString)), this, SLOT(stdOut(QString)));
    connect(PythonQt::self(), SIGNAL( pythonStdErr(QString)), this, SLOT(stdOut(QString)));


    // Logs
    log.setLogicFileName(settings.logicLogFN);
    connect(device,SIGNAL(signal_logicMessage(QString)),&log,SLOT(logicMessage(QString)));
    if(settings.enableLog)
    {
        connect(this, SIGNAL(startLog()), &log, SLOT(startRecord()));
        connect(this, SIGNAL(stopLog()), &log, SLOT(stopRecord()));
        connect(this, SIGNAL(logMessage(QString)), &log, SLOT(softwareMessage(QString)));
        connect(this, SIGNAL(error(QString)), &log, SLOT(softwareMessage(QString)));
        connect(device, SIGNAL(statusMessage(QString)), &log, SLOT(hardwareMessage(QString)));
        connect(ui_thread, SIGNAL(message(QString)), &log, SLOT(softwareMessage(QString)));
    }
    connect(this, SIGNAL(error(QString)), this, SLOT(storeLastError(QString)));

}

//~~~~~ DESTRUCTOR ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ProgramExecuteThread::~ProgramExecuteThread()
{
 /*   if(bar_scanner) delete bar_scanner;

	mutex.lock();
	abort = true;
	condition.wakeOne();
	mutex.unlock();
	wait();

    delete ui_thread;
    delete bar_scanner;
    delete device;
    delete database;
*/
}
//~~~~~ Process QT script file ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//http://doc.qt.digia.com/qq/qq23-pythonqt.html#otherfeatures
int ProgramExecuteThread::processScr( QString fName )
{
  printf("Start script file\n");qDebug()<<"scr name"<<fName;
  QFileInfo file(fName);
  userErrorMsg="";
  if(file.suffix().toAscii()==settings.pythonExMask) scrType=1;
  else
    if(file.suffix().toAscii()==settings.jsExMask) scrType=2;
    else
      scrType=0; // unknown script type
    switch (scrType){
    case 1:{ // python
      ioPs->runScript(); ui_thread->setExec(0);
      device->setLightReq(LIGHT_UOFF);
      device->setScriptState(true);
      PythonQt::self()->addSysPath(QCoreApplication::applicationDirPath ());
      mainModule.evalFile (fName);
      device->setScriptState(false);
      userErrorMsg=ioPs->getErrorText(); printf("End script file\n"); return 1;
      //if(ioPs->getError()) { userErrorMsg=ioPs->getErrorText(); return 1; } // 20.08.2018
      break;
    }
#ifdef WITH_QT_SCR
    case 2:{ // JS
      ioQs->runScript();
      QFile scriptFile(fName);
      scriptFile.open(QIODevice::ReadOnly);
      QTextStream stream(&scriptFile);
      contentsQtScript = stream.readAll();
      scriptFile.close();
      device->setScriptState(true);
      QScriptValue result = engine.evaluate(contentsQtScript);
      device->setScriptState(false);
      if(result.isError()) {
        qDebug()<<QString("Script failure. Line %1, str=%2").arg(result.property("lineNumber").toInt32()).arg(result.toString());
        emit error(QString("Script failure. Line %1, str=%2").arg(result.property("lineNumber").toInt32()).arg(result.toString()));
        return 1;
      }
      if(ioQs->getError()) { userErrorMsg=ioQs->getErrorText(); return 1; }
      break;
    }
#endif
    default: // unknown script type
      break;
  }
  userErrorMsg="";
  printf("End script file\n");
  return 0;
}
//~~~~~ Terminate script file ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ProgramExecuteThread::slotTerminateScript(void)
{
   switch (scrType){
   case 1:{ // python
     ioPs->terminateScript();
     break;
   }
   case 2:{ // JS
     //engine.abortEvaluation();
     ioQs->terminateScript();
     break;
   }
   default: // unknown script type
     break;
   }

}
//~~~~~ TPause script file ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ProgramExecuteThread::slotPauseScript(void)
{
   switch (scrType){
   case 1:{ // python
     ioPs->pauseScript();
     break;
   }
   case 2:{ // JS
     //engine.abortEvaluation();
     ioQs->pauseScript();
     break;
   }
   default: // unknown script type
     break;
   }

}
//~~~~~ Continue script file ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ProgramExecuteThread::slotContinueScript(void)
{
   switch (scrType){
   case 1:{ // python
     ioPs->continueScript();
     break;
   }
   case 2:{ // JS
     //engine.abortEvaluation();
     ioQs->continueScript();
     break;
   }
   default: // unknown script type
     break;
   }

}

void ProgramExecuteThread::deviceError(ProgramFinishStatus::Detail detail)
{
    emit logMessage("ProgramExecuteThread::deviceError slot");
    ui_thread->setFinishStatusDetail(detail);
    device_error = true;
}

void ProgramExecuteThread::barscannerFound(QString name)
{
//    emit logMessage("barcode scanner: " + name);
}

void ProgramExecuteThread::barcodeReady(QString code)
{
  //  emit logMessage("barcode: " + code);
//    database->current_proc_status.barcode = code;
//    database->current_proc_status.barcode_time = QDateTime::currentDateTime();
    barcode_ready = true;
    barcodeValue=code;
}
//~~~~~ run ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ProgramExecuteThread::run()
{
    photoPs =new TPhotoQtScr();
    mainModule.addObject("camera",photoPs );
    if(!settings.cameraIPAddress.isEmpty()) photoPs->setCameraIP(settings.cameraIPAddress);

    emit logMessage("run()");
    bar_scanner->start();

    emit logMessage("commInit");
    device->commInit();

    if(!device_error){
      device->setSigReq(SIG_READY);
      while(!device->isReady()) msleep(100);
    }
    else{
      device->setSigReq(SIG_ERROR);
      while(!device->isReady()) msleep(100);
    }
    device_error = false;
    is_launched = false;

    emit logMessage("read hostname");
    {
        QFile file;
        QTextStream text_stream;
        file.setFileName("/etc/hostname");
        text_stream.setDevice(&file);
        file.open(QIODevice::ReadOnly);
        file.close();
    }
    emit logMessage("exec main cycle");

    while(!abort)
    {
        if(is_launched)
        {//while(1){
            ui_thread->user_interface->canRefresh=0;
            if(settings.enableSound)  /*system("mplayer -ao oss:/dev/dsp1 start.mp3");*/system("mplayer start.mp3");
            msleep(100);
            ui_thread->setStatus(DBS_STATUS_RUNNING);
            ui_thread->user_interface->canRefresh=1;
            emit startLog();
            emit logMessage("app_version=ExecModule_v" + QString(APP_VERSION));
            programExecute();
            is_launched=false;
            barcode_ready=false;
            ui_thread->user_interface->canRefresh=0;
            if(settings.enableSound)  /*system("mplayer -ao oss:/dev/dsp1 stop.mp3");*/system("mplayer stop.mp3");
            msleep(100);
            ui_thread->user_interface->canRefresh=1;
            if(userErrorMsg.length()) ui_thread->showUserMessageScreen(userErrorMsg);
            else
              ui_thread->showMessageScreen();

            emit stopLog();
        }
        ui_thread->setStatus(DBS_STATUS_STOPPED);
        while(!is_launched && !abort)
        {
            if(pressResumeButton) {
               pressResumeButton=false;
               //device->setSigReq(SIG_READY);
               //while(!device->isReady()) msleep(100);
            }
            msleep(100);
            mutex.lock();
            readControl(); // get script file name for run
            mutex.unlock();

        }
    }
}
//~~~~~ programExecute ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~private
void ProgramExecuteThread::programExecute(void)
{
    device_error = false;
    ui_thread->setFinishStatusGeneral(ProgramFinishStatus::GENERAL_NONE);
    ui_thread->setFinishStatusDetail(ProgramFinishStatus::DETAIL_NONE);

    //device->setSigReq(SIG_BUSY);
    //while(!device->isReady());// msleep(100);

    if(barcode_ready){ // BAR CODE read success

      QString scriptFile=settings.bcScriptProcFile;
      qDebug()<<barcodeValue;
      if(scriptFile.size()){
        ui_thread->setProgramName("BCR: "+QFileInfo(scriptFile).fileName()+" "+barcodeValue);
        ioPs->setRunName(barcodeValue);
        /*if(processScr(scriptFile)) { device->setSigReq(SIG_ERROR);while(!device->isReady()) msleep(100);}
        else{
          device->setSigReq(SIG_READY);while(!device->isReady()) msleep(100);
        }*/
        if(processScr(scriptFile)) msleep(100);
      }

    }
    else
    {
        QString scriptFile=ui_thread->user_interface->runProgramName;
        ioPs->setRunName(scriptFile);
        /*if(processScr(scriptFile)) { device->setSigReq(SIG_ERROR);while(!device->isReady()) msleep(100);}
        else{
          device->setSigReq(SIG_READY);while(!device->isReady()) msleep(100);
        }*/
        if(processScr(scriptFile)) msleep(100);
    }

    ui_thread->setFinishStatusGeneral(ProgramFinishStatus::SUCCESS);
    ui_thread->setFinishStatusDetail(ProgramFinishStatus::DETAIL_NONE);
	is_launched = false;
    emit(complete());
	emit procChanged(0);
	emit(logMessage("complete"));
	emit(programFinished(1));

}
//~~~~~ readControl ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~private
// set start script here in is_launcher
bool ProgramExecuteThread::readControl(void)
{
  if(ui_thread->user_interface->runSelectProgram()){
    is_launched = true;
    ui_thread->user_interface->clearRunSelectProgram();
  }
  if(barcode_ready){
    is_launched = true;
  }
  return true;
}






