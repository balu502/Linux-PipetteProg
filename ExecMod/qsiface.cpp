#include "qsiface.h"
#include <stdio.h>

qsIface::qsIface(QObject *parent) :
    QObject(parent)
{
  //  init();
}

int qsIface::iTest(QString t)
{
    printf("%s\n",t.toAscii().data());
    return 123;
}

void qsIface::conOut(QString t)
{
    fprintf(stderr,"%s",t.toAscii().data());
}

int qsIface::mt_cmd(int n, QString c)
{
	return lw.mt_cmd(n,c.toAscii().data());
}
