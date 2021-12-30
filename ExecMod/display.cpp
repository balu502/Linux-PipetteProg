#include "display.h"

#include <stdio.h>

//#include "lcd.h"

//#include "border.pic"
//#include "fnt8x16.fnt"
//#include "dna.pic"
//---commands----------------
#define COM_SET_COLUMN_ADDRESS		0x15
#define COM_SET_ROW_ADDRESS			0x75

#define COM_WRITE_RAM    			0x5c
#define COM_READ_RAM				0x5d

#define COM_SET_REMAP_AND_DUAL_COM_LINE_MODE	0xa0
#define COM_SET_DISPLAY_START_LINE	0xa1
#define COM_SET_DISPLAY_OFFSET		0xa2
#define COM_SET_DISPLAY_MODE		0xa0		//(a4 to a7)
#define COM_FUNCTION_SELECTION		0xab
#define COM_SET_SLEEP_MODE_ON		0xae
#define COM_SET_SLEEP_MODE_OFF		0xaf
#define COM_SET_PHASE_LENGHT		0xb1
#define COM_SET_DISPLAY_CLOCK		0xb3
#define COM_SET_GPIO				0xb5
#define SET_SECOND_PRE_CHARGE_PERIOD	0xb6
#define SELECT_DEFAULT_LINEAR_GRAYSCALE_TABLE	0xb9
#define SET_PRE_CHARGE_VOLTAGE		0xbb
#define COM_SET_VCOMH				0xbe
#define COM_SET_CONTRAST_CURRENT	0xc1
#define COM_MASTER_CONTRAST_CURRENT_CONTROL	0xc7
#define COM_SET_MULTIPLEX_RATIO		0xca
#define COM_SET_COMMAND_LOCK		0xfd
//---parameters--------------
#define HORIZONTAL_ADDRESS_INCREMENT	0x00
#define DISABLE_COLUMN_ADDRESS_REMAP	0x00
#define ENABLE_NIBBLE_REMAP				0x04
#define SCAN_FROM_COM_N					0x10
#define DISABLE_COM_SPLIT_ODD_EVEN		0x00
#define DISABLE_DUAL_COM_MODE			0x01
#define ENABLE_DUAL_COM_MODE			0x11
#define CLOCK_DIVIDER_FREQ		0x91
#define GPIO0_PIN_HI_Z_INPUT_DISABLED	0x00
#define GPIO1_PIN_HI_Z_INPUT_DISABLED	0x00
#define RATIO_64MUX				0x3f
#define UNLOCK_OLED_DRIVER		0x12
#define LOCK_OLED_DRIVER		0x16
#define EXTERNAL_VDD			0x00
#define INTERNAL_VDD			0x01
#define CONTRAST_CURRENT_NO_CHANGE		0x0f
#define PHASE_1_PERIOD_5_DCLKS	0x02
#define PHASE_2_PERIOD_14_DCLKS	0xe0
#define PRE_CHARGE_VOLTAGE_040_VCC		0x1f
#define VCOMH_086_VCC			0x07

#define DISPLAY_MODE_ENTIRE_OFF	0x04
#define DISPLAY_MODE_ENTIRE_ON	0x05
#define DISPLAY_MODE_NORMAL		0x06
#define DISPLAY_MODE_INVERSE	0x07
//---------------------------

Display::Display(QObject *parent) :
    QObject(parent)
{
    outena_map = 8538;
}
/*
void Display::outBuffer(void)
{
    setAddress(0,0);
    spi2_tx(COM_WRITE_RAM);
    for(int i=0;i<(0x78*0x80);i++) spi2_tx(0x100);
}

void Display::setAddress(int x, int y)
{
  spi2_tx(SET_COL_ADDR);
  spi2_tx(0x100|(x+0x1C));
  spi2_tx(SET_ROW_ADDR);
  spi2_tx(0x100|y);
}*/

