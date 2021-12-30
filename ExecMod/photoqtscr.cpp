#include "photoqtscr.h"

TPhotoQtScr::TPhotoQtScr(QObject *parent) :
    QObject(parent)
{
    photo=new TPhotoTcp();
    IP="";
    lastSaveFile="";
}

TPhotoQtScr::~TPhotoQtScr()
{
  if(photo) delete photo;
}

/*!
   \param[in] ip  IP4 адрес сервера в формате xx.xx.xx.xx
   \return 0 - Ok, 1-can't find server, 2-server IP don't set
*/

int TPhotoQtScr::setLink(QString ip,int port)
{
  if(ip.isEmpty()){
    if(IP.isEmpty()) { return 2;}
    ip=IP;
  }
  if((port<9002)||(port>9999)) port=9002;
  return(photo->link(ip,port));
}

/*!
   \return QString camera error
*/
QString TPhotoQtScr::getCameraError()
{
  return photo->getError();
}

/*!
  Возвращает статус получения изображения на сервере.
  \return Код возврата. 0 - Изображение получено. 1 - программа получения изображения запускается.
  2 - программа получения изображения запущена. 4 - таймаут сетевой операции.
  В случае ошибки, описание ошибки можно посмотреть, вызвав метод
  getCameraError(), возвращающий QString.
*/
int TPhotoQtScr::getCameraStatus()
{
  return photo->getStatus();
}

/*!
   \param[in] phName  name of photo file
   \param[in] typePhoto  0 - instant (default) 1 - compleat
   \return 0 - Ok, 1 - программа получения изображения запускается.
   2 - программа получения изображения запущена. 3 - remote server file inp/out error. 4 - таймаут сетевой операции.
    В случае ошибки, описание можно посмотреть, вызвав метод
    getCameraError(), возвращающий QString.
*/
int TPhotoQtScr::makePhoto(QString phName,int typePhoto)
{
   int ret=0;
   if(phName.isEmpty()) phName=QString("%1.jpg").arg(time(NULL));
   lastSaveFile=phName;
   switch(typePhoto){
   case 1:
       ret=photo->makePhoto(phName);
     break;
   default:
     ret=photo->makeInstantPhoto(phName);
   }
   return ret;
}

/*!
   \param[in] videoName  name of video file
   \param[in] timeDuration  duration of video in ms
   \return 0 - Ok,  1 - программа получения изображения запускается.
   2 - программа получения изображения запущена. 3 - remote server file inp/out error. 4 - таймаут сетевой операции.
    В случае ошибки, описание можно посмотреть, вызвав метод
    getCameraError(), возвращающий QString.
*/
int TPhotoQtScr::makeVideo(QString videoName,int timeDuration)
{
   int ret=0;
   if(videoName.isEmpty()) videoName=QString("%1.avi").arg(time(NULL));
   lastSaveFile=videoName;
   if(!timeDuration) timeDuration=5000;
   if(timeDuration>60000) timeDuration=60000;
   ret=photo->makeVideo(videoName,timeDuration);
   return ret;
}

/*!
   \param[in] getMediaFileName  name of video file on sarver for save in locale catalogue.
   \param[in] saveMediaFileName  name of video file for save on locale computer. If name don't set
   save in format "time.media"
   if both parameters don't set save last produce media file with name "time.media"
   \return 0 - Ok, 1 - программа получения изображения запускается.
   2 - программа получения изображения запущена. 3 - remote server file inp/out error. 4 - таймаут сетевой операции.
   5 - locale device file inp/out error.
    В случае ошибки, описание можно посмотреть, вызвав метод
    getCameraError(), возвращающий QString.
*/
int TPhotoQtScr::getMediaFile(QString getMediaFileName,QString saveMediaFileName)
{
  if(saveMediaFileName.isEmpty()) saveMediaFileName=QString("%1.media").arg(time(NULL));
  if(getMediaFileName.isEmpty()) getMediaFileName=lastSaveFile;
  return photo->getFile(getMediaFileName,saveMediaFileName);
}

/*!
Get list of file in catalogue /var/www/html/ocr/datajpg on server.

  \return string with name of files parted by spaces
*/
QString TPhotoQtScr::getListFiles(void)
{
  QStringList strList;
  photo->getListFiles(strList);
  return (strList.join(" "));
}

/*!
Удаление заданных файлов в каталоге /var/www/html/ocr/datajpg на сервере.

 \param[in] list QString string with files name for remove in cataloge /var/www/html/ocr/datajpg on server. terminated by space.

 \return Код возврата. 0 если нет ошибки.3 - remote server file inp/out error. 4 - таймаут сетевой операции.
 В случае ошибки, описание можно посмотреть, вызвав метод
 getCameraError(), возвращающий QString.
*/
int TPhotoQtScr::removeFiles(QString list)
{
  QStringList strList=list.simplified().split(" ");
  return photo->removeFiles(strList);
}
