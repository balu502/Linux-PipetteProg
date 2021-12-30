#include "logobject.h"


LogObject::LogObject(QObject *parent) :
    QObject(parent)
{
    is_started = false; is_started_logic=false;
    hardware_log.setFileName("hardware_log_0.txt");
    software_log.setFileName("software_log_0.txt");
    hardware_out.setDevice(&hardware_log);
    software_out.setDevice(&software_log);

    max_log_size = 100000;
    archive_lenght = 9;
}

LogObject::~LogObject()
{
    hardware_log.close();
    software_log.close();
    logic_log.close();
}
void LogObject::setLogicFileName(QString str)
{
  logic_log.setFileName(str);
  logic_out.setDevice(&logic_log);
  is_started_logic=true;
}

void LogObject::startRecord(void)
{
    if(is_started) return;

    is_started = true;
    hardware_log.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    software_log.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    //hardware_out.readAll();
    //software_out.readAll();
    hardware_out << "START " + QDateTime::currentDateTime().toString("yyyy/MM/dd - hh:mm:ss") + "\n";
    software_out << "START " + QDateTime::currentDateTime().toString("yyyy/MM/dd - hh:mm:ss") + "\n";
    //
    hardware_log.close();
    software_log.close();
    //
}

void LogObject::stopRecord(void)
{
    if(!is_started) return;

    is_started = false;
    //
    hardware_log.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    software_log.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    //hardware_out.readAll();
    //software_out.readAll();
    //
    hardware_out << "STOP " + QDateTime::currentDateTime().toString("yyyy/MM/dd - hh:mm:ss") + "\n\n";
    software_out << "STOP " + QDateTime::currentDateTime().toString("yyyy/MM/dd - hh:mm:ss") + "\n\n";
    hardware_log.close();
    software_log.close();
    if(hardware_log.size() > max_log_size
            || software_log.size() > max_log_size) archive();
}

void LogObject::archive(void)
{
    QFile file;
    file.setFileName("hardware_log_" + QString::number(archive_lenght) + ".txt");
    file.remove();
    file.setFileName("software_log_" + QString::number(archive_lenght) + ".txt");
    file.remove();

    for(int n=archive_lenght; n>0; n--)
    {
        file.setFileName("hardware_log_" + QString::number(n-1) + ".txt");
        file.rename("hardware_log_" + QString::number(n) + ".txt");
        file.setFileName("software_log_" + QString::number(n-1) + ".txt");
        file.rename("software_log_" + QString::number(n) + ".txt");
    }
}