int Display::spi2_tx(int data)
{
  *(unsigned long *)(spi2+0x38)=data; // set OLED current level
  while(1) {
    //printf("SPI2 status %x\n",*(unsigned long *)(spi2+0x30));
    if ((*(unsigned long *)(spi2+0x30))&2) break; // wait for the word transmitted
  }
  return 0;
}

bool Display::init(void)
{


    int mfd = open("/dev/mem",O_RDWR);
    if (mfd < 0)
    {
        emit message("Error: mem open");
        return 0;
    }

    kev = epoll_create(5);
    if (kev < 0)
    {
        emit message("Error: epoll_create");
        return 0;
    }

    kfd = open("/dev/input/event0",O_RDWR | O_NONBLOCK);
    if (kfd < 0)
    {
        emit message("Error: input event0 open");
        close(kev);
        return 0;
    }

    event.data.fd = kfd; /* return the fd to us later */
    event.events = EPOLLIN|EPOLLET;
    int ret = epoll_ctl (kev, EPOLL_CTL_ADD, kfd, &event);
    if(ret) emit message ("Error: epoll_ctl");

    padconf = (unsigned char *) mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, 0x48002000);
    if(padconf == MAP_FAILED)
    {
        emit message("Error: mmap padconf");
        return 0;
    }
    gpio4 = (unsigned char *) mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, 0x49054000);
    if(gpio4 == MAP_FAILED)
    {
        emit message("Error: mmap gpio4");
        return 0;
    }

    spi2 = (unsigned char *) mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,MAP_SHARED, mfd, 0x4809A000);
    if(spi2 == MAP_FAILED)
    {
        emit message("Error: mmap spi2");
        return 0;
    }

    cm = (unsigned char *) mmap(NULL, 0x2000, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, 0x48004000);
    if(cm == MAP_FAILED)
    {
        emit message("Error: mmap cm");
        return 0;
    }

    emit message("GPIO ver " + QString::number(*(unsigned long *)gpio4));
    emit message("CM   ver " + QString::number(*(unsigned long *)(cm+0x800))); //revision
    emit message("PADcf4  " + QString::number(*(unsigned long *)(padconf+0x1D4)));
    emit message("PADcf8  " + QString::number(*(unsigned long *)(padconf+0x1D8)));

      //padconf_spi2cs = *(unsigned long *)(padconf+0x1DC);
      //padconf_spi2cs &= ~((0xf<<16)|0xf); // set gpio178 & gpio180 pins mode0 (cs0&cs1)
      //*(unsigned long *)(padconf+0x1DC)=padconf_spi2cs;

    emit message("PADcfC  " + QString::number(*(unsigned long *)(padconf+0x1DC)));

    cm_fclken1_core = *(unsigned long *)(cm+0xA00);
    cm_fclken1_core |= (1<<19); // McSPI2 func clock enable
    *(unsigned long *)(cm+0xA00) = cm_fclken1_core;
    //printf("CORE clock ena %x\n",*(unsigned long *)(cm+0xA00));

    cm_iclken1_core = *(unsigned long *)(cm+0xA10);
    cm_iclken1_core |= (1<<19); // McSPI2 interface clock enable
    *(unsigned long *)(cm+0xA10) = cm_iclken1_core;
    //printf("CORE clock ena %x\n",*(unsigned long *)(cm+0xA10));

    emit message("SPI  ver " + QString::number(*(unsigned long *)spi2));

    emit message("GPIO sysstat " + QString::number(*(unsigned long *)(gpio4+0x14)));
    emit message("GPIO control " + QString::number(*(unsigned long *)(gpio4+0x30)));

    outena_map=*(unsigned long *)(gpio4+0x34);
    outena_map &= ~(1<<(117%32)); // BS0
    outena_map &= ~(1<<(118%32)); // BS1
    outena_map &= ~(1<<(119%32)); // RES#

    *(unsigned long *)(gpio4+0x34)=outena_map;
    emit message("GPIO output ena " + QString::number(*(unsigned long *)(gpio4+0x34)));
    emit message("GPIO output data before " + QString::number(*(unsigned long *)(gpio4+0x3c)));
    // init display
    gpio_data = *(unsigned long *)(gpio4+0x3C);
    gpio_data = (1<<(117%32)); // BS=01,RES#=0 - 3-wire SPI, reset active
    *(unsigned long *)(gpio4+0x3C)=gpio_data;
    usleep(100); // keep RES# active at least 100 us
    gpio_data |= (1<<(119%32));
    *(unsigned long *)(gpio4+0x3C)=gpio_data; // deactivate RES#

    emit message("GPIO output data after " + QString::number(*(unsigned long *)(gpio4+0x3c)));

    *(unsigned long *)(spi2+0x10)=2; // SPI2 softreset
    while(1) if( (*(unsigned long *)(spi2+0x14))&1) break; // wait for reset completed

    *(unsigned long *)(spi2+0x34)=0; // disable ch0
    *(unsigned long *)(spi2+0x1c)=0; // disable IRQs
    *(unsigned long *)(spi2+0x18)=0x1777f; // clear IRQs
    // SPI2 config
    *(unsigned long *)(spi2+0x28)=0; // master, multichan
    *(unsigned long *)(spi2+0x2C)=0x20012450; //CLKG=1, div=4,pol=0,ph=0
    *(unsigned long *)(spi2+0x34)=1; // enable ch0
    // SPI2_0 Tx

    spi2_tx(0xfd);  // set interface unlock
    spi2_tx(0x112);

    spi2_tx(0xae); // sleep display on

    spi2_tx(0xb3);  // display clock freq
    spi2_tx(0x191);

    spi2_tx(0xca);  // set mux ratio
    spi2_tx(0x13f);

    spi2_tx(0xa2);  // set display offset
    spi2_tx(0x100);

    spi2_tx(0xa1);  // set display start line
    spi2_tx(0x100);

    spi2_tx(0xa0);  // set display remap and dual COM
    spi2_tx(0x106);
    spi2_tx(0x111);

    spi2_tx(0xb5);  // set GPIO off
    spi2_tx(0x100);

    spi2_tx(0xab);  // set func (internal VDD)
    spi2_tx(0x101);

    spi2_tx(0xb4);  // enable ext VSL
    spi2_tx(0x1a0);
    spi2_tx(0x1fd);

    spi2_tx(0xc1);  // set OLED current level
    spi2_tx(0x1cf); // current level

    spi2_tx(0xc7);  // set master contrast
    spi2_tx(0x10f);

    spi2_tx(0xB9);  // linear GrayScale Table

    spi2_tx(0xb1);  // set phase len
    spi2_tx(0x1e2);

    spi2_tx(0xd1);  // enh driving capabilities
    spi2_tx(0x182);
    spi2_tx(0x120);

    spi2_tx(0xbb);  // set precharge voltage
    spi2_tx(0x11f);

    spi2_tx(0xb6);  // set 2nd precharge period
    spi2_tx(0x108);

    spi2_tx(0xbe);  // set VCOMH 0.86
    spi2_tx(0x107);

    spi2_tx(0x5C);  // write display RAM
    for(i=0; i<(0x78*0x80); i++) spi2_tx(0x100);
    //spi2_tx(0xa6);  // set DISPLAY normal
    setDisplayMode(DISPLAY_MODE_NORMAL);
    spi2_tx(0xaf); // sleep off

    //pi2_tx(0xa5);  // set entire DISPLAY ON (all pixels ON)
    //usleep(1000000L);
    //spi2_tx(0xa6);  // set DISPLAY normal

    //LcdCLS();
 /*   setPen(0x55);
    fill();
    outBuffer();*/
}

