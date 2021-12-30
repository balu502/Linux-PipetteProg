#ifndef QSIFACE_H
#define QSIFACE_H

#include <QObject>
#include <QString>
#include "/home/dnadev/sam/prog/qt/lwlib2/myclient.h"
#include "/home/dnadev/sam/prog/qt/lwlib2/lw.h"

class qsIface : public QObject
{
    Q_OBJECT
public:
    explicit qsIface(QObject *parent = 0);
    int doQs(char* scr);
private:
    Lw lw;

signals:

public slots:
    int iTest(QString t);
    void conOut(QString t);
    int loop() {return lw.lw_loop(); }
    void init() {lw.lw_init(); }
    int move() { return lw.lw_move(); }
    int pos(int n, int v) { return lw.lw_pos(n, v); }
    int vel(int n, int v) { return lw.lw_vel(n, v); }

    int acc(int n, int v) { return lw.lw_acc(n, v); }
    int flt(int n, int v) { return lw.lw_flt(n, v); }
    int force(int n, int v) { return lw.lw_force(n, v); }
    int sts() { return lw.lw_sts(); }
    int State() { return lw.lw_State(); }
    int det() { return lw.lw_det(); }
    int wait() { return lw.lw_wait(); }
    int waitBuf() { return lw.lw_waitBuf(); }
    int setVal(int n, int v) { return lw.lw_setVal(n, v); }
    int setReq(int v[4]) { return lw.lw_setReq(v); }
    int setFilled() { return lw.lw_setFilled(); }
    int isFilled() { return lw.lw_isFilled(); }
    int isPrep() { return lw.lw_isPrep(); }
    int prepTo() { return lw.lw_prepTo(); }
    int runTo() { return lw.lw_runToP(); }
//    int homoRun(int e) { return lw.lw_homoRun(e); }
    int homoStart(int e, int w=1) { return lw.lw_homoStart(e,w); }
    int homoStop() { return lw.lw_homoStop(); }
    int delay(int e) { return lw.lw_delay(e); }

    void mt_init(int n) { lw.mt_init(n); }
//    int stop();
    int mt_cmd(int n, QString c);
    QString mt_com(int n, QString c){ return lw.mt_com(n,c.toAscii().data());}
    int mt_move(int n, int p) { return lw.mt_move(n,p); }
    int mt_dcm(int n, int p) { return lw.mt_dcm(n,p); }
    int mt_prep(int n, int p) { return lw.mt_prep(n,p); }
    int mt_run(int n) { return lw.mt_run(n); }
    int mt_vel(int n, int v) { return lw.mt_vel(n, v); }
    int mt_acc(int n, int v) { return lw.mt_acc(n, v); }
    int mt_cur(int n, int v) { return lw.mt_cur(n, v); }
    int mt_flt(int n, int v) { return lw.mt_flt(n, v); }
    int mt_sts(int n) { return lw.mt_sts(n); }
//    int mt_isp(int n) { return lw.mt_isp(n); }
    int mt_off(int n) { return lw.mt_off(n); }
    int mt_sleep(int t) { return lw.mt_sleep(t); }

    QString im_pos() {  return lw.im_pos(); }
    QString im_bar() {   return lw.im_bar();}
};

#endif // QSIFACE_H
