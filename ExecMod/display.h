#ifndef DISPLAY_H
#define DISPLAY_H

#include <QObject>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <stddef.h>
#include <linux/input.h>

#define UG5664_WIDTH_BYTES		128
#define UG5664_HEIGHT_BYTES		64
#define UG5664_BUFFER_BYTES		UG5664_WIDTH_BYTES*UG5664_HEIGHT_BYTES

class Display : public QObject
{
    Q_OBJECT
public:
    explicit Display(QObject *parent = 0);
    bool init(void);
    void deinit(void);

    int scanButtons(void);

    void outBuffer(void);
    void setPen(unsigned char value);
    void fill(void);
    void point(int x, int y);
    void rectangle(int x, int y, int width, int height);
    void picture(int x, int y, const unsigned char* data);
    void setFont(const unsigned char* data, int sym_bytes_count, int sym_data_width);
    void text(int x, int y, char* string);
    int textWidth(char* string);

signals:
    void message(QString);
private:
    int spi2_tx(int data);

    struct epoll_event event,evs[10];
    unsigned char *cm,*padconf;

    int ver,i;
    unsigned long outena_map,data_out1,data_out2, cm_fclken1_core,cm_iclken1_core;
    unsigned long padconf_spi2cs,gpio_data;


    int kev;
    int kfd;
    unsigned char *gpio4;
    unsigned char *spi2;

    unsigned char buffer[UG5664_BUFFER_BYTES];
    unsigned char pen;
    const unsigned char* font_ptr;
    int font_sym_bytes_count;
    int font_sym_data_width;

    void writeData(int d);
    void writeComm(int comm);
    //---------------------------
    // Command for set display
    void setCommandLock(int parameter);
    void setSleepModeOn(void);
    void setSleepModeOff(void);
    void setDisplayClock(int parameter);
    void setMultiplexRatio(int parameter);
    void setDisplayOffset(int parameter);
    void setDisplayStartLine(int parameter);
    void setRemapAndDualCOMLineMode(int param_a, int param_b);
    void setGPIO(int parameter);
    void functionSelection(int parameter);
    void EnableExternalVSL(void);
    void setContrastCurrent(int parameter);
    void masterContrastCurrentControl(int parameter);
    void selectDefaultLinearGrayScaleTable(void);
    void setPhaseLength(int parameter);
    void enhanceDrivingSchemeCapability(void);
    void setPreChargeVoltage(int parameter);
    void setSecondPreChargePeriod(int parameter);
    void setVCOMH(int parameter);
    void setDisplayMode(int parameter);
    void setColumnAddress(int start, int end);
    void setRowAddress(int start, int end);

    void drawPicture(int x, int y, const unsigned char* data, int sym_data_width);
};

#endif // DISPLAY_H
