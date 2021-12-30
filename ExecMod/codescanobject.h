#ifndef CODESCANOBJECT_H
#define CODESCANOBJECT_H

#include <QObject>
#include <QProcess>
#include "settings.h"

class CodeScanObject : public QObject
{
    Q_OBJECT
public:
    explicit CodeScanObject(QObject *parent = 0, Settings *settings=0);
    ~CodeScanObject(void);

    void start(void){startTimer(1000);}
signals:
    void deviceFound(QString name);
    void barcodeReady(QString code);
protected:
    void timerEvent(QTimerEvent *event);
private slots:
    void getDeviceProcessFinished(void);
    void scanOutputReady(void);
private:
    QProcess* get_device_process;
    QProcess* scan_process;
    QString barcode;
    QStringList listManufacturers,listProducts;
};

#endif // CODESCANOBJECT_H
