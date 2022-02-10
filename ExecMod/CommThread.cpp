#include <QtCore/QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QProcess>
#include <QStringList>

#include "CommThread.h"
#include "commobject.h"
#include "chreader.h"

//~~~~~ CONSTRUCTOR ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CommThread::CommThread(void)
{
	abort = false;
	is_launched = false;

    comm_object = new CommObject(&abort);
    comm_initialized = false;

	phase = READY;
    sigCtrl=SIG_UNKNOWN;
    lightCtrl=LIGHT_UNKNOWN;
//    oldSigState=SIG_OFF;
    connect(this,SIGNAL(onPosition()),this,SLOT(cmdCompleate()));
    connect(comm_object, SIGNAL(commError()), this, SLOT(trError()));
    connect(comm_object, SIGNAL(message(QString)), this, SIGNAL(statusMessage(QString)));
    connect(comm_object, SIGNAL(controlLight(int)),this, SLOT( setLightReq(int)));

    scriptRun=false;
    lightEn=true;
    sigEn=true;
    timerUVOff=new QTimer(this);
    connect(timerUVOff, SIGNAL(timeout()), this, SLOT(timerUVtout()));

    start(NormalPriority);
}
//~~~~~ DESTRUCTOR ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CommThread::~CommThread()
{
	mutex.lock();
	abort = true;
	condition.wakeOne();
	mutex.unlock();
	wait();
    delete comm_object;
}
// timeout timer for turn off UV light
void CommThread::timerUVtout(void)
{
  setLightReq(LIGHT_UOFF);
  timerUVOff->stop();
}

//~~~~~ commInit ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool CommThread::commInit(void)
{
	bool return_value;

	mutex.lock();
	if(phase != READY)
	{
		return_value = false;
        emit statusMessage("commInit: device not ready");
	}
	else
	{
        if(comm_object->open())
        {
			phase = COMM_INIT;
			is_launched = true;
			condition.wakeOne();
			return_value = true;
		}
		else
		{
            emit statusMessage("commInit: can't open comm_object");
            emit commError(ProgramFinishStatus::DEVICE_ERROR_COMM);
			return_value = false;
		}
	}
	mutex.unlock();
	return return_value;
}
void CommThread::setScriptState(bool val)
{
    scriptRun=val;
    comm_object->setScrRun(val);
}

bool CommThread::getScriptState(void)
{
  return scriptRun;
}


//~~~~~ setLightReq ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~slot
// 0 - led On ; 1 - led off ; 2 -UV on ; 3 - UV off
void CommThread::setLightReq(int light)
{
  if(!lightEn) return;
  if(getScriptState()&&(light==LIGHT_UON)) return;
  if(light==LIGHT_UON) {
    comm_object->setUVVal(true);   // set variable for button control
    timerUVOff->stop();
    timerUVOff->start(uvOffInterval);
  }
  if(light==LIGHT_UOFF) comm_object->setUVVal(false); // set variable for button control
  if(light==LIGHT_LON) comm_object->setLedVal(true); // set variable for button control
  if(light==LIGHT_LOFF) comm_object->setLedVal(false); // set variable for button control

  mutex.lock();
  if(isCommInitialized()){
    if(!setLightState((CLights)light)){
       emit error("executeSetLightState: device->setLightReq()");
    }
  }
  mutex.unlock();
}
//~~~~~ setSigReqFromScr ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~slot
// setup signalisation from script
// -1 - error(red), 0-pause(yellow), 1- work(green), 2-all off
void CommThread::setSigReqFromScr(int sigSt)
{
  if(!sigEn) return;
  CSigs sigState;
  if(sigSt<0) sigState=SIG_ERROR;
  else if(sigSt==0) sigState=SIG_READY;
  else if(sigSt==1) sigState=SIG_BUSY;
  else sigState=SIG_OFF;

  mutex.lock();
  device_ready=false;
  if(isCommInitialized()){
     if(!setSigState(sigState)){
       emit error("executeSetSigState: device->setSigsReq()");
     }
  }
  mutex.unlock();
}

