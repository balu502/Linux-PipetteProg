/*!
  \file phototcp.cpp
  \brief Реализация методов класса TPhotoTcp.

  Реализация методов.
*/
#include "QApplication"
#include "phototcp.h"

// Constructors
/*!
  Конструктор класса. После создания класса необходимо вызываеть
  функцию link(), для назначения IP адреса сервера и соединения с ним.
  \sa link()
*/
TPhotoTcp::TPhotoTcp()
{
  qDebug()<<"TPhotoTcp object constructor start.";
  Catalogue_Requests << CATALOGUE;
  create_TimerRequest();
  m_nNextBlockSize = 0;
  m_pTcpSocket = new QTcpSocket(this);
  connect(m_pTcpSocket, SIGNAL(connected()), SLOT(slot_Connected()));
  connect(m_pTcpSocket, SIGNAL(readyRead()), SLOT(slot_readyRead()));
  connect(m_pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
          this,     SLOT(slot_error(QAbstractSocket::SocketError)));

  timoutNWConnection=10000;
  nwError="Absent network connection.";
  ph_error=0;
  qDebug()<<"TPhotoTcp object constructor end.";
}
/*!
  Конструктор класса. После вызова конструктора для дальнейшей работы необходимо убедиться,
  в том, что соединение с сервером установлено. Для этого вызвать функцию getError(). Если она
  вернула пустую строку, то соединение установлено и можно продолжить работу.
  \param[in] ip IP4 адрес сервера в формате xx.xx.xx.xx, например:(192.168.31.76)
  \param[in] port Порт связи с сервером. По умолчанию 9002.
  \sa getError()
*/
TPhotoTcp::TPhotoTcp(QString ip,int port)
{
  qDebug()<<"TPhotoTcp object constructor start.";
  Catalogue_Requests << CATALOGUE;
  create_TimerRequest();
  m_nNextBlockSize = 0;
  m_pTcpSocket = new QTcpSocket(this);
  connect(m_pTcpSocket, SIGNAL(connected()), SLOT(slot_Connected()));
  connect(m_pTcpSocket, SIGNAL(readyRead()), SLOT(slot_readyRead()));
  connect(m_pTcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
          this,     SLOT(slot_error(QAbstractSocket::SocketError)));

  timoutNWConnection=10000;
  nwError="Absent network connection.";
  ph_error=0;
  link(ip,port);
  qDebug()<<"TPhotoTcp object constructor end.";
}

//destructor
/*!
  Деструктор.
*/
TPhotoTcp::~TPhotoTcp()
{
 qDebug()<<"TPhotoTcp object destructor begin.";
 QBasicTimer *timer;
 List_Requests.clear();
 foreach(timer, Map_TimerRequest) {
   timer->stop();
   delete timer;
 }
 Map_TimerRequest.clear();
 map_FilesList.clear();
 map_DelFilesList.clear();
 map_PhotoStatus.clear();
 map_PhotoData.clear();
 map_MakePhotoHeader.clear();
 map_ConnectedStatus.clear();
 m_pTcpSocket->deleteLater();
 qDebug()<<"TPhotoTcp object destructor end.";
}

//-----------------------------------------------------------------------------
//--- create_TimerRequest()
//-----------------------------------------------------------------------------
void TPhotoTcp::create_TimerRequest(void)
{
  int i;
  QString request;

  for(i=0; i<Catalogue_Requests.size(); i++){
    request = Catalogue_Requests.at(i);
    QBasicTimer *timer = new QBasicTimer();
    Map_TimerRequest.insert(request, timer);
  }
}


