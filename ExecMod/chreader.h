#ifndef CHREADER_H
#define CHREADER_H
#include <QObject>
#include <QProcess>


class TChannelsReader: public QObject{

	Q_OBJECT
	
public:
	TChannelsReader(QObject* p=0): QObject(p){endProc=false;}
        void setProcStatus(bool sts){ endProc=sts; }
        bool readProcStatus(void){ return endProc; }
private:
   bool endProc;
public slots:
	void readChOutput();
	void readChError();	
	void endProcess(int exitCode, QProcess::ExitStatus exitStatus);
};
#endif // CHREADER_H