void Display::deinit()
{
    *(unsigned long *)(spi2+0x34)=0; // disable ch0

    close(kev);
    munmap(gpio4,0x1000);
    munmap(spi2,0x1000);
}

int Display::scanButtons(void)
{
    int return_value = -1;
    int len;
    struct input_event keydat;

    len = read(kfd, &keydat, sizeof(keydat));
    if (keydat.type == EV_KEY && keydat.value == 1)
    {
        if (keydat.code == KEY_ENTER) return_value = 0;
        else if (keydat.code == KEY_DOWN) return_value = 1;
        else if (keydat.code == KEY_MENU) return_value = 2;
    }

    return return_value;
}

void Display::outBuffer(void)
{
    int n;
    //CS_on();
    setColumnAddress(28, 91);
    setRowAddress(0, 63);
    writeComm(COM_WRITE_RAM);
    for(n=0; n<UG5664_BUFFER_BYTES; n++) writeData(buffer[n]);
    //CS_off();
}

void Display::setPen(unsigned char value)
{
    pen = value;
}

void Display::fill(void)
{
    int n;
    for(n=0; n<UG5664_BUFFER_BYTES; n++) buffer[n] = pen;
}

void Display::point(int x, int y)
{
    unsigned char mask;

    if(x >= 0 && x < UG5664_WIDTH_BYTES*2
            && y >= 0 && y < UG5664_HEIGHT_BYTES)
    {
        mask = 0xf0 >> 4*(x%2);
        buffer[x/2 + y*UG5664_WIDTH_BYTES] &= mask ^ 0xff;
        buffer[x/2 + y*UG5664_WIDTH_BYTES] |= mask & pen;
    }
}

