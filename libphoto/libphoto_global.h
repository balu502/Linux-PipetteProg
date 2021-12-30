/*!
  \file libphoto_global.h
  \mainpage Библиотека libphoto для создания изображений на сервере RPI.
  Реализовано два класса TPhotoHttp и TPhotoTcp.
  TPhotoHttp для доступа к изображениям через http протокол.
  TPhotoTcp для доступа к изображениям через tcp протокол.
  Функцонал классов примерно одинаков, и позволяет создать изображение на сервере, сохранить
  изображение на локальном компьютере, получить список изображений,
  удалить файл с изображением на сервере. В классе TPhotoTcp осуществлено получение изображения
  паралельно выполнению программы на локальной машине. В классе TPhotoHttp, при вызове функции
  получения изображения, продолжение выполнение программы на локальном компьютере происходит по завершению
  формирования изображения.
*/
/*!
  \file
  \brief Подключение глобального заголовка и формирование определения класса для экспорта в библиотеку.

*/
// doxygen https://habr.com/ru/post/252101/

#ifndef HIST_ANALYSE_GLOBAL_H
#define HIST_ANALYSE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LIBPHOTO_LIBRARY)
/// @brief Определение заголовка для использования библиотеки
#  define LIBPHOTOSHARED_EXPORT Q_DECL_EXPORT
#else
/// @brief Определение заголовка для использования библиотеки
#  define LIBPHOTOSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // HIST_ANALYSE_GLOBAL_H
