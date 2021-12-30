//define IO port for LCD operation UG5664 256 x 64
#ifndef LCD_H
#define LCD_H

//---commands----------------
#define COM_WRITE_RAM    0x5C
#define SET_COL_ADDR 0x15
#define SET_ROW_ADDR 0x75

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

#define BYTE unsigned char

//for bmp and fonts
typedef struct               /* header */
   {         
   char ccChar;              /* optional char */
   char cxPix;
   char cyPix;
   } sHeader;

typedef struct               /* one entry */
   {
   sHeader sH;               /* header */
   BYTE b[16];               /* Data */
   } sUnit8x16;
   
typedef struct               /* one entry */
   {
   sHeader sH;               /* header */
   BYTE b[32];               /* Data */
   } sUnit16x16;
typedef struct               /* one entry */
   {
   sHeader sH;               /* header */
   BYTE b[48];                /* Data */
   } sUnit16x24;

// controller functions
void writeData(int data);
void writeComm(int comm);
void reset(void);

// UG5664 functions
void setRemapAndDualCOMLineMode(int param_a, int param_b);
void setDisplayStartLine(int parameter);
void setDisplayOffset(int parameter);
void functionSelection(int parameter);
void setSleepModeOn(void);
void setSleepModeOff(void);
void setDisplayClock(int parameter);
void setGPIO(int parameter);
void setMultiplexRatio(int parameter);
void setCommandLock(int parameter);
void EnableExternalVSL(void);
void setContrastCurrent(int parameter);
void masterContrastCurrentControl(int parameter);
void selectDefaultLinearGrayScaleTable(void);
void setPhaseLength(int parameter);
void enhanceDrivingSchemeCapability(void);
void setPreChargeVoltage(int parameter);
void setSecondPreChargePeriod(int parameter);
void setVCOMH(int parameter);
void UG5664_Init(void);
void UG5664_Cls(void);

// common graph function
void InitLcd(void );
void LcdClear(void );
void LcdSetBrightnessHight(void);
void LcdSetBrightnessLow(void);
void LcdStr8x16(int,int,char *);
int LcdSym8x16(int,int,const sUnit8x16* );
void LcdStr16x16(int,int,char *);
int LcdSym16x16(int,int,const sUnit16x16* );
void LcdStr16x24(int,int,char *);
int LcdSym16x24(int,int,const sUnit16x24* );
void LcdBmp(char ,char, const char[] );
void ProgressBar(char x0,char y0,unsigned int procent);

#endif
