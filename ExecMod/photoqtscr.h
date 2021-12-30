#ifndef PHOTOQTSCR_H
#define PHOTOQTSCR_H

#include <QObject>
#include <QString>
#include <time.h>
#include <phototcp.h>

class TPhotoQtScr : public QObject
{
    Q_OBJECT
public:
    explicit TPhotoQtScr(QObject *parent = 0);
    ~TPhotoQtScr();
    void setCameraIP(QString ip){ IP=ip; }
private:
 TPhotoTcp *photo;
 QString IP;
 QString lastSaveFile;

signals:

public slots:
// function for photo
    int setLink(QString ip="", int port=9002);
    QString getCameraError(void);
    int getCameraStatus(void);
    int makePhoto(QString phName="",int typePhoto=0);
    int makeVideo(QString videoName,int timeDuration=5000);
    int getMediaFile(QString getMediaFileName,QString saveMediaFileName);
    QString getListFiles(void);
    int removeFiles(QString list);
};

#endif // PHOTOQTSCR_H
