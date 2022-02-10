#ifndef USERINTERFACE_H
#define USERINTERFACE_H

#include "display.h"
#include <QObject>
#include <QDebug>
#include <QMutex>
#include <QProcess>

#define SCREENSAVER_ENABLE_TIME     1000

struct MovingPoint
{
    int src_x, src_y;
    int dst_x, dst_y;
    int step;
    int step_count;
    int cur_x, cur_y;
};

class UserInterface : public QObject
{
    Q_OBJECT
public:
    explicit UserInterface(QObject *parent = 0);
    ~UserInterface(void);

    bool init(QString scrDir, QString tstDir, QStringList scrExMask);
    void deinit(void);

    void scanButtons(void);
    void showMessage(QString header, QString text);
    void showShortMessage(QString);
    void displayUpdate(void);
    void setOptionsList(QString str) ;
    void printStatus(QString string);   // debug
    
    //data to display
    int status;
	int canRefresh;
    QString current_program_name;
    QString current_procedure_name;
    QString runProgramName;
    QString optionsExtraText;
    int execution_progress;
    int statusExecute; // status from script program 0 if run 1 - pause
    bool runSelectProgram(void){ return runSelectProg; }
    void clearRunSelectProgram(void){ runSelectProg=false; }
    bool readCanTerminateProg(void){ return canTerminateProg; }
    void setTerminateStatus(bool sts) { canTerminateProg=sts; }
    void setTermPid(Q_PID pid){ termPid=pid; }
signals:
    void message(QString string);
    void pressToolMenuBtn(int btn);
    void pressOptionMenuBtn(int);
	void pressMessageBtn(void);
    void terminateScript(void);
    void pauseScript(void);
    void continueScript(void);
public slots:
    void slotReInitDisplay(void);
    void optionListDisable(void);
    void slotSetOptionExtraText(QString);
private slots:
protected:
    //void timerEvent(QTimerEvent *event){displayUpdate();}
private:
    void intro(void);
    void idle(void);
    void programList(void);
	void toolsList(void);
    void optionList(void);
    void executeInfo(void);
    void messageScreen(void);
    void shortMessageScreen(void);
    void abortConfirmScreen(void);
    void screensaver(void);
    void screensaverAM(void);


    void drawTime(void);
    void drawSN(void);
    void drawIP(void);
    void drawSelectedProgram(void);
    void drawProgramBox(void);
    void drawCursor(void);
	void drawToolsCursor(void);
    void drawOptionsCursor(void);
    void drawProgramList(void);
	void drawToolsList(void);
    void drawOptionsList(void);
    void drawOptionsExtraText(QString);
    void drawButtons(int button_set);
    void drawProgressBar(void);

    void gotoProgramList(void);
    void gotoTestsList(void);
	void gotoToolsList(void);
    void gotoOptionsList(void);
    void disableButtons(void);

    void initCursorMove(void);
    void initToolsCursorMove(void);
    void initOptionsCursorMove(void);
    void initMovingPoint(MovingPoint* point, int x, int y, int step_count = 1);
    void initPointMove(MovingPoint* point, int x, int y, int step_count = 0);
    void movePoint(MovingPoint* point);

    void generateJulia(uint16_t size_x, uint16_t size_y,
                           uint16_t offset_x, uint16_t offset_y, uint16_t zoom, int fader_value);
    float REAL_CONSTANT;//-0.8;//0.285;
    float IMG_CONSTANT;//0.156;//0.01;

    enum {INTRO=0, IDLE=1, PROG_LIST=2, EXECUTE_INFO=3, MESSAGE=4, ABORT_CONFIRM=5, SCREENSAVER=6,TOOLS_LIST=7,SHORTMESSAGE=8,OPTIONS_LIST=9,SCREENSAVERAM=10} screen;
#ifndef i386
    Display display;
#endif
    QString message_screen_header;
    QString message_screen_text;
    QString shortMsg;
    QString scriptDir; // string with script directory
    QString testsDir; // string with tests script directory
    QStringList scriptExMask; //list with mask for extentions of script file
    int button;

    int selected_program_index;

    MovingPoint cursor_pos;
    MovingPoint prog_list_pos;
    MovingPoint sel_prog_pos;

    int button_color[3];
    int button_enabled[3];
	
	int selected_tools_index;

    MovingPoint tools_cursor_pos;
    MovingPoint tools_list_pos;
    MovingPoint sel_tools_pos;

    int selected_options_index;

    MovingPoint options_cursor_pos;
    MovingPoint options_list_pos;
    MovingPoint sel_options_pos;
	
	QList <QString> scriptFileList;
	QList <QString> toolsMenuList;
    QList <QString> optionsList;
    bool runSelectProg,canTerminateProg;
    Q_PID termPid;    

};

#endif // USERINTERFACE_H
