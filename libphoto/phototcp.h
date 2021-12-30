/*!
  \file
  \brief Заголовочный файл описания класса TPhotoTcp

  Данный файл содержит в себе определения
  класса TPhotoTcp. Используется для получения изображения через tcp протокол.
*/
#ifndef PHOTOTCP_H
#define PHOTOTCP_H

#include <QtCore/QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QNetworkInterface>
#include <QStringList>
#include <QThread>
#include <QBasicTimer>
#include <QTimer>
#include <stdlib.h>
#include "libphoto_global.h"
#include "request_photo.h"

// error from server
/// @brief Операция выполнена успешно.
#define OK_ERR 0          // all ok
/// @brief Невозможно запустить программу raspistill на сервере.
#define CANTSTART_ERR 1   // can't start external program
/// @brief Программа raspistill на сервере не закончила работу.
#define CANTEND_ERR   2   // can't wait finish of external propgram
/// @brief I/O ошибка чтения/записи файла на сервере.
#define FILERW_ERR    3   // error for file read write (may be file can't found)
/// @brief При выполнении сетевой операции произошла ошибка таймаута.
#define TIMEOUT_ERR   4
/// @brief I/O ошибка записи файла на локальном компьютере.
#define LOCFILERW_ERR 5
/// @brief Неизвестная ошибка.
#define UNKNOWN_ERR   10

/*!
  \brief Используется для получения изображения от сервера RPI через tcp протокол.

  Класс TPhotoHttp используется для получения изображения через tcp протокол.
  Изображения хранятся в каталоге /home/user/prog/datajpg.
  Настройка параметров изображения осуществляется через web интерфейс на странице
  http://IPaddress/index.html
*/
class LIBPHOTOSHARED_EXPORT TPhotoTcp : public QObject
{
    Q_OBJECT
public:
    TPhotoTcp();
    TPhotoTcp(QString ip,int port=9002);
    ~TPhotoTcp();
    int link(QString ip,int port=9002);

    int makeInstantPhoto(QString fotoFileName="imgtmp.jpg");
    int makePhoto(QString fotoFileName="imgtmp.jpg");
    int makePhoto(QString fotoFileName,QString saveFileName);
    int makeVideo(QString fotoFileName="imgtmp.avi",uint32_t vTime=5000);
    int getFile(QString fotoFileName,QString saveFileName);
    int getListFiles(QStringList &fl);
    int removeFiles(QStringList fl);
    int getStatus(void);
    /*!
      Назначение временного таймаута для сетевых операций. По умолчанию установлен на 10 с.
      \param[in] ms таймаут в мс.
    */
    void setTimeout(int ms){timoutNWConnection=ms;}
    /*!
      Считывание временного таймаута для сетевых операций.
      \return таймаут в мс.
    */
    int getTimeout(void) { return timoutNWConnection ; }
    /*!
      Возвращает строку с описанием ошибки.
      \return QString строка с описанием ошибки.
      \sa В файле phototcp.h содержатся описание возвращаемых кодов.
    */
    QString getError(void) { return nwError; }

private:
// Methods
    void create_TimerRequest(void);
    int waitServer(int tout);
    void msleep(qint64 msec)
    {
      QEventLoop loop;
      QTimer::singleShot(msec, &loop, SLOT(quit()));
      loop.exec();
    }
    int errorProc(int); // process code of error and set nwError variable
    //... NetWork ...
    QString ipAddress;
    int ipPort;
    int timoutNWConnection;
    QTcpSocket  *m_pTcpSocket;           // socket for check connection status for every Server_Dev's
    quint32      m_nNextBlockSize;
    QString nwError;
    QMap<QString,QString> map_ConnectedStatus;   // status of connected
    QMap<QString,QString> map_MakePhotoHeader;
    QMap<QString,QByteArray> map_PhotoData;
    QMap<QString,uint32_t> map_PhotoStatus;
    QMap<QString,QStringList> map_FilesList, map_DelFilesList;
    uint32_t ph_error,srvConnect;
// processing requests
    QStringList Catalogue_Requests;
    QMap<QString, QBasicTimer*> Map_TimerRequest;
    QList<QString> List_Requests;
    int progress,progressSts,progressFL,progressGD,progressDF,progressVideo;
    bool phTOut,dirListTOut,getStsTOut,getFileTOut,delFilesTOut,videoTOut;
public slots:
    /// @private
    void slot_Connected();
    /// @private
    void slot_readyRead();
    /// @private
    void slot_sendToServer(QString);
    /// @private
    void slot_error(QAbstractSocket::SocketError);
protected:
    /// @private
    void timerEvent(QTimerEvent *);
};

#endif