//
//--------------------Get file list fl from  directory datajpg (/home/user/prog/datajpg) on the device.
// Return code error 0 if Ok. call getError() for read error
//
/*!
  Получаение содержимое каталога /var/www/html/ocr/datajpg на сервере.

  \param[out] fl ссылка на переменную QStringList в которой сохраняются имена файлов каталога /var/www/html/ocr/datajpg с изображениями.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoTcp::getListFiles(QStringList &fl)
{
  progressFL=0;
  ph_error=0;
  nwError="";
  fl.clear();
  dirListTOut=false;
  slot_sendToServer(GETDIRLIST_REQUEST);

  while(progressFL==0){
    if(dirListTOut) break;
    QApplication::processEvents();
  }
  if(dirListTOut) return errorProc(TIMEOUT_ERR); // timeout
  if(ph_error) return errorProc(ph_error);
  fl=map_FilesList.value(GETDIRLIST_DATA);
  return(OK_ERR);
}

/*!
  Удаление заданных файлов в каталоге /var/www/html/ocr/datajpg на сервере.

  \param[in] fl QStringList с именами файлов в каталоге /var/www/html/ocr/datajpg на сервере.

  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoTcp::removeFiles(QStringList fl)
{
  progressDF=0;
  ph_error=0;
  nwError="";
  delFilesTOut=false;
  map_DelFilesList.clear();
  map_DelFilesList.insert(GETDIRLIST_DATA,fl);
  slot_sendToServer(DELFILESLIST_REQUEST);

  while(progressDF==0){
    if(delFilesTOut) break;
    QApplication::processEvents();
  }
  if(delFilesTOut) return errorProc(TIMEOUT_ERR); // timeout
  return(errorProc(ph_error));
}
/*!
  Моментальное получение изображения на сервере и помещение его в каталог /var/www/html/ocr/datajpg
  Передает управление вызывающей программе сразу же после получении ответа от сервера, что
  изображение начало формироваться, или при выполнении условия таймаута.
  \param[in] fotoFileName имя файла с сохраняемым изображением. Если отсутствует, то на сервере изображение
  по умолчанию будет сохранено в файл imgtmp.jpg.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoTcp::makeInstantPhoto(QString fotoFileName)
{
  progress=0;
  ph_error=0;
  phTOut=false;
  map_MakePhotoHeader.clear();
  map_MakePhotoHeader.insert(PHOTO_TYPE,PHOTO_TYPE_I);
  map_MakePhotoHeader.insert(PHOTO_FNAME,fotoFileName);
  slot_sendToServer(PHOTO_REQUEST);
  while(progress==0){
    if(phTOut) break;
    QApplication::processEvents();
  }
  if(phTOut) return errorProc(TIMEOUT_ERR); // timeout
  return errorProc(ph_error);
}

/*!
  Получение изображения на сервере и помещение его в каталог /var/www/html/ocr/datajpg.
  Передает управление вызывающей программе после получения ответа от сервера, что
  изображение сформировано, или при выполнении условия таймаута.
  \param[in] fotoFileName имя файла с сохраняемым изображением. Если отсутствует, то на сервере изображение
  по умолчанию будет сохранено в файл imgtmp.jpg.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoTcp::makePhoto(QString fotoFileName)
{
  progress=0;
  ph_error=0;
  phTOut=false;
  map_MakePhotoHeader.clear();
  map_MakePhotoHeader.insert(PHOTO_TYPE,PHOTO_TYPE_WF);
  map_MakePhotoHeader.insert(PHOTO_FNAME,fotoFileName);
  slot_sendToServer(PHOTO_REQUEST);
  while(progress==0){
    if(phTOut) break;
    QApplication::processEvents();
  }
  if(phTOut) return errorProc(TIMEOUT_ERR); // timeout
  return errorProc(ph_error);
}

/*!
  Получение изображения на сервере с помещением его в каталог /var/www/html/ocr/datajpg и
  скачиванием файла с сервера и сохранение его в текущий каталог локального компьютера.
  Передает управление вызывающей программе после получения от сервера файла с изображением или при выполнении условия таймаута.
  \param[in] fotoFileName имя файла с сохраняемым изображением. Если отсутствует, то на сервере изображение
  по умолчанию будет сохранено в файл imgtmp.jpg.
  \param[in] saveFileName Имя файла под которым он записывается в текущий каталог локального компьютера.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoTcp::makePhoto(QString fotoFileName,QString saveFileName)
{
  progress=0;
  ph_error=0;
  phTOut=false;
  map_MakePhotoHeader.clear();
  map_MakePhotoHeader.insert(PHOTO_TYPE,PHOTO_TYPE_WF);
  map_MakePhotoHeader.insert(PHOTO_FNAME,fotoFileName);
  slot_sendToServer(PHOTO_REQUEST);
  while(progress==0){
    if(phTOut) break;
    QApplication::processEvents();
  }
  if(phTOut) return errorProc(TIMEOUT_ERR); // timeout
  if(ph_error) return errorProc(ph_error);
  QFile file(saveFileName);
  if(!file.open(QIODevice::WriteOnly)) return errorProc(LOCFILERW_ERR);
  file.write(map_PhotoData.value(GETDATA_DATA),map_PhotoData.value(GETDATA_DATA).size());
  file.close();
  return(OK_ERR);
}

/*!
  Получение видео изображения на сервере и помещение его в каталог /var/www/html/ocr/datajpg.
  Передает управление вызывающей программе после получения ответа от сервера, что
  изображение сформировано, или при выполнении условия таймаута.
  \param[in] videoFileName имя файла с сохраняемым изображением. Если отсутствует, то на сервере изображение
  по умолчанию будет сохранено в файл imgtmp.avi.
  \param[in] vTime время записываемого видеофайла в мс. По умолчантю 10000 милисекунд.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoTcp::makeVideo(QString videoFileName,uint32_t vTime)
{
  progressVideo=0;
  ph_error=0;
  videoTOut=false;
  map_MakePhotoHeader.clear();
  map_MakePhotoHeader.insert(PHOTO_TYPE,PHOTO_TYPE_V);
  map_MakePhotoHeader.insert(PHOTO_FNAME,videoFileName);
  map_MakePhotoHeader.insert(VIDEO_TIME,QString("%1").arg(vTime));
  slot_sendToServer(PHOTO_REQUEST);
  while(progressVideo==0){
    if(videoTOut) break;
    QApplication::processEvents();
  }
  if(phTOut) return errorProc(TIMEOUT_ERR); // timeout
  return errorProc(ph_error);
}


/*!
  Получение изображения с сервера и сохранение его в текущий каталог локального компьютера.
  Передает управление вызывающей программе после получения от сервера файла с изображением или при выполнении условия таймаута.
  \param[in] fotoFileName имя файла с изображением.
  \param[in] saveFileName Имя файла под которым он записывается в текущий каталог локального компьютера.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoTcp::getFile(QString fotoFileName,QString saveFileName)
{
  progressGD=0;
  ph_error=0;
  getFileTOut=false;
  map_MakePhotoHeader.clear();
  map_MakePhotoHeader.insert(PHOTO_TYPE,PHOTO_TYPE_WF);
  map_MakePhotoHeader.insert(PHOTO_FNAME,fotoFileName);
  slot_sendToServer(GETDATA_REQUEST);
  while(progressGD==0){
    if(getFileTOut) break;
    QApplication::processEvents();
  }
  if(getFileTOut) return errorProc(TIMEOUT_ERR); // timeout
  if(ph_error) return errorProc(ph_error);
  QFile file(saveFileName);
  if(!file.open(QIODevice::WriteOnly)) return(errorProc(LOCFILERW_ERR));
  file.write(map_PhotoData.value(GETDATA_DATA),map_PhotoData.value(GETDATA_DATA).size());
  file.close();
  return(OK_ERR);
}

// return status of photo process on RPI device 0-1 bits status of process
// 0 - process not run 1 process running 2 process run
// 2 bit - timeout operation
/*!
  Возвращает статус получения изображения на сервере.
  \return Код возврата. 0 - Изображение получено. 1 - программа получения изображения запускается.
  2 - программа получения изображения запущена. 4 - таймаут сетевой операции.
  В случае таймаута, описание ошибки можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoTcp::getStatus(void)
{
  progressSts=0;
  getStsTOut=false;
  slot_sendToServer(STATUS_REQUEST);
  while(progressSts==0){
    if(getStsTOut) break;
    QApplication::processEvents();
  }
  if(getStsTOut) return errorProc(TIMEOUT_ERR); // timeout
  return(map_PhotoStatus.value(STATUS_PROGRESS));
}


//---------------------- NETWORK ----------------------------------------------
//-----------------------------------------------------------------------------
//--- link
//-----------------------------------------------------------------------------
/*!
  Назначает IP адрес и порт сервера. В случае успешного соединения блокирует сервер от возможности соединения с
  другой копией программы.
  Передает управление вызывающей программе после подтверждения о соединение с сервером или при выполнении условия таймаута.
  В случае запуска второй и последующих копий возвращается ошибочное состояние.
  \param[in] ip  IP4 адрес сервера в формате xx.xx.xx.xx, например:(192.168.31.76)
  \param[in] port Порт связи с сервером. По умолчанию 9002.
  \return Код возврата. 0 если нет ошибки. В случае ошибки, описание можно посмотреть, вызвав метод
  getError(), возвращающий QString.
*/
int TPhotoTcp::link(QString ip,int port)
{
  ipAddress=ip;
  ipPort=port;
  srvConnect=0;
  qDebug()<<"TRY connect";

  if(m_pTcpSocket->state() == QAbstractSocket::UnconnectedState) {
    m_pTcpSocket->connectToHost(ip, port);
    if(m_pTcpSocket->waitForConnected(timoutNWConnection)){
      if(waitServer(timoutNWConnection)) return(1); //server busy in nwError
      return 0;
    }
  }
  else{
    m_pTcpSocket->disconnectFromHost();
    m_pTcpSocket->connectToHost(ip, port);
    if(m_pTcpSocket->waitForConnected(timoutNWConnection)){
      if(waitServer(timoutNWConnection)) return(1); //server busy in nwError
      return 0;
    }
  }
  return 1; // can't find server
}

