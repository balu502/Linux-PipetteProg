/*!
  \file photohttp.h
  \brief Заголовочный файл описания класса TPhotoHttp

  Данный файл содержит в себе определения
  класса TPhotoHttp. Используется для получения изображения через http протокол.
*/

#ifndef PHOTOHTTP_H
#define PHOTOHTTP_H
#include <QtDebug>
#include <QThread>
#include <QEventLoop>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QTextCodec>
#include <QFile>
#include <QDir>
#include <QTimer>
#include "libphoto_global.h"

/// @brief Операция выполнена успешно.
#define ERR_NONE       0
/// @brief Произошел таймаут во время операции.
#define ERR_TOUT       1
/// @brief Сервер вернул пустую строку, чего не должно было быть. Возможно ошибка в файле glfulf.php
#define ERR_EMPTY      2
/// @brief Ошибка выполнения программы на сервере.
#define ERR_OPERATION  3
/// @brief Пришел неожиданный ответ от сервера..
#define ERR_UANSWER    4
/// @brief Задано пустое имя файла.
#define ERR_FNEMPTY    5
/// @brief Запрашиваемый на сервере файл не существует.
#define ERR_FEXIST     6
/// @brief Ошибка открытия локального файла на чтение.
#define ERR_FOPEN      7
/// @brief Ошибка открытия локального файла на запись.
#define ERR_FWRITE     8
/// @brief Файл /var/www/html/ocp/php/glfulf.php не найден.
#define ERR_NFPHP      9

/// \brief 50 мс задержка
const int sleep_time=50;  // sleep time in ms for NW operation
/*!
  \brief Используется для получения изображения от сервера RPI через http протокол.

  Класс TPhotoHttp используется для получения изображения через http протокол.
  Изображения хранятся в каталоге /var/www/html/ocr/datajpg.
  Настройка параметров изображения осуществляется через web интерфейс на странице
  http://IPaddress/index.html
*/
class LIBPHOTOSHARED_EXPORT TPhotoHttp :public QObject
{
  Q_OBJECT

public:
  explicit TPhotoHttp();
  explicit TPhotoHttp(QString);
  ~TPhotoHttp();

   QString getError(void); // return description of error
   /*!
     Назначение IP адреса сервера.
     \param[in] address IP адрес сервера.
   */
   void setDeviceAddress(QString address){SERVICE_URL="http://"+address+"/ocr/php/glfulf.php";}
   int makePhoto(QString fn="");
   int getListJPEGFiles(QStringList &fl);  // get list files drom directiry  datajpg (/var/www/html/ocr/datajpg)
   int getJPEGFile(QString fn,QString fns); // get jpeg file from directory datajpg (/var/www/html/ocr/datajpg)
   int removeJPEGFile(QString fn); // remove file fn from directory datajpg (/var/www/html/ocr/datajpg)
   int removeAllJPEGFiles(void);  // remove all file from directory datajpg (/var/www/html/ocr/datajpg)
   /*!
     Назначение временного таймаута для сетевых операций. По умолчанию установлен на 10 с.
     \param[in] ms таймаут в мс.
   */
   void setTimeout(uint32_t ms) {timeOut_ms=ms ; }
   /*!
     Считывание временного таймаута для сетевых операций.
     \return таймаут в мс.
   */
   int getTimeout(void) { return timeOut_ms ; }
private slots:
  void replyFinishedNetworkReq(QNetworkReply*); // slot on reply from device

private:
  void msleep(qint64 msec)
  {
    QEventLoop loop;
    QTimer::singleShot(msec, &loop, SLOT(quit()));
    loop.exec();
  }
  QString SERVICE_URL;
  QNetworkAccessManager  *manager;
  QString hostName;

  bool reqCompleate;  // true after request
  int error;          // error code
  QString uError;     // return string when unknown answer recieved
  QByteArray data;    // data with answer from device
  int timeOut_ms; // network operation timeout in ms

};
#endif // PHOTOHTTP_H
