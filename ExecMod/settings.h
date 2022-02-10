#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QStringList>
// default UV light turn off interval
#define UIOFFDEFTIME 45
class Settings
{
public:
    Settings();
    void load(void);
    QString devicePort;
    QString scriptsDirName;
    QString testsScriptsDirName;
    QString pythonExMask,jsExMask;
    QString bcScriptProcFile;
    QString logicLogFN;
    QString cameraIPAddress;
    //QString BSManufacturer;
    //QString BSNames;
    QStringList scanners;
    bool enableLog,enableSound,enableLight,enableSig,disablePauseOnCoverUP,enableCANtest;
    int uvOffInterval;
};

#endif // SETTINGS_H