void Display::rectangle(int x, int y, int width, int height)
{
    unsigned char first_byte, last_byte;
    unsigned char first_mask, last_mask;
    int bytes;
    int row, col;

    if(x%2) first_mask = 0x0f;
    else first_mask = 0xff;


    if((x+width)%2) last_mask = 0xf0;
    else last_mask = 0xff;

    bytes = (x+width+1)/2 - x/2;
    if(bytes == 1) first_mask &= last_mask;

    first_byte = first_mask & pen;
    last_byte = last_mask & pen;

    for(row=y; row<y+height; row++)
    {
        if(x >= 0 && x < UG5664_WIDTH_BYTES*2
                && row >= 0 && row < UG5664_HEIGHT_BYTES)
        {
            buffer[x/2 + row*UG5664_WIDTH_BYTES] &= first_mask^0xff;
            buffer[x/2 + row*UG5664_WIDTH_BYTES] |= first_byte;
        }
        for(col=x/2+1; col<x/2+1+bytes-2; col++)
        {
            if(col >= 0 && col < UG5664_WIDTH_BYTES && row >= 0 && row < UG5664_HEIGHT_BYTES)
                buffer[col + row*UG5664_WIDTH_BYTES] = pen;
        }
        if(bytes>1 && col >= 0 && col < UG5664_WIDTH_BYTES
                && row >= 0 && row < UG5664_HEIGHT_BYTES)
        {
            buffer[col + row*UG5664_WIDTH_BYTES] &= last_mask^0xff;
            buffer[col + row*UG5664_WIDTH_BYTES] |= last_byte;
        }
    }
}

