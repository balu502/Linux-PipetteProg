#include "userinterface.h"

#include <QTextCodec>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/epoll.h>

#include "lcd.h"
#include "dna.pic"

#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <rpc/rpc.h>

const unsigned char picture[] =
{
    #include "logo_dna_tech_rus.pic"
};
#define sUnit	const unsigned char
#include "fnt6x8.fnt"
#include "tnr.fnt"
#include "tnr28.fnt"
#include "dt_stream_buttons.pic"

const unsigned int button_sheme[10][3] =
{
    {0, 0,  0},  // intro
    {4, 10, 6},  // idle
    {1, 2,  3},  // program list
    {7, 8,  9},  // execute info
    {0, 0,  3},  // .. message
    {5, 0,  3},  // abort confirm
    {0, 0,  0},  // screensaver
    {1, 2,  3},  // tool menu list
    {0, 0,  0},  // short message
    {1, 2,  3}   // options list
};

// definitions from ProgramExecuteThread
#define DBS_STATUS_STOPPED	0
#define DBS_STATUS_RUNNING	1
#define DBS_STATUS_PAUSE	2

#define PROG_LIST_LINES_SPACE           13
#define PROG_LIST_VERTICAL_PIXEL_COUNT  64
#define PROG_LIST_CURSOR_X              16
#define PROG_LIST_CURSOR_WIDTH          104

#define BUTTON_COLOR_BLACK              1
#define BUTTON_COLOR_WHITE              8
#define BUTTON_COLOR_WHITE_DISABLED     2

