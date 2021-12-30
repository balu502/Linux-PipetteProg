/*!
  \file photohttp.cpp
  \brief Реализация методов класса TPhotoHttp.

  Реализация методов класса TPhotoHttp.
*/
//#include <w32api.h>
#include "photohttp.h"
#ifdef Q_OS_WIN
  /// Определение версии windows не ниже XP
  #define WINVER WindowsXP
  #include <windows.h>
#endif

/*!
  Конструктор класса. После создания класса необходимо вызываеть
  функцию setDeviceAddress(), для назначения IP адреса сервера.
  \sa setDeviceAddress()
*/
TPhotoHttp::TPhotoHttp()
{
  //loop =new QEventLoop();
  manager = new QNetworkAccessManager(this);
  hostName="127.0.0.1";
  setDeviceAddress(hostName); // set local
  uError="";
  timeOut_ms=10000;
}

/*!
  Конструктор класса.
  \param[in] ip IP4 адрес сервера в формате xx.xx.xx.xx, например:(192.168.31.76)
*/
TPhotoHttp::TPhotoHttp(QString ip):hostName(ip)
{
  //loop =new QEventLoop();
  manager = new QNetworkAccessManager(this);
  if(hostName.isEmpty()) setDeviceAddress("127.0.0.1"); // set local
  else
    setDeviceAddress(hostName);
  uError="";
  timeOut_ms=10000;
}

/*!
  Деструктор.
*/
TPhotoHttp::~TPhotoHttp()
{
  if(manager) {delete manager; manager=0;}
  data.clear();
}

//********************** Public functions ***********************
//
//--------------------Make photo and put them into directory datajpg (/var/www/html/ocr/datajpg) on the device . Return 0 if OK
//
/*!
  Получение изображения на сервере и помещение его в каталог /var/www/html/ocr/datajpg.
  Передает управление вызывающей программе после получения ответа от сервера, что
  изображение сформировано, или при выполнении условия таймаута.
  \param[in] fotoFileName имя файла с сохраняемым изображением. Если отсутствует, то на сервере изображение
  по умолчанию будет сохранено в файл imgtmp.jpg.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoHttp::makePhoto(QString fotoFileName)
{
  QUrl url(SERVICE_URL);
  QNetworkRequest request(url);
  QByteArray postData("make_photo=");
  postData.append(fotoFileName);

  reqCompleate=false;
  error=ERR_NONE;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  connect(manager, SIGNAL(finished(QNetworkReply*)),
           this, SLOT(replyFinishedNetworkReq(QNetworkReply*)));
  manager->post(request,postData);

  int tout=0;
  while(!reqCompleate) {
    //loop->processEvents();
    msleep(sleep_time);
    if(tout++>timeOut_ms/sleep_time){
      error=ERR_TOUT; break;
    }
  }
  disconnect(manager,0,0,0);
  if(!error){
    bool ok=false;
    int ret=data.toInt(&ok);
    if(ok){
      if(ret) error=ERR_OPERATION;
   }
    else {
      uError=data;
      error=ERR_UANSWER;
    }
  }
  return error;
}

//
//--------------------Get file list fl from  directory datajpg (/var/www/html/ocr/datajpg) on the device. Return 0 if OK
//
/*!
  Получение содержимое каталога /var/www/html/ocr/datajpg на сервере.

  \param[out] fl ссылка на переменную QStringList в которой сохраняются имена файлов каталога /var/www/html/ocr/datajpg с изображениями.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoHttp::getListJPEGFiles(QStringList &fl)
{
  QUrl url(SERVICE_URL);
  QNetworkRequest request(url);
  QByteArray postData("getlist_jpegfiles");

  reqCompleate=false;
  error=ERR_NONE;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  connect(manager, SIGNAL(finished(QNetworkReply*)),
           this, SLOT(replyFinishedNetworkReq(QNetworkReply*)));
  manager->post(request,postData);

  int tout=0;
  while(!reqCompleate) {
    //loop->processEvents();
    msleep(sleep_time);
    if(tout++>timeOut_ms/sleep_time){
      error=ERR_TOUT; break;
    }
  }
  fl.clear();
  disconnect(manager,0,0,0);
  if(!error){
    QString st(data);
    fl.append(st.simplified().split(' '));
  }
  return error;
}

//
//--------------------  Get file fn from directory datajpg (/var/www/html/ocr/datajpg) on the device
//                      and save one as fns. Return 0 if OK
//
/*!
  Получение содержимого заданного файла с сервера. На локальной машине файл сохраняется в текущем каталоге.

  \param[in] fn имя файла с изображением в каталоге /var/www/html/ocr/datajpg на сервере.
  \param[in] fns имя файла под которым содержимое файла fn будет сохранено на локальной машине.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
  */
