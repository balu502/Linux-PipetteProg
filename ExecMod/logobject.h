#ifndef LOGOBJECT_H
#define LOGOBJECT_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

class LogObject : public QObject
{
    Q_OBJECT
public:
    explicit LogObject(QObject *parent = 0);
    ~LogObject();
    void setLogicFileName(QString str);
signals:
    
public slots:
    void startRecord(void);
    void stopRecord(void);
    void hardwareMessage(QString message){//if(is_started)
            hardware_log.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append);
            //hardware_out.readAll();
            hardware_out << message + "\n";
            hardware_log.close();}
    void softwareMessage(QString message){//if(is_started)
            software_log.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append);
            //software_out.readAll();
            software_out << message + "\n";
            software_log.close();}
    void logicMessage(QString message){
            if(!is_started_logic) return;
            logic_log.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Append);
            //software_out.readAll();
            logic_out << QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ") + message + "\n";
            logic_out.flush();
            logic_log.close();}
private:
    void archive(void);

    QFile hardware_log;
    QFile software_log;
    QFile logic_log;

    QTextStream hardware_out;
    QTextStream software_out;
    QTextStream logic_out;

    int max_log_size;
    int archive_lenght;

    bool is_started,is_started_logic;
};

#endif // LOGOBJECT_H
