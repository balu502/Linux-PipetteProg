#include "ioqtscr.h"
#include <stdio.h>
#include <QStringList>
#include <QDebug>

TIoQtScr::TIoQtScr(QObject *parent) :
    QObject(parent)
{
  ctrlRun=0; // 0 - normal run, 1 - terminate run, 2 - pause run
  Error=0;
}
TIoQtScr::~TIoQtScr()
{
  //if(photo) delete photo;
}

void TIoQtScr::setProcedureName(QString t)
{
    emit setProcOnDisplay(t);
}

void TIoQtScr::setProgressValue(int val)
{
    if(val>100)val=100;
    if(val<0) val=0;
    emit setProgressOnDisplay(val);
}
void TIoQtScr::setPauseValue(int val)
{
    emit setExecutionOnDisplay(val);
}

void TIoQtScr::terminateScript(void)
{
   ctrlRun=1;
}
void TIoQtScr::pauseScript(void)
{
   ctrlRun=2;
}
void TIoQtScr::continueScript(void)
{
   ctrlRun=0;
}
void TIoQtScr::runScript(void)
{
   ctrlRun=0;
   Error=0;
   errStr="";
   setProgressValue(0);
}
int TIoQtScr::getCtrlStatus(void)
{
   return ctrlRun;
}
void TIoQtScr::setError(int err)
{
  Error=err;
  emit setSignalisation(err);
}
int TIoQtScr::getError(void)
{
  return Error;
}
void TIoQtScr::setErrorText(QString s)
{
 // if(s.length()) setError(1);
  errStr=s;
}
QString TIoQtScr::getErrorText(void)
{
  return errStr;
}
void TIoQtScr::setRunName(QString rn)
{
  runName=rn;
}

char* TIoQtScr::getRunName(void)
{
  return runName.toAscii().data();
}
// set list of options from python script
void TIoQtScr::setOptionsList(QString list)
{
   optionsStatus=-1;
   emit setOptionOnDisplay(list);
}
// return choise value of options list into python script
int TIoQtScr::getOptionsChoice(void)
{
   return optionsStatus;
}
// set choise value of options list from user interface
void TIoQtScr::setOptionsChoice(int val)
{
   optionsStatus=val;
}
// set text (count down on button panel)
void TIoQtScr::setOptionsExtraText(QString s)
{
  emit signalSetOptionsExtraText(s);
}
//disable option list on screen
void TIoQtScr::disableOptionsList(void)
{
  emit disableOptions();
}

//Extra initialisation of display system
void TIoQtScr::setReInitDisplay(void)
{
   emit sReInitDisplay();
}

