/*!
  \file request_photo.h
  \brief Файл с определением сетевых запросов используемых классом TPhotoTcp

  Данный файл содержит в себе определения
  сетевых запросов используемых классом TPhotoTcp.
*/
#ifndef REQUEST_PHOTO_H
#define REQUEST_PHOTO_H

//........................MEASURE_REQUEST..................................................
/// @brief Запрос на получение изображения.
#define PHOTO_REQUEST     "PHOTO"
/// @brief Поле со способом получения изображения.
#define PHOTO_TYPE        "typephpto"
/// @brief Поле с именем файла изображения.
#define PHOTO_FNAME       "jpegfilename"
/// @brief Возврат в вызывающую программу без ожидания получения изображения.
#define PHOTO_TYPE_I      "photoinstance"
/// @brief Возврат в вызывающую программу с ожиданием получения изображения.
#define PHOTO_TYPE_WF     "photowaitfinish"
/// @brief Возврат в вызывающую программу с ожиданием получения изображения. И получение изображения на локальном компьютере.
#define PHOTO_TYPE_WFD    "photowfgetdata"
/// @brief Начать видеосъемку
#define PHOTO_TYPE_V      "videoinstance"
/// @brief Время, в течении которого осуществляется видеосъемка.
#define VIDEO_TIME        "videotime"

//........................GETDATA_REQUEST..................................................
/// @brief Запрос на считывания файла изображения.
#define GETDATA_REQUEST     "GETDATA"
/// @brief Поле с содержимым файла.
#define GETDATA_DATA        "getdatadate"

//............................STATUS_REQUEST...................................................
/// @brief Запрос на получение статуса выполнения программы по получению изображения.
#define STATUS_REQUEST       "GETSTATUS"
/// @brief Поле со статусом изображения
#define STATUS_PROGRESS      "getprogress"

//............................GETDIRSLIST_REQUEST...............................................
/// @brief Запрос на получение содержимого каталога с изображениями.
#define GETDIRLIST_REQUEST  "GETDIRLIST"
/// @brief Поле со списком файлов.
#define GETDIRLIST_DATA      "getdirlistdata"

//............................DELFILELIST...................................................
/// @brief Запрос на удаление файлов.
#define DELFILESLIST_REQUEST "DELFILELIST"

//............................CONNECT_REQUEST...................................................
/// @brief Запрос на установление соединения.
#define CONNECT_REQUEST       "ISCONNECTED"
/// @brief Поле статуса соединения Ready/Busy.
#define CONNECT_STATUS        "status" // can be READY or BUSY

/// @brief Лист всех запросов. Последний должен быть запрос на установление соединения.
#define CATALOGUE   PHOTO_REQUEST << GETDATA_REQUEST << STATUS_REQUEST << GETDIRLIST_REQUEST << DELFILESLIST_REQUEST << CONNECT_REQUEST
#endif // REQUEST_PHOTO_H