void Display::picture(int x, int y, const unsigned char* data)
{
    drawPicture(x, y, data, (data[1] + 7) / 8);
}
void Display::drawPicture(int x, int y, const unsigned char* data, int sym_data_width)
{
    //unsigned char code = *(data++);
    data++;	//skip symbol code
    int width = *(data++);
    int height = *(data++);
    int row, col;
    int pic_horizontal_bytes = (width + 7) / 8;
    int bit;
    unsigned char mask;
    int buffer_index;

    for(row=y; row<y+height; row++)
    {
        col = x/2;
        mask = 0xf0;
        if(x%2) mask ^=0xff;

        for(bit=0; ;)
        {
            if((((*data)<<(bit%8)) & 0x80)	//pixel is set
                    && col >= 0 && col < UG5664_WIDTH_BYTES
                    && row >= 0 && row < UG5664_HEIGHT_BYTES)
            {
                buffer_index = col + row*UG5664_WIDTH_BYTES;
                buffer[buffer_index] &= mask ^ 0xff;
                buffer[buffer_index] |= mask & pen;
            }
            mask ^=0xff;
            if(mask & 0xf0) col++;
            bit++;
            if(bit >= width) {data += 1 + sym_data_width - pic_horizontal_bytes; break;}
            if(bit%8 == 0) data++;
        }
    }
}

void Display::setFont(const unsigned char* data, int sym_bytes_count, int sym_data_width)
{
    font_ptr = data;
    font_sym_bytes_count = sym_bytes_count;
    font_sym_data_width = sym_data_width;
}

void Display::text(int x, int y, char* string)
{
    char current_char;
    unsigned char* current_sym_ptr;

    while(1)
    {
        current_char = *(string++);
        if(current_char == 0) break;
        if(font_ptr[0] <= current_char)	//font contains symbol for character
        {
            current_sym_ptr = (unsigned char*) font_ptr + (current_char - font_ptr[0]) * font_sym_bytes_count;
            drawPicture(x, y, current_sym_ptr, font_sym_data_width);
            x += current_sym_ptr[1];
        }
    }
}

int Display::textWidth(char* string)
{
    char current_char;
    unsigned char* current_sym_ptr;
    int width = 0;

    while(1)
    {
        current_char = *(string++);
        if(current_char == 0) break;
        if(font_ptr[0] <= current_char)	//font contains symbol for character
        {
            current_sym_ptr = (unsigned char*) font_ptr + (current_char - font_ptr[0]) * font_sym_bytes_count;
            width += current_sym_ptr[1];
        }
    }
    return width;
}

// Display write data
void Display::writeData(int d)
{
    spi2_tx(0x100 | d);
}
// Display write command
void Display::writeComm(int comm)
{
    spi2_tx(comm);
}

//---------------------------
// Command for set display
void Display::setCommandLock(int parameter)
{
    writeComm(COM_SET_COMMAND_LOCK);
    writeData(parameter);
}
void Display::setSleepModeOn(void)
{
    writeComm(COM_SET_SLEEP_MODE_ON);
}
void Display::setSleepModeOff(void)
{
    writeComm(COM_SET_SLEEP_MODE_OFF);
}
void Display::setDisplayClock(int parameter)
{
    writeComm(COM_SET_DISPLAY_CLOCK);
    writeData(parameter);
}
void Display::setMultiplexRatio(int parameter)
{
    writeComm(COM_SET_MULTIPLEX_RATIO);
    writeData(parameter);
}
void Display::setDisplayOffset(int parameter)
{
    writeComm(COM_SET_DISPLAY_OFFSET);
    writeData(parameter);
}
void Display::setDisplayStartLine(int parameter)
{
    writeComm(COM_SET_DISPLAY_START_LINE);
    writeData(parameter);
}
void Display::setRemapAndDualCOMLineMode(int param_a, int param_b)
{
    writeComm(COM_SET_REMAP_AND_DUAL_COM_LINE_MODE);
    writeData(param_a);
    writeData(param_b);
}
void Display::setGPIO(int parameter)
{
    writeComm(COM_SET_GPIO);
    writeData(parameter);
}
void Display::functionSelection(int parameter)
{
    writeComm(COM_FUNCTION_SELECTION);
    writeData(parameter);
}
void Display::EnableExternalVSL(void)
{
    writeComm(0xb4);
    writeData(0xa0);
    writeData(0xfd);
}
void Display::setContrastCurrent(int parameter)
{
    writeComm(COM_SET_CONTRAST_CURRENT);
    writeData(parameter);
}
void Display::masterContrastCurrentControl(int parameter)
{
    writeComm(COM_MASTER_CONTRAST_CURRENT_CONTROL);
    writeData(parameter);
}
void Display::selectDefaultLinearGrayScaleTable(void)
{
    writeComm(SELECT_DEFAULT_LINEAR_GRAYSCALE_TABLE);
}
void Display::setPhaseLength(int parameter)
{
    writeComm(COM_SET_PHASE_LENGHT);
    writeData(parameter);
}
void Display::enhanceDrivingSchemeCapability(void)
{
    writeComm(0xd1);
    writeData(0x82);
    writeData(0x20);
}
void Display::setPreChargeVoltage(int parameter)
{
    writeComm(SET_PRE_CHARGE_VOLTAGE);
    writeData(parameter);
}
void Display::setSecondPreChargePeriod(int parameter)
{
    writeComm(SET_SECOND_PRE_CHARGE_PERIOD);
    writeData(parameter);
}
void Display::setVCOMH(int parameter)
{
    writeComm(COM_SET_VCOMH);
    writeData(parameter);
}
void Display::setDisplayMode(int parameter)
{
    writeComm(COM_SET_DISPLAY_MODE | parameter);
}
void Display::setColumnAddress(int start, int end)
{
    writeComm(COM_SET_COLUMN_ADDRESS);
    writeData(start);
    writeData(end);
}
void Display::setRowAddress(int start, int end)
{
    writeComm(COM_SET_ROW_ADDRESS);
    writeData(start);
    writeData(end);
}

