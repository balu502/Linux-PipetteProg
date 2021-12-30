#ifndef IOQTSCR_H
#define IOQTSCR_H

#include <QObject>
#include <QString>


class TIoQtScr : public QObject
{
    Q_OBJECT
public:
    explicit TIoQtScr(QObject *parent = 0);
    ~TIoQtScr();
    void setRunName(QString);
private:
 int ctrlRun,Error,optionsStatus;
 QString errStr;
 QString runName;


signals:
  void setProcOnDisplay(QString s);
  void setProgressOnDisplay(int n);
  void setExecutionOnDisplay(int val);//1 - pause
  void setSignalisation(int val) ;
  void setOptionOnDisplay(QString);
  void disableOptions(void);
  void sReInitDisplay();
  void signalSetOptionsExtraText(QString);

public slots:
    void setProcedureName(QString t);
    void setProgressValue(int val);
    void setPauseValue(int val);
    int getCtrlStatus(void);
    void terminateScript(void);
    void pauseScript(void);
    void continueScript(void);
    void runScript(void);
    void setError(int err);
    int getError(void);
    void setErrorText(QString);
    QString getErrorText(void);
    char* getRunName(void);
    void setOptionsList(QString list);
    int getOptionsChoice(void);
    void setOptionsChoice(int);
    void disableOptionsList(void);
    void setOptionsExtraText(QString);
    void setReInitDisplay(void);
};


#endif // IOQTSCR_H