//~~~~~ setSigsReq ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~slot
// Set signal 3 Leds on front panel 0 - off, 1- red(error) 2- yellow(ready), 3- green(busy)
void CommThread::setSigReq(CSigs sigState)
{
//  if(sigState==oldSigState) return;
  if(!sigEn) return;
  mutex.lock();
//  oldSigState=sigState;
  device_ready=false;
  if(isCommInitialized()){
    if(!setSigState(sigState)){
      emit error("executeSetSigState: device->setSigsReq()");
    }
  }

  mutex.unlock();
}
//~~~~~ setLightState ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool CommThread::setLightState(CLights light)
{
    bool return_value;
	
    if(phase != READY || !comm_initialized)
    {
        if(phase != READY) emit statusMessage("setLightState: device not ready");
        if(!comm_initialized) emit statusMessage("setLightState: comm not initialized");
        return_value = false;
    }
    else
    {
        if(comm_object->open())
        {
            phase = SET_LIGHT_STATE;
            lightCtrl=light;
            is_launched = true;
            condition.wakeOne();
            return_value = true;
        }
        else
        {
            emit statusMessage("setLightState: can't open comm_object");
            emit (commError(ProgramFinishStatus::DEVICE_ERROR_COMM));
            return_value = false;
        }
    }
    return return_value;
}
//~~~~~ setSigState ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool CommThread::setSigState(CSigs sigLed)
{
  bool return_value;

  if(phase != READY|| !comm_initialized){
    if(phase != READY) emit statusMessage("setSigState: device not ready");
    if(!comm_initialized) emit statusMessage("setSigState: comm not initialized");
    return_value = false;
  }
  else{
    if(comm_object->open()){
       sigCtrl = sigLed;
       phase = SET_SIG_STATE;
       is_launched = true;
       condition.wakeOne();
       return_value = true;
    }
    else{
      emit statusMessage("setSigState: can't open comm_object");
      emit (commError(ProgramFinishStatus::DEVICE_ERROR_COMM));
      return_value = false;
    }
  }

  return return_value;
}
//~~~~~ run ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void CommThread::run()
{
	while(!abort)
    {
        switch(phase)
        {
			case READY:
			{
				break;
			}
			case COMM_INIT:
			{
				commInitExecute();
				phase = READY;
				if(is_launched)
				{
					is_launched = false;
//					emit(onPosition());
				}
				break;
            }
            case SET_LIGHT_STATE:
            {
                setLightStateExecute();
                phase = READY;
                if(is_launched)
                {
                    is_launched = false;
                    //emit onPosition();
                }
                break;
            }
			case SET_SIG_STATE:
            {
                setSigStateExecute();
                phase = READY;
                if(is_launched)
                {
                    is_launched = false;
                    //emit(onPosition());
                }
                break;
            }
        }
		if(abort) return;

		mutex.lock();
		if(!is_launched) condition.wait(&mutex);
        mutex.unlock();

	}
    comm_object->close();
}
//~~~~~ commInitExecute ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~private
void CommThread::commInitExecute(void)
{
   comm_initialized = true;
   comm_object->open();
   comm_object->setChannel(COMM_CHAN_LEDS);
   comm_object->close();
}
//~~~~~ setLightStateExecute ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~private
void CommThread::setLightStateExecute(void)
{
    QString message;
    QString answer;
    //mutex.lock();
	bool ok = comm_object->setChannel(COMM_CHAN_LEDS);
    if(!ok)
    {
        emit commError(ProgramFinishStatus::DEVICE_ERROR_COMM);
        comm_initialized = false;
        return;
    }
	else
	{
        switch(lightCtrl){
          case LIGHT_LON:
		    message = "LLED 100";
		  break;
          case LIGHT_LOFF:
		    message = "LLED 0";
		  break;
          case LIGHT_UON:
		    message = "LUV 1";
            emit signal_logicMessage("UV 1");
		  break;
          case LIGHT_UOFF:
		    message = "LUV 0";
            emit signal_logicMessage("UV 0");
		  break;
		  default:
		  return;
		}	
		qDebug()<<message;
        answer = *comm_object->CommTR(&message);
		//answer must be LC>Ok  test it. 
        //qDebug()<<answer;
		if(!is_launched || abort) return;
	}
    //mutex.unlock();
}
//~~~~~ setSigStateExecute ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~private
void CommThread::setSigStateExecute(void)
{
  QString message;
  QString answer;
  int sigLed;

  switch(sigCtrl){
  case SIG_OFF:
    sigLed=0;
  break;
  case SIG_READY:
    sigLed=2;
  break;
  case SIG_BUSY:
    sigLed=3;
  break;
  case SIG_ERROR:
    sigLed=1;
  break;
  default:
    sigLed=0;
  }
  bool ok = comm_object->setChannel(COMM_CHAN_LEDS);
  if(!ok){
    emit commError(ProgramFinishStatus::DEVICE_ERROR_COMM);
    comm_initialized = false;
    return;
  }
  else{
    message ="LSIG " + QString::number((int)sigLed);
	qDebug()<<message;
    answer = *comm_object->CommTR(&message);
	qDebug()<<answer;
    //answer must be LC>Ok  test it. 
	if(!is_launched || abort) return;
  }
}

void CommThread::setLightEn(bool l)
{
  lightEn=l;
}
void CommThread::setSigEn(bool s)
{
  sigEn=s;
}
// set timeout of UV light turn off
void CommThread::setUVOffInterval(int val)
{
  // val in minits
  uvOffInterval=val*60000;
}
// 1 - disable pause on cover up
void CommThread::setPauseCoverUPEn(int val)
{
   comm_object->setPauseCoverUPEn(val);
}