/*

// Display Progress Bar without border in position x0, y0 (x0%4==0)
// Border display as Bmp. With 122 pix, height 2 pix
void Display::ProgressBar(char x0,char y0,unsigned int procent)
{
  int i,j,colons,pic_dat,wr_dat,x,b;
  char w=3;
  x = x0>>2;

  colons=procent*118/100;

  for(i=0;i<2;i++){
    setAddress(x, y0+i);
    spi2_tx(COM_WRITE_RAM);
    for(j=0;j<colons;j++){
      spi2_tx(0x1ff);
    }
  }
}

// Display Bmp from LcdIcon program in position x0, y0 (x0%4==0)
void Display::LcdBmp(char x0, char y0, const char *pic)
{
  int x,b,i,j,y,n=2,w,h;
  int pic_dat,wr_dat;

  w = pic[0]>>3 ;
  h = pic[1];

  x = x0>>2;
  y = y0;

  for(i=0;i<h;i++){
    setAddress(x, y);
    spi2_tx(COM_WRITE_RAM);
    y++;
    for(j=0;j<w;j++){
      pic_dat = pic[n++];
      for(b=0; b<4; b++){
        if(pic_dat & 0x80) wr_dat = 0xf0;
        else wr_dat = 0x00;
        if(pic_dat & 0x40) wr_dat |= 0x0f;
        spi2_tx(0x100|wr_dat);
        pic_dat = pic_dat<<2;
      }
    }
  }
}

// display font chars 8X16 pix
int Display::LcdSym8x16(int x, int y, const sUnit8x16* sym)
{
    char* pnt;
    pnt = (char*)sym;
    LcdBmp(x,y,pnt+1);
    return pnt[1];
}
// display string of chars x%4==0
void Display::LcdStr8x16(int x,int y, char *s)
{
    int cx;
    char ch, *pnt;
    cx = x;
    pnt = s;
    while((ch = *pnt++) != '\0')
    {
        LcdSym8x16(cx,y,&fnt8x16[ch]);
        cx +=fnt8x16[ch].sH.cxPix;
    }
}

void Display::LcdCLS(void)
{
  int i;
  setAddress(0,0);
  spi2_tx(COM_WRITE_RAM);
  for (i=0;i<(0x78*0x80);i++) spi2_tx(0x100);
}
*/
