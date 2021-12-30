/*!
\example main.cpp
*/
#include <QtCore/QCoreApplication>

#include <QTime>
#include <photohttp.h>
#include <phototcp.h>
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    TPhotoHttp *http;
    http=new TPhotoHttp("192.168.31.76");
    QStringList list;
    TPhotoTcp *photo;
    photo=new TPhotoTcp();
    while(1){
      int ret;
      qDebug()<<"link"<<(ret=photo->link("192.168.31.76",9002));
      if(ret) { qDebug()<<photo->getError(); break;}
      qDebug()<<"getList"<<(ret=photo->getListJPEGFiles(list));
      if(ret) { qDebug()<<photo->getError(); break;}
      qDebug()<<"list files JPEG"<<list;
      QStringList l; l<<"0.jpg";
      //qDebug()<<"del"<<(ret=photo->delFiles(l));
      if(ret) { qDebug()<<photo->getError(); break;}
      qDebug()<<"Make instante photo beg"<<QTime::currentTime().toString();
      qDebug()<<"Make iphoto1"<<(ret= photo->makeInstantPhoto("0.jpg"));
      if(ret) { qDebug()<<photo->getError(); break;}
      qDebug()<<"Make iphoto1"<<(ret=photo->makeInstantPhoto("1.jpg"));
      if(ret) { qDebug()<<photo->getError(); break;}
     // while(photo->getPhotoStatus());
      qDebug()<<"Make photo end"<<QTime::currentTime().toString();
      qDebug()<<"Save photo file begin";
      qDebug()<<QTime::currentTime().toString();
      qDebug()<<photo->getJPEGFile("0.jpg","myf.jpg");
      qDebug()<<QTime::currentTime().toString();
      qDebug()<<"Save photo file finish";
      //sleep(2);
      //return(0);
// make photo

      qDebug()<<"HTTP";
      qDebug()<<"Make photo";
      qDebug()<<QTime::currentTime().toString();
      if(http->makePhoto("kak.jpg")) {qDebug()<<"Error! "<<http->getError(); break;}
      qDebug()<<QTime::currentTime().toString();
      qDebug()<<"Make photo compleate.";
      qDebug();

// get list of datajpg
      if(http->getListJPEGFiles(list)) {qDebug()<<"Error! "<<http->getError(); break;}
      qDebug()<<"Get JPEG files list";
      for(int i=0;i<list.size();i++){
        qDebug()<<list.at(i);
      }
      qDebug();

      list.clear();

// get file imgtmp.jpg from dir datajpg and save as tmp.jpg
      qDebug()<<"Get JPEG file";
      qDebug()<<QTime::currentTime().toString();
      http->setTimeout(20000);
      if(http->getJPEGFile("kak.jpg","tmp.jpg")) {qDebug()<<"Get JPEG file error! "<<http->getError(); break;}
      qDebug()<<QTime::currentTime().toString();
      qDebug()<<"Get JPEG file compleate.";
      qDebug();

      qDebug()<<"Delete JPEG file";
     // if(http->removeJPEGFile("imgtmp.jpg")) {qDebug()<<"Delete JPEG file error! "<<http->getStrError(); break;}
      qDebug()<<"Delete JPEG file compleate.";

      qDebug()<<"Delete all JPEG file";
      if(http->removeAllJPEGFiles()) {qDebug()<<"Delete JPEG file error! "<<http->getError(); break;}
      qDebug()<<"Delete all JPEG file compleate.";
      break;
    }
    qDebug()<<"End!";
    delete http;
    return a.exec();
}
