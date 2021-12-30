#include <QtCore/QCoreApplication>

#include "ProgramExecuteThread.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ProgramExecuteThread program_execute_thread;

    program_execute_thread.start(QThread::NormalPriority);
    return a.exec();
}