int TPhotoHttp::getJPEGFile(QString fn,QString fns)
{
  QUrl url(SERVICE_URL);
  QNetworkRequest request(url);
  QByteArray postData("get_jpegfile=");
  postData.append(fn);

  reqCompleate=false;
  error=ERR_NONE;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  connect(manager, SIGNAL(finished(QNetworkReply*)),
           this, SLOT(replyFinishedNetworkReq(QNetworkReply*)));

  manager->post(request,postData);
  int tout=0;
  while(!reqCompleate) {
    //loop->processEvents();
    msleep(sleep_time);
    if(tout++>timeOut_ms/sleep_time){
      error=ERR_TOUT; break;
    }
  }
  disconnect(manager,0,0,0);
  if(!error){
    QFile fileWr(fns);
    if(!fileWr.open(QIODevice::WriteOnly)) {
      error=ERR_FWRITE;
      return error;
    }
    fileWr.write(data);
    fileWr.close();
  }
  return error ;
}

//
//-------------------- Delete file fn from directory datajpg (/var/www/html/ocr/datajpg). Return 0 if OK
//
/*!
  Удаление содержимого заданного файла в каталоге /var/www/html/ocr/datajpg на сервере.

  \param[in] fn имя файла с изображением в каталоге /var/www/html/ocr/datajpg на сервере.

  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoHttp::removeJPEGFile(QString fn)
{
  QUrl url(SERVICE_URL);
  QNetworkRequest request(url);
  QByteArray postData("remove_jpegfile=");
  postData.append(fn);

  reqCompleate=false;
  error=ERR_NONE;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  connect(manager, SIGNAL(finished(QNetworkReply*)),
           this, SLOT(replyFinishedNetworkReq(QNetworkReply*)));

  manager->post(request,postData);

  int tout=0;
  while(!reqCompleate) {
    //loop->processEvents();
    msleep(sleep_time);
    if(tout++>timeOut_ms/sleep_time){
      error=ERR_TOUT;  break;
    }
  }
  disconnect(manager,0,0,0);
  if(!error){
    bool ok=false;
    int ret=data.toInt(&ok);
    if(ok){
      if(ret) error=ERR_OPERATION;
    }
    else {
      uError=data;
      error=ERR_UANSWER;
    }
  }
  return error;
}
//
//-------------------- Delete all files fn from directory datajpg (/var/www/html/ocr/datajpg). Return 0 if OK
//
/*!
  Удаление всех файлов в каталоге /var/www/html/ocr/datajpg на сервере.

  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoHttp::removeAllJPEGFiles(void)
{
  QUrl url(SERVICE_URL);
  QNetworkRequest request(url);
  QByteArray postData("remove_alljpeg");

  reqCompleate=false;
  error=ERR_NONE;

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  connect(manager, SIGNAL(finished(QNetworkReply*)),
           this, SLOT(replyFinishedNetworkReq(QNetworkReply*)));

  manager->post(request,postData);

  int tout=0;
  while(!reqCompleate) {
    //loop->processEvents();
    msleep(sleep_time);
    if(tout++>timeOut_ms/sleep_time){
      error=ERR_TOUT;  break;
    }
  }
  disconnect(manager,0,0,0);
  if(!error){
    bool ok=false;
    int ret=data.toInt(&ok);
    if(ok){
      if(ret) error=ERR_OPERATION;
    }
    else {
      uError=data;
      error=ERR_UANSWER;
    }
  }
  return error;
}

// Convert error code into string
/*!
  Возвращает строку с описанием ошибки.
  \return QString строка с описанием ошибки.
  \sa В файле photohttp.h содержатся описание возвращаемых кодов.
*/
QString TPhotoHttp::getError(void)
{
  switch(error){
    case ERR_NONE:       return "";
    case ERR_TOUT:       return tr("Timeout network operation. Can't recieved answer from device.");
    case ERR_EMPTY:      return tr("Device return empty answer.");
    case ERR_OPERATION:  return tr("Can't execute operation.");
    case ERR_UANSWER:    return (tr("Device return unknown answer... ")+uError);
    case ERR_FNEMPTY:    return tr("Input file for execute is empty.");
    case ERR_FEXIST:     return tr("File not exists.");
    case ERR_FOPEN:      return tr("File open I/O error.");
    case ERR_FWRITE:     return tr("File write I/O error.");
    case ERR_NFPHP:      return tr("Execute PHP file not found on device.");
    default:             return tr("");
  }
}

//********************** Private SLOTS functions ***********************
// answer on request from device
void TPhotoHttp::replyFinishedNetworkReq(QNetworkReply *reply)
{
  data.clear();
  data = reply->readAll();
  while(1){
    if(data.isEmpty()) { error=ERR_EMPTY; break; }
    if(data.startsWith("-1")) { error=ERR_FNEMPTY; break; }
    if(data.startsWith("-2")) { error=ERR_FEXIST; break; }
    if(data.startsWith("File not found")) { error=ERR_NFPHP; break; }
    break;
  }
  reqCompleate=true;
}