UserInterface::UserInterface(QObject *parent) :
    QObject(parent)
{
    status = DBS_STATUS_STOPPED;
    //prev_status = DBS_STATUS_RUNNING;
    execution_progress = 0;

    button_color[0] = BUTTON_COLOR_BLACK;
    button_color[1] = BUTTON_COLOR_BLACK;
    button_color[2] = BUTTON_COLOR_BLACK;
    screen = INTRO;
    canRefresh=1;
    runSelectProg=false;
   
    connect(&display, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
}

UserInterface::~UserInterface(void)
{
}

bool UserInterface::init(QString scrDir, QString tstDir, QStringList scrExMask)
{

    selected_program_index = -1;
	scriptDir=scrDir;
    testsDir=tstDir;
	scriptExMask=scrExMask;
    QDir dir(scriptDir);
    scriptFileList=dir.entryList(scriptExMask,QDir::Files,QDir::Name);
//  tools menu initialisations
    toolsMenuList <<"Light_On"<<"Light_Off"<<"UVlamp_On"<<"UVlamp_Off"<<"Exit";
    optionsList<<"Cancel";
	selected_tools_index=0;	
    selected_options_index=0;
    statusExecute=0; // script running default value
    if(!display.init()) return false;
    return 1;
}

void UserInterface::deinit(void)
{
    display.deinit();
}
void UserInterface::slotReInitDisplay(void)
{
  qDebug()<<"Display extra initialisation procedure.";
  display.deinit();
  display.init();
}

void UserInterface::printStatus(QString string)
{
    display.setPen(0x00);
    display.fill();
    display.setFont(suFont_OEM_6x8[0], 3+8, 1);
    display.setPen(0xff);
    display.text(0, 0, string.toAscii().data());
    display.outBuffer();
}

// prepare to show message
void UserInterface::showMessage(QString header, QString text)
{
    message_screen_header = header;
    message_screen_text = text;
    screen = MESSAGE;
}

// prepare to show short message
void UserInterface::showShortMessage(QString msg) // show message without btns
{
    shortMsg=msg;
    screen = SHORTMESSAGE;
}

// prepare to set options list from uithread.cpp by function slotSetOptions
void UserInterface::setOptionsList(QString str)
{
    optionsList=str.split(",");
    screen=OPTIONS_LIST;
    disableButtons();
    gotoOptionsList();
}

// scan buttons
void UserInterface::scanButtons(void)
{
    button = display.scanButtons();
    if (button >= 0)
    {
        if(button_enabled[button]) button_color[button] = 0x0f;
    }
}

// display current state
void UserInterface::displayUpdate(void)
{
    static int screensaver_enable_counter = 0,screensaverAM_enable_counter = 0;
    if(canRefresh==0) return;
    if(screen == IDLE)
    {
        if(screensaver_enable_counter++ > SCREENSAVER_ENABLE_TIME) // ~20 s
        {
            screensaver_enable_counter = 0;
            REAL_CONSTANT = -0.4;
            IMG_CONSTANT = +0.6;
            screen = SCREENSAVER;
        }
    }
    else screensaver_enable_counter = 0;
    if(screen==MESSAGE)
    {
        if(screensaverAM_enable_counter++ > SCREENSAVER_ENABLE_TIME)
        {
            screensaverAM_enable_counter = 0;
            REAL_CONSTANT = -0.4;
            IMG_CONSTANT = +0.6;
            screen = SCREENSAVERAM;
        }
    }
    else screensaverAM_enable_counter = 0;

    if(status != DBS_STATUS_STOPPED & screen != OPTIONS_LIST
    && screen != ABORT_CONFIRM) screen = EXECUTE_INFO;

    if(screen == INTRO) intro();
    else if(screen == IDLE) idle();
    else if(screen == PROG_LIST) programList();
    else if(screen == TOOLS_LIST) toolsList();
    else if(screen == OPTIONS_LIST) optionList(); //show options list in process run of script
    else if(screen == EXECUTE_INFO) executeInfo(); // run script state
    else if(screen == MESSAGE) messageScreen();
    else if(screen == SHORTMESSAGE) shortMessageScreen();
    else if(screen == ABORT_CONFIRM) abortConfirmScreen();
    else if(screen == SCREENSAVER) screensaver();
    else if(screen == SCREENSAVERAM) screensaverAM(); //screensaver after measure

    display.outBuffer();
}

// first time logo print on the screen
void UserInterface::intro(void)
{
    static int intro_cycle = 0;

    int fill_color, logo_color;

    if(intro_cycle < 16*4)
    {
        fill_color = intro_cycle/4 | (intro_cycle/4 << 4);
        logo_color = fill_color;
    }
    else if(intro_cycle < 32*4)
    {
        fill_color = 0xff;
        logo_color = 31 - intro_cycle/4; logo_color |= logo_color << 4;
    }
    else if(intro_cycle > 400 - 15*4)
    {
        fill_color = 100 - intro_cycle/4; fill_color |= fill_color << 4;
        logo_color = 0;
    }
    else
    {
        fill_color = 0xff;
        logo_color = 0;
    }

    display.setPen(fill_color);
    display.fill();

    display.setPen(logo_color);
    display.picture(7, 5, (const unsigned char *)picture);

    intro_cycle++;
    if(intro_cycle > 400 || button >= 0)
    {
        disableButtons();
        intro_cycle = 0;
        initMovingPoint(&sel_prog_pos, 255, 30, 10);
        screen = IDLE;
    }
}

// idle state. Get state of scripts list and tools buttons. On choice Run script set runSelectProg=true
void UserInterface::idle(void)
{
    movePoint(&sel_prog_pos);
    display.setPen(0x00);
    display.fill();

    display.setPen(0xff);

    display.setFont(suFont_Font_View_Times_New_Roman_Height_26[0],
                    sizeof(suFont_Font_View_Times_New_Roman_Height_26[0]), 4);
    display.text(0, -5, "DTstream");

    drawSN();
    drawIP();
    drawTime();
    drawSelectedProgram();

    button_enabled[0] = 1;
    button_enabled[1] = 1;
    if(selected_program_index >= 0) button_enabled[2] = 1;
    else button_enabled[2] = 0;
    drawButtons(screen);

    if(button == 0 && button_enabled[0])
    {
        disableButtons();
        gotoProgramList();
    }
	if(button == 1 && button_enabled[1])
    {
        disableButtons();
        gotoTestsList();
    }
    else if(button == 2 && button_enabled[2])
    {
       disableButtons();
       runProgramName=scriptDir+scriptFileList.at(selected_program_index);
       current_program_name=scriptFileList.at(selected_program_index);
       int indx=current_program_name.lastIndexOf('.');
       if(indx>0) current_program_name=current_program_name.left(indx);
       runSelectProg=true;
       setTerminateStatus(false);
    }

}

// print list of script program on display. Choice runing script
void UserInterface::programList(void)
{
    movePoint(&cursor_pos);
    movePoint(&prog_list_pos);

    display.setPen(0x00);
    display.fill();

    drawProgramBox();
    drawCursor();
    drawProgramList();

    if(selected_program_index > 0) button_enabled[0] = 1;
    else button_enabled[0] = 0;

    if(selected_program_index< scriptFileList.count()-1) button_enabled[1] = 1;
    else 
      button_enabled[1] = 0;
    button_enabled[2] = 1;
    drawButtons(screen);

    if(button == 0 && button_enabled[0])
    {
        disableButtons();
        selected_program_index--;
        initCursorMove();
    }
    else if(button == 1 && button_enabled[1])
    {
        disableButtons();
        selected_program_index++;
        initCursorMove();
    }
    else if(button == 2 && button_enabled[2])
    {
        disableButtons();
        QString name;
        display.setFont(suFont_Font_Times_New_Roman_Height_15[0],
                                sizeof(suFont_Font_Times_New_Roman_Height_15[0]), 2);
        if(selected_program_index >= 0) name = scriptFileList.at(selected_program_index);
        else name = "";
        int text_width = display.textWidth(name.toAscii().data());
        initMovingPoint(&sel_prog_pos, cursor_pos.cur_x + 2, cursor_pos.cur_y - 2);
        initPointMove(&sel_prog_pos, 255 - text_width, 30, 10);
        screen = IDLE;
    }
}

// print tools list on display and choice tool
void UserInterface::toolsList(void)
{
    movePoint(&tools_cursor_pos);
    movePoint(&tools_list_pos);

    display.setPen(0x00);
    display.fill();

    drawProgramBox();
    drawToolsCursor();
    drawToolsList();

    drawTime();
    //buttons
    if(selected_tools_index > 0) button_enabled[0] = 1;
    else button_enabled[0] = 0;
    if(toolsMenuList.count()
            && selected_tools_index < toolsMenuList.count() - 1) button_enabled[1] = 1;
    else button_enabled[1] = 0;
    button_enabled[2] = 1;
    drawButtons(screen);

    if(button == 0 && button_enabled[0])
    {
        disableButtons();
        selected_tools_index--;
        initToolsCursorMove();
    }
    else if(button == 1 && button_enabled[1])
    {
        disableButtons();
        selected_tools_index++;
        initToolsCursorMove();
    }
    else if(button == 2 && button_enabled[2])
    {
        disableButtons();
        QString name;
        if(selected_tools_index >= 0) name = toolsMenuList.at(selected_tools_index);
        display.setFont(suFont_Font_Times_New_Roman_Height_15[0],
                                sizeof(suFont_Font_Times_New_Roman_Height_15[0]), 2);
        int text_width = display.textWidth(name.toAscii().data());
        initMovingPoint(&sel_tools_pos, tools_cursor_pos.cur_x + 2, tools_cursor_pos.cur_y - 2);
        initPointMove(&sel_tools_pos, 255 - text_width, 30, 10);
        screen = IDLE;
        if(selected_tools_index!=toolsMenuList.size()-1) emit pressToolMenuBtn(selected_tools_index);
    }
}

// print options list on display in run script time and choice option set
void UserInterface::optionList(void)
{
    movePoint(&options_cursor_pos);
    movePoint(&options_list_pos);

    display.setPen(0x00);
    display.fill();

    drawProgramBox();
    drawOptionsCursor();
    drawOptionsList();

    drawTime();
    //buttons
    if(selected_options_index > 0) button_enabled[0] = 1;
    else button_enabled[0] = 0;
    if(optionsList.count()
            && selected_options_index < optionsList.count() - 1) button_enabled[1] = 1;
    else button_enabled[1] = 0;
    button_enabled[2] = 1;
    drawButtons(screen);
    drawOptionsExtraText(optionsExtraText);

    if(button == 0 && button_enabled[0])
    {
        disableButtons();
        selected_options_index--;
        initOptionsCursorMove();
    }
    else if(button == 1 && button_enabled[1])
    {
        disableButtons();
        selected_options_index++;
        initOptionsCursorMove();
    }
    else if(button == 2 && button_enabled[2])
    {
        disableButtons();
        QString name;
        if(selected_options_index >= 0) name = optionsList.at(selected_options_index);
        display.setFont(suFont_Font_Times_New_Roman_Height_15[0],
                                sizeof(suFont_Font_Times_New_Roman_Height_15[0]), 2);
        int text_width = display.textWidth(name.toAscii().data());
        initMovingPoint(&sel_options_pos, options_cursor_pos.cur_x + 2, options_cursor_pos.cur_y - 2);
        initPointMove(&sel_options_pos, 255 - text_width, 30, 10);
        optionsExtraText="";
        screen = EXECUTE_INFO;
        //qDebug()<<"index"<<selected_options_index<<optionsList.size()-1;
        //if(selected_options_index!=optionsList.size()-1)
        emit pressOptionMenuBtn(selected_options_index);
    }
}
void UserInterface::optionListDisable(void)
{
  disableButtons();
  optionsExtraText="";
  screen = EXECUTE_INFO;
}

void UserInterface::slotSetOptionExtraText(QString str)
{
  optionsExtraText= str;
}

//print display on execution script time
void UserInterface::executeInfo(void)
{
    display.setPen(0x00);
    display.fill();
    drawTime();

    display.setFont(suFont_Font_Times_New_Roman_Height_15[0],
                    sizeof(suFont_Font_Times_New_Roman_Height_15[0]), 2);
    display.setPen(0xff);

    display.text(0, 0, "Run:");

    display.text(32, 0,current_program_name.toAscii().data());

    display.setFont(suFont_OEM_6x8[0], sizeof(suFont_OEM_6x8[0]), 1);
    display.setPen(0xff);
    display.text(0, 24, current_procedure_name.toAscii().data());

    drawProgressBar();
    //buttons
    if(statusExecute==0){ // run
      button_enabled[0] = 1;
      if(status == DBS_STATUS_RUNNING) {
        button_enabled[1] = 1;
        button_enabled[2] = 0;
      }
      else {
        button_enabled[1] = 0;
        button_enabled[2] = 1;
      }
    }
    else { // pause get from script
      emit pauseScript();
      button_enabled[0] = 1; //stop
      button_enabled[1] = 0; // pause
      button_enabled[2] = 1; // run

    }
    drawButtons(screen);

    if(button == 0 && button_enabled[0])
    {
        disableButtons();
        screen = ABORT_CONFIRM;
    }
    else if(button == 1 && button_enabled[1])
    {
        disableButtons();
        status=DBS_STATUS_PAUSE;
        emit pauseScript();
    }
    else if(button == 2 && button_enabled[2])
    {
        disableButtons();
        status=DBS_STATUS_RUNNING;
        statusExecute=0;
        emit continueScript();
    }
}

// print message screen
void UserInterface::messageScreen(void)
{
    display.setPen(0x00);
    display.fill();

    drawTime();

    display.setPen(0x10);
    int center = (256 - 16) / 2;
    int window_width = 256 - 32;

    display.rectangle(center - window_width/2, 10, window_width, 50);

    display.setFont(suFont_Font_View_Times_New_Roman_Height_26[0],
                    sizeof(suFont_Font_View_Times_New_Roman_Height_26[0]), 4);
    display.setPen(0xff);

    int text_width;
    text_width = display.textWidth(message_screen_header.toAscii().data());
    display.text(center - text_width/2, 10, message_screen_header.toAscii().data());
    text_width = display.textWidth(message_screen_text.toAscii().data());
    display.text(center - text_width/2, 30, message_screen_text.toAscii().data());

    button_enabled[0] = 0;
    button_enabled[1] = 0;
    button_enabled[2] = 1;

    drawButtons(screen);

    if(button == 2 && button_enabled[2])
    {
        disableButtons();
		emit pressMessageBtn();
        screen = IDLE;
    }
}

//print short message screen
void UserInterface::shortMessageScreen(void) //without buttons!
{
    display.setPen(0x00);
    display.fill();

    drawTime();

    display.setPen(0x10);
    int center = (256 - 16) / 2;
    int window_width = 256 - 32;

    display.rectangle(center - window_width/2, 10, window_width, 50);

    display.setFont(suFont_Font_View_Times_New_Roman_Height_26[0],
                    sizeof(suFont_Font_View_Times_New_Roman_Height_26[0]), 4);
    display.setPen(0xff);

    int text_width;
    text_width = display.textWidth(shortMsg.toAscii().data());
    display.text(center - text_width/2, 10, shortMsg.toAscii().data());

}

// confirm abort script screen
void UserInterface::abortConfirmScreen(void)
{
    display.setPen(0x00);
    display.fill();

    drawTime();

    display.setPen(0x10);
    int center = (256) / 2;
    int window_width = 256 - 32*2;

    display.rectangle(center - window_width/2, 10, window_width, 50);

    display.setFont(suFont_Font_View_Times_New_Roman_Height_26[0],
                    sizeof(suFont_Font_View_Times_New_Roman_Height_26[0]), 4);
    display.setPen(0xff);

    QString str = "Stop Program?";
    int text_width;
    text_width = display.textWidth(str.toAscii().data());
    display.text(center - text_width/2, 20, str.toAscii().data());

    drawButtons(screen);
    button_enabled[0] = 1;
    button_enabled[1] = 0;
    button_enabled[2] = 1;

    if(button == 0 && button_enabled[0])
    {
        disableButtons();
        screen = EXECUTE_INFO;
    }
    if(button == 2 && button_enabled[2])
    {
        setTerminateStatus(true);
        emit terminateScript();
        status=DBS_STATUS_STOPPED;
        disableButtons();
        showShortMessage("Wait...");
    }
}

void UserInterface::screensaver(void)
{
    static int dir_x = 2, dir_y = 1;
    static int pos_x = 0, pos_y = 0;

    QString datetime = QDateTime::currentDateTime().toString("h:mm:ss").rightJustified(8, ' ');

    display.setPen(0x00);
    display.fill();
    display.setFont(suFont_Font_View_Times_New_Roman_Height_26[0],
                    sizeof(suFont_Font_View_Times_New_Roman_Height_26[0]), 4);

    pos_x += dir_x;
    pos_y += dir_y;
    if(pos_x < 0) dir_x = 2;
    if(pos_y < -2) dir_y = 1;
    if(pos_x/2 > 256 - display.textWidth(datetime.toAscii().data())) dir_x = -2;
    if(pos_y/2 > 64 - 22) dir_y = -1;


    display.setPen(0x22);
    display.text(pos_x/2, pos_y/2, datetime.toAscii().data());

    if(button >= 0)
    {
        disableButtons();
        slotReInitDisplay();
        screen = IDLE;
    }
}

// screensaver after message on screen
void UserInterface::screensaverAM(void)
{
    static int dir_x = 2, dir_y = 1;
    static int pos_x = 0, pos_y = 0;

    QString datetime = message_screen_header+" "+message_screen_text;

    display.setPen(0x00);
    display.fill();
    display.setFont(suFont_Font_View_Times_New_Roman_Height_26[0],
                    sizeof(suFont_Font_View_Times_New_Roman_Height_26[0]), 4);

    pos_x += dir_x;
    pos_y += dir_y;
    if(pos_x < 0) dir_x = 2;
    if(pos_y < -2) dir_y = 1;
    if(pos_x/2 > 256 - display.textWidth(datetime.toAscii().data())) dir_x = -2;
    if(pos_y/2 > 64 - 22) dir_y = -1;


    display.setPen(0x22);
    display.text(pos_x/2, pos_y/2, datetime.toAscii().data());

    if(button >= 0)
    {
        disableButtons();
        slotReInitDisplay();
        screen = MESSAGE;
    }
}


void UserInterface::drawTime(void)
{
    QString datetime = QDateTime::currentDateTime().toString("h:mm:ss").rightJustified(8, ' ');

    display.setFont(suFont_OEM_6x8[0], 3+8, 1);
    display.setPen(0xff);
    display.text(256 - (datetime.length() * 6), 0, datetime.toAscii().data());
}

void UserInterface::drawSN(void)
{
    QFile file;
    QTextStream text_stream;
    file.setFileName("/etc/hostname");
    text_stream.setDevice(&file);
    file.open(QIODevice::ReadOnly);
    QString hostname = "SN: " + text_stream.readLine();
    display.setFont(suFont_OEM_6x8[0], sizeof(suFont_OEM_6x8[0]), 1);
    display.setPen(0x66);
    display.text(0, 20, hostname.toAscii().data());
}

void UserInterface::drawIP(void)
{
    struct sockaddr_in sin;
    char myaddr[24] = {"IP: "};
    get_myaddress(&sin);
    display.setFont(suFont_OEM_6x8[0], 3+8, 1);
    display.setPen(0x66);
    if(inet_ntop(AF_INET,&sin.sin_addr, myaddr+4, 20) != NULL)
        display.text(0, 29, myaddr);
    else display.text(0, 29, "IP: unknown");
}

void UserInterface::drawSelectedProgram(void)
{
    QString name;
    if(selected_program_index >= 0) name = scriptFileList.at(selected_program_index);
    else name = "";
    display.setFont(suFont_Font_Times_New_Roman_Height_15[0],
                    sizeof(suFont_Font_Times_New_Roman_Height_15[0]), 2);
    display.setPen(0xff);

    int indx=name.lastIndexOf('.');
    if(indx>0) name=name.left(indx);


    display.text(sel_prog_pos.cur_x, sel_prog_pos.cur_y+2, name.toAscii().data());
}

// draw elements on lists prog,tools,option
void UserInterface::drawProgramBox(void)
{
    display.setPen(0x11);
    display.rectangle(PROG_LIST_CURSOR_X - 2, 0, PROG_LIST_CURSOR_WIDTH + 4, PROG_LIST_VERTICAL_PIXEL_COUNT);
}

void UserInterface::drawCursor(void)
{
    display.setPen(0x44);
    display.rectangle(cursor_pos.cur_x, cursor_pos.cur_y, PROG_LIST_CURSOR_WIDTH, PROG_LIST_LINES_SPACE + 1);
}

void UserInterface::drawToolsCursor(void)
{
    display.setPen(0x44);
    display.rectangle(tools_cursor_pos.cur_x, tools_cursor_pos.cur_y, PROG_LIST_CURSOR_WIDTH, PROG_LIST_LINES_SPACE + 1);
}

void UserInterface::drawOptionsCursor(void)
{
    display.setPen(0x44);
    display.rectangle(options_cursor_pos.cur_x, options_cursor_pos.cur_y, PROG_LIST_CURSOR_WIDTH, PROG_LIST_LINES_SPACE + 1);
}

void UserInterface::drawProgramList(void)
{
    display.setPen(0xff);
    display.setFont(suFont_Font_Times_New_Roman_Height_15[0],
                    sizeof(suFont_Font_Times_New_Roman_Height_15[0]), 2);
    //QDir dir(scriptDir);
    //scriptFileList=dir.entryList(scriptExMask,QDir::Files,QDir::Name);
    int count = scriptFileList.count();
    QString ls;
    int indx;
    //QTextCodec *codec = QTextCodec::codecForName("KOI8-R");

    for(int n=0; n<count; n++)
    {
        indx=scriptFileList.at(n).lastIndexOf('.');
        if(indx>0) ls=scriptFileList.at(n).left(indx);
        else
          ls=scriptFileList.at(n);
       // QString s;
       // s=QString::fromUtf8(ls.toLatin1().constData());
        display.text(prog_list_pos.cur_x, prog_list_pos.cur_y + n*PROG_LIST_LINES_SPACE - 2,
                     ls.toAscii().data());
                     //s.toAscii().data()  );
                     //codec->fromUnicode(ls).data());
                    // ls.toUtf8().data());
        //qDebug()<<codec->fromUnicode(ls).data()<<QString::fromUtf8(ls.toLatin1().constData());
    }
}

void UserInterface::drawToolsList(void)
{
    display.setPen(0xff);
    display.setFont(suFont_Font_Times_New_Roman_Height_15[0],
                    sizeof(suFont_Font_Times_New_Roman_Height_15[0]), 2);
    int count = toolsMenuList.count();

    for(int n=0; n<count; n++)
    {
        display.text(tools_list_pos.cur_x, tools_list_pos.cur_y + n*PROG_LIST_LINES_SPACE - 2,
                     toolsMenuList.at(n).toAscii().data());
    }
}

void UserInterface::drawOptionsList(void)
{
    display.setPen(0xff);
    display.setFont(suFont_Font_Times_New_Roman_Height_15[0],
                    sizeof(suFont_Font_Times_New_Roman_Height_15[0]), 2);
    int count = optionsList.count();

    for(int n=0; n<count; n++)
    {
        display.text(options_list_pos.cur_x, options_list_pos.cur_y + n*PROG_LIST_LINES_SPACE - 2,
                     optionsList.at(n).toAscii().data());
    }
}

void UserInterface::drawOptionsExtraText(QString str)
{
  if(str.isEmpty()) return;
  display.setPen(0xff);
  display.setFont(suFont_Font_Times_New_Roman_Height_15[0],
                  sizeof(suFont_Font_Times_New_Roman_Height_15[0]), 2);
  if(str.length()>16) str[16]=0;
  display.text(140,48,str.toAscii().data());

}

void UserInterface::drawButtons(int button_set)
{
    for(int n=0; n<3; n++)
    {
        if(button_sheme[button_set][n] != 0)
        {
            display.setPen(button_color[n] | (button_color[n] << 4));
            display.picture((256/2 - 8)*n, 64 - 16, suFontButton[0]);
            if(button_color[n] > BUTTON_COLOR_BLACK) button_color[n]--;

            int pen;
            if(button_enabled[n]) pen = BUTTON_COLOR_WHITE | (BUTTON_COLOR_WHITE << 4);
            else pen = BUTTON_COLOR_WHITE_DISABLED | (BUTTON_COLOR_WHITE_DISABLED << 4);
            display.setPen(pen);
            display.picture((256/2 - 8)*n, 64 - 16, suFontButton[button_sheme[button_set][n]]);
        }
    }
}

void UserInterface::disableButtons(void)
{
    for(int n=0; n<3; n++) button_enabled[n] = 0;
}

void UserInterface::drawProgressBar(void)
{
    display.setPen(0xff);
    display.rectangle(0, 36, 256, 7);
    display.setPen(0x22);
    display.rectangle(1,37, 254, 5);
    display.setPen(0x66);
    display.rectangle(2, 38, 252*execution_progress/100, 3);
}

// prepare procedure before display scripts list
void UserInterface::gotoProgramList(void)
{
    selected_program_index = -1;
    QDir dir(scriptDir);
    scriptFileList=dir.entryList(scriptExMask,QDir::Files,QDir::Name);
    int count = scriptFileList.count();

    if(count && selected_program_index == -1) selected_program_index = 0;

    int cursor_x = PROG_LIST_CURSOR_X;
    int cursor_y;
    if(count*PROG_LIST_LINES_SPACE < PROG_LIST_VERTICAL_PIXEL_COUNT)
        cursor_y = selected_program_index*PROG_LIST_LINES_SPACE;
    else
    {
        if(count != 1) cursor_y = selected_program_index*(PROG_LIST_VERTICAL_PIXEL_COUNT - 1 - PROG_LIST_LINES_SPACE)/(count-1);
        else cursor_y = 0;
    }
    initMovingPoint(&cursor_pos, cursor_x, cursor_y, 10);

    int prog_x = cursor_x + 2;
    int prog_y = cursor_y - selected_program_index * PROG_LIST_LINES_SPACE;
    initMovingPoint(&prog_list_pos, prog_x, prog_y, 10);

    screen = PROG_LIST;
}

// prepare procedure before display scripts list
void UserInterface::gotoTestsList(void)
{
    selected_program_index = -1;
    QDir dir(testsDir);
    scriptFileList=dir.entryList(scriptExMask,QDir::Files,QDir::Name);
    qDebug()<<scriptFileList;
    int count = scriptFileList.count();

    if(count && selected_program_index == -1) selected_program_index = 0;

    int cursor_x = PROG_LIST_CURSOR_X;
    int cursor_y;
    if(count*PROG_LIST_LINES_SPACE < PROG_LIST_VERTICAL_PIXEL_COUNT)
        cursor_y = selected_program_index*PROG_LIST_LINES_SPACE;
    else
    {
        if(count != 1) cursor_y = selected_program_index*(PROG_LIST_VERTICAL_PIXEL_COUNT - 1 - PROG_LIST_LINES_SPACE)/(count-1);
        else cursor_y = 0;
    }
    initMovingPoint(&cursor_pos, cursor_x, cursor_y, 10);

    int prog_x = cursor_x + 2;
    int prog_y = cursor_y - selected_program_index * PROG_LIST_LINES_SPACE;
    initMovingPoint(&prog_list_pos, prog_x, prog_y, 10);

    screen = PROG_LIST;
}

// prepare procedure before display tools list
void UserInterface::gotoToolsList(void)
{
    selected_tools_index = -1;

    int count = toolsMenuList.count();
	selected_tools_index=0;

    int cursor_x = PROG_LIST_CURSOR_X;
    int cursor_y;
    if(count*PROG_LIST_LINES_SPACE < PROG_LIST_VERTICAL_PIXEL_COUNT)
        cursor_y = selected_tools_index*PROG_LIST_LINES_SPACE;
    else
    {
        if(count != 1) cursor_y = selected_tools_index*(PROG_LIST_VERTICAL_PIXEL_COUNT - 1 - PROG_LIST_LINES_SPACE)/(count-1);
        else cursor_y = 0;
    }
    initMovingPoint(&tools_cursor_pos, cursor_x, cursor_y, 10);

    int prog_x = cursor_x + 2;
    int prog_y = cursor_y - selected_tools_index * PROG_LIST_LINES_SPACE;
    initMovingPoint(&tools_list_pos, prog_x, prog_y, 10);

    screen = TOOLS_LIST;
}

// prepare procedure before display options list
void UserInterface::gotoOptionsList(void)
{
    selected_options_index = -1;

    int count = optionsList.count();
    selected_options_index=0;

    int cursor_x = PROG_LIST_CURSOR_X;
    int cursor_y;
    if(count*PROG_LIST_LINES_SPACE < PROG_LIST_VERTICAL_PIXEL_COUNT)
        cursor_y = selected_options_index*PROG_LIST_LINES_SPACE;
    else
    {
        if(count != 1) cursor_y = selected_options_index*(PROG_LIST_VERTICAL_PIXEL_COUNT - 1 - PROG_LIST_LINES_SPACE)/(count-1);
        else cursor_y = 0;
    }
    initMovingPoint(&options_cursor_pos, cursor_x, cursor_y, 10);

    int prog_x = cursor_x + 2;
    int prog_y = cursor_y - selected_options_index * PROG_LIST_LINES_SPACE;
    initMovingPoint(&options_list_pos, prog_x, prog_y, 10);

    screen = OPTIONS_LIST;
}


void UserInterface::initCursorMove(void)
{
    QDir dir(scriptDir);
    scriptFileList=dir.entryList(scriptExMask,QDir::Files,QDir::Name);
    int count = scriptFileList.count();
    int cursor_x = PROG_LIST_CURSOR_X;
    int cursor_y;
    if(count*PROG_LIST_LINES_SPACE < PROG_LIST_VERTICAL_PIXEL_COUNT)
        cursor_y = selected_program_index*PROG_LIST_LINES_SPACE;
    else
    {
        if(count != 1) cursor_y = selected_program_index*(PROG_LIST_VERTICAL_PIXEL_COUNT - 1 - PROG_LIST_LINES_SPACE)/(count-1);
        else cursor_y = 0;
    }
    initPointMove(&cursor_pos, cursor_x, cursor_y);

    int prog_x = cursor_x + 2;
    int prog_y = cursor_y - selected_program_index * PROG_LIST_LINES_SPACE;
    initPointMove(&prog_list_pos, prog_x, prog_y, 10);
}

void UserInterface::initToolsCursorMove(void)
{
    int count = toolsMenuList.count();

    int cursor_x = PROG_LIST_CURSOR_X;
    int cursor_y;
    if(count*PROG_LIST_LINES_SPACE < PROG_LIST_VERTICAL_PIXEL_COUNT)
        cursor_y = selected_tools_index*PROG_LIST_LINES_SPACE;
    else
    {
        if(count != 1) cursor_y = selected_tools_index*(PROG_LIST_VERTICAL_PIXEL_COUNT - 1 - PROG_LIST_LINES_SPACE)/(count-1);
        else cursor_y = 0;
    }
    initPointMove(&tools_cursor_pos, cursor_x, cursor_y);

    int prog_x = cursor_x + 2;
    int prog_y = cursor_y - selected_tools_index * PROG_LIST_LINES_SPACE;
    initPointMove(&tools_list_pos, prog_x, prog_y, 10);
}

void UserInterface::initOptionsCursorMove(void)
{
    int count = optionsList.count();

    int cursor_x = PROG_LIST_CURSOR_X;
    int cursor_y;
    if(count*PROG_LIST_LINES_SPACE < PROG_LIST_VERTICAL_PIXEL_COUNT)
        cursor_y = selected_options_index*PROG_LIST_LINES_SPACE;
    else
    {
        if(count != 1) cursor_y = selected_options_index*(PROG_LIST_VERTICAL_PIXEL_COUNT - 1 - PROG_LIST_LINES_SPACE)/(count-1);
        else cursor_y = 0;
    }
    initPointMove(&options_cursor_pos, cursor_x, cursor_y);

    int prog_x = cursor_x + 2;
    int prog_y = cursor_y - selected_options_index * PROG_LIST_LINES_SPACE;
    initPointMove(&options_list_pos, prog_x, prog_y, 10);
}

void UserInterface::initMovingPoint(MovingPoint *point, int x, int y, int step_count)
{
    point->cur_x = x;
    point->cur_y = y;
    point->src_x = x;
    point->src_y = y;
    point->dst_x = x;
    point->dst_y = y;
    point->step = step_count;
    point->step_count = step_count;
}

void UserInterface::initPointMove(MovingPoint *point, int x, int y, int step_count)
{
    if(step_count > 0) point->step_count = step_count; // change if necessary
    point->step = 0;
    point->src_x = point->cur_x;
    point->src_y = point->cur_y;
    point->dst_x = x;
    point->dst_y = y;
}

void UserInterface::movePoint(MovingPoint* point)
{
    if(point->step < point->step_count)
    {
        point->step++;
        point->cur_x = point->src_x + (point->dst_x - point->src_x) * point->step / point->step_count;
        point->cur_y = point->src_y + (point->dst_y - point->src_y) * point->step / point->step_count;
    }
}

void UserInterface::generateJulia(uint16_t size_x, uint16_t size_y,
                                  uint16_t offset_x, uint16_t offset_y, uint16_t zoom, int fader_value)
{
    float tmp1, tmp2;
    float num_real, num_img;
    float radius;
    int16_t i;
    int16_t x, y;

    int ITERATION = 16;

    for (y=0; y<size_y; y++)
    {
        for (x=0; x<size_x; x++)
        {
            num_real = x - offset_x;
            num_real = num_real / zoom;
            num_img = y - offset_y;
            num_img = num_img / zoom;
            i=0;
            radius = 0;
            while ((i<ITERATION-1) && (radius < 4))
            {
                tmp1 = num_real * num_real;
                tmp2 = num_img * num_img;
                num_img = 2*num_real*num_img + IMG_CONSTANT;
                num_real = tmp1 - tmp2 + REAL_CONSTANT;
                radius = tmp1 + tmp2;
                i+=1;
            }
            int pen = i/2;
            pen |= pen << 4;
            display.setPen(pen);
            display.point(x, y);
        }
    }
}