// waitServer
int TPhotoTcp::waitServer(int tout)
{
  QEventLoop loop;
  nwError="";
  while(!srvConnect && (tout>0)) {
    msleep(1);
    loop.processEvents();
    tout--;
  }
  if(!tout) { nwError="Can't connect to "+ipAddress+" "+nwError; return 1;}
  return 0;
}

int TPhotoTcp::errorProc(int code)
{
  switch(code){
  case OK_ERR: nwError=""; return(OK_ERR);
  case CANTSTART_ERR: nwError="Can't start external program for get photo on RPI device (raspistill)."; return(CANTSTART_ERR);
  case CANTEND_ERR: nwError="External program for get photo on RPI device (raspistill) can't finished normally."; return(CANTEND_ERR);
  case FILERW_ERR: nwError="File I/O error on RPI device."; return(FILERW_ERR);
  case TIMEOUT_ERR: return(TIMEOUT_ERR);
  default: nwError="Unknown error"; return(UNKNOWN_ERR);
  }
}

//-----------------------------------------------------------------------------
//--- slot_Connected
//-----------------------------------------------------------------------------
void TPhotoTcp::slot_Connected()
{
  qDebug() << "Connected";
  nwError="";
}

//-----------------------------------------------------------------------------
//--- slotError
//-----------------------------------------------------------------------------
void TPhotoTcp::slot_error(QAbstractSocket::SocketError err)
{
  QString strError =
        (err == QAbstractSocket::HostNotFoundError ?
                     "The host was not found." :
                     err == QAbstractSocket::RemoteHostClosedError ?
                     "The remote host is closed." :
                     err == QAbstractSocket::ConnectionRefusedError ?
                     "The connection was refused." :
                     QString(m_pTcpSocket->errorString())
                    );
  //qDebug() << strError;
  nwError=strError;
}

