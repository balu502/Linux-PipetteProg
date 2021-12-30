#include <QDebug>
#include <QStringList>
#include <QString>

#include "chreader.h"

void TChannelsReader::readChOutput(){

	QProcess* pcs = qobject_cast<QProcess *>( sender() );

	QByteArray byteArray = pcs->readAllStandardOutput();
	QStringList strLines = QString(byteArray).split("\n");

	foreach (QString line, strLines)
		qDebug("%s", line.toStdString().c_str());
}


void TChannelsReader::readChError(){

	QProcess* pcs = qobject_cast<QProcess *>( sender() );

	QByteArray byteArray = pcs->readAllStandardError();
	QStringList strLines = QString(byteArray).split("\n");

	foreach (QString line, strLines)
		qDebug("%s", line.toStdString().c_str());
}
void TChannelsReader::endProcess(int exitCode, QProcess::ExitStatus exitStatus)
{
  setProcStatus(true);
  qDebug()<<"pr"<<endProc;
}
