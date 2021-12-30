#include "codescanobject.h"
#include <linux/input.h>
#include <QDebug>


#define EV_MAKE   1  // when key pressed

CodeScanObject::CodeScanObject(QObject *parent,Settings *settings) :
    QObject(parent)
{
    barcode = "";
    //listManufacturers=settings->BSManufacturer.split(" ");

    listProducts=settings->scanners;


    get_device_process = new QProcess(this);
    scan_process = new QProcess(this);
    connect(get_device_process, SIGNAL(finished(int)), this, SLOT(getDeviceProcessFinished()));
    connect(scan_process, SIGNAL(readyReadStandardOutput()), this, SLOT(scanOutputReady()));
    //connect(scan_process, SIGNAL(readyReadStandardError()),this
     //scan_process->start("cat /dev/input/event1");
}

CodeScanObject::~CodeScanObject(void)
{

}

void CodeScanObject::timerEvent(QTimerEvent *event)
{
    event = event;  //dummy remove warning

    if(get_device_process->state() == QProcess::NotRunning
            && scan_process->state() == QProcess::NotRunning)
    {
        get_device_process->start("cat /proc/bus/input/devices");
    }
   // qDebug()<<"Timer";
    //startTimer(2000);
}

void CodeScanObject::getDeviceProcessFinished(void)
{
    QString text = QString::fromLocal8Bit(get_device_process->readAllStandardOutput());
    QStringList list = text.split("\n");

    QString device;
//qDebug()<<"list"<<list;
    bool findOk;
    for(int n=0; n<list.count(); n++)
    {
        findOk=false;
        if(listProducts.empty()) { listProducts<<"Honeywell:Scanner"<<"Honeywell:CCB04"; }
        for(int k=0;k<listProducts.count();k++){
            QStringList tmp=listProducts.at(k).simplified().split(':');
            if(list.at(n).contains(tmp.at(0),Qt::CaseInsensitive) && list.at(n).contains(tmp.at(1),Qt::CaseInsensitive)) {findOk=true;break;}
        }
        //if( (list.at(n).contains("Honeywell",Qt::CaseInsensitive)) &&
        //  ( (list.at(n).contains("Scanner",Qt::CaseInsensitive))||(list.at(n).contains("Imaging",Qt::CaseInsensitive)) ) )
        if(findOk)
        {
            for(int m=n+1;m<list.count();m++){
              if(list.at(m).contains("Handlers=kbd")){

                device = list.at(m).simplified();
                device = device.remove("H: Handlers=kbd ");
               // emit deviceFound(device);
                QString input_event_device = "/dev/input/" + device;
                qDebug()<<input_event_device;
                scan_process->start("cat " + input_event_device);
                return;
              }
            }
        }
    }
}

void CodeScanObject::scanOutputReady(void)
{
    struct input_event event;
    char* c_ptr;
    QByteArray byte_array = scan_process->readAllStandardOutput();

    for(uint e=0; e<byte_array.size()/sizeof(event); e++)
    {
        c_ptr = (char*) &event;
        for(uint n=0; n<sizeof(event); n++) *(c_ptr++) = byte_array.at(e*sizeof(event) + n);

        if (event.type == EV_KEY && event.value == EV_MAKE)
        {
            switch (event.code)
            {
                case KEY_0: barcode += "0"; break;
                case KEY_1: barcode += "1"; break;
                case KEY_2: barcode += "2"; break;
                case KEY_3: barcode += "3"; break;
                case KEY_4: barcode += "4"; break;
                case KEY_5: barcode += "5"; break;
                case KEY_6: barcode += "6"; break;
                case KEY_7: barcode += "7"; break;
                case KEY_8: barcode += "8"; break;
                case KEY_9: barcode += "9"; break;

                case KEY_A: barcode += "A"; break;
                case KEY_B: barcode += "B"; break;
                case KEY_C: barcode += "C"; break;
                case KEY_D: barcode += "D"; break;
                case KEY_E: barcode += "E"; break;

                case KEY_F: barcode += "F"; break;
                case KEY_G: barcode += "G"; break;
                case KEY_H: barcode += "H"; break;
                case KEY_I: barcode += "I"; break;
                case KEY_J: barcode += "J"; break;

                case KEY_K: barcode += "K"; break;
                case KEY_L: barcode += "L"; break;
                case KEY_M: barcode += "M"; break;
                case KEY_N: barcode += "N"; break;
                case KEY_O: barcode += "O"; break;

                case KEY_P: barcode += "P"; break;
                case KEY_Q: barcode += "Q"; break;
                case KEY_R: barcode += "R"; break;
                case KEY_S: barcode += "S"; break;
                case KEY_T: barcode += "T"; break;

                case KEY_U: barcode += "U"; break;
                case KEY_V: barcode += "V"; break;
                case KEY_W: barcode += "W"; break;
                case KEY_X: barcode += "X"; break;
                case KEY_Y: barcode += "Y"; break;

                case KEY_Z: barcode += "Z"; break;

                case KEY_DOT: barcode += "."; break;
                case KEY_COMMA: barcode += ","; break;

                case KEY_LEFTSHIFT: break;

                case KEY_ENTER:
                {
                    emit barcodeReady(barcode);
                    barcode = "";
                    break;
                }
            default: { barcode += "?"; qDebug()<<"?bar char="<<event.code;}
            }


        }
    }
}