//-----------------------------------------------------------------------------
//--- slot_ReadyRead
//-----------------------------------------------------------------------------
void TPhotoTcp::slot_readyRead()
{
  int index;

  QDataStream in(m_pTcpSocket);
  in.setVersion(QDataStream::Qt_4_4);

  for(;;) {
    if(!m_nNextBlockSize) {
      if(m_pTcpSocket->bytesAvailable() < sizeof(quint32)) break;
      in >> m_nNextBlockSize;
    }
    if(m_pTcpSocket->bytesAvailable() < m_nNextBlockSize) break;
    m_nNextBlockSize = 0;

    QString answer;
    in >> answer>>ph_error;
  //  qDebug() << "Get Request: " << answer <<"error"<<ph_error;

    if(answer == CONNECT_REQUEST){
      map_ConnectedStatus.clear();
      in >> map_ConnectedStatus;
     // qDebug()<<"Get connected status"<<map_ConnectedStatus;
      if(map_ConnectedStatus.value(CONNECT_STATUS)=="BUSY") {
        nwError="Server busy";
        srvConnect=0;
      }
      else {
        nwError="";
        srvConnect=1;
      }
      break;
    }
    if(!List_Requests.contains(answer)) { qDebug()<<"Unrecognize answer "<<answer;break; }// unrecognize answer
    if(Map_TimerRequest.contains(answer)) {
      Map_TimerRequest.value(answer)->stop(); // stop timers if was this answer
    }
    index = Catalogue_Requests.indexOf(answer);
//    qDebug()<<"index of request"<<index;

    switch(index) {
      case 0:{ //PHOTO_REQUEST
      qDebug()<<"get photo answer";
        in >> map_PhotoData;
        progress=1;
        progressVideo=1;
        break;
      }
      case 1:{ //GETDATA_REQUEST
        qDebug()<<"get data";
         in >> map_PhotoData;
         progressGD=1;
        break;
      }
      case 2:{ //STATUS_REQUEST
        in >> map_PhotoStatus;
        progressSts=1;
       // qDebug()<<"Get status of HW";
        break;
      }
      case 3:{ //GETFILES_REQUEST
        in >> map_FilesList;
        progressFL=1;
       // qDebug()<<"Get file list">>map_JPEGFilesList.value(GETFILES_DATA);
        break;
      }
      case 4:{
        progressDF=1;
        break;
      }
      default:{
        break;
      }
    } // end case
    index = List_Requests.indexOf(answer);
    if(index >= 0) List_Requests.removeAt(index);
//    qDebug()<<"listReq"<<List_Requests<<progress;

  } // end for
}

//-----------------------------------------------------------------------------
//--- SlotSendToServer
//-----------------------------------------------------------------------------
void TPhotoTcp::slot_sendToServer(QString request)
{
  qint64 out_byte;
  int interval;

 //if(List_Requests.contains(GETDATA_REQUEST)) return;  // when GETDATA_REQUEST is active, ALL other are disabled
 // if(List_Requests.contains(PHOTO_REQUEST)) return;    // when PHOTO_REQUEST is active, ALL other are disabled
  if(List_Requests.contains(request)) return;          //don't get answer on previous request
  else
    List_Requests.append(request);

 // qDebug()<<"Send request"<<request<<List_Requests;

  QByteArray  arrBlock;
  QDataStream out(&arrBlock, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_4_4);
  out << quint32(0) << request;
  if(request == PHOTO_REQUEST)  out<<map_MakePhotoHeader;
  if(request == GETDATA_REQUEST) out<<map_MakePhotoHeader;
  if(request == DELFILESLIST_REQUEST) out<< map_DelFilesList;
  out.device()->seek(0);
  out << quint32(arrBlock.size() - sizeof(quint32));
  out_byte = m_pTcpSocket->write(arrBlock);
  //set timer on requests
  if(Map_TimerRequest.contains(request)){
      if(request == PHOTO_REQUEST)       interval =10000;
      if(request == GETDATA_REQUEST)     interval =10000;
      if(request == STATUS_REQUEST)      interval =2000;
      if(request == GETDIRLIST_REQUEST)  interval =5000;
      if(request == DELFILESLIST_REQUEST)interval =5000;
      Map_TimerRequest.value(request)->start(interval,this);
    }
}

//------------------------ EVENTS ----------------------------------------------
//-------------------------------------------------------------------------------
//--- timerEvent
//-------------------------------------------------------------------------------
void TPhotoTcp::timerEvent(QTimerEvent *event)
{
  bool sts = false;
  QBasicTimer *timer;
  int index;
  QString text;
  int id = event->timerId();

  foreach(timer, Map_TimerRequest) {
    if(id == timer->timerId()) {
      timer->stop();
      sts = true;
      text=Map_TimerRequest.key(timer);
      break;
    }
  }
  if(sts) {
    index = List_Requests.indexOf(text);
    if(index >= 0) List_Requests.removeAt(index);

    if(text==GETDIRLIST_REQUEST){
      dirListTOut=true;
    }
    else  if(text==PHOTO_REQUEST){
      phTOut=true;
    }
    else  if(text==STATUS_REQUEST){
      getStsTOut=true;
    }
    else  if(text==GETDATA_REQUEST){
      getFileTOut=true;
    }
    else  if(text==DELFILESLIST_REQUEST){
      delFilesTOut=true;
    }
    nwError="Timeout on network request"+text;
  }
  else QObject::timerEvent(event);
}
