#include "settings.h"
#include <QFile>
#include <QXmlStreamReader>

Settings::Settings()
{
    enableLog=0;
    enableSound=0;
    enableLight=1;
    enableSig=1;
    enableCANtest=0;
    uvOffInterval=UIOFFDEFTIME;// default value 45 minuts
    disablePauseOnCoverUP=0;
    logicLogFN="logic_log.txt";
    cameraIPAddress="";
}

void Settings::load(void)
{
    QFile xmlFile("settings.xml");
    xmlFile.open(QIODevice::ReadOnly);
    QXmlStreamReader reader(&xmlFile);

    QString level_name;
    pythonExMask="";
    jsExMask="";
    bcScriptProcFile="";
    scanners.clear();

    //BSManufacturer="Honeywell";
    //BSNames="scanner CCB04";

    while (!reader.atEnd())
    {
        reader.readNext();

        if(reader.tokenType() == QXmlStreamReader::StartElement)
        {
            level_name = reader.name().toString();
        }
        else if(reader.tokenType() == QXmlStreamReader::EndElement) level_name = "";
        else if(reader.tokenType() == QXmlStreamReader::Characters)
        {
          if(level_name == "scripts_dirname")
          {
              scriptsDirName = reader.text().toString();
          }
          else if(level_name == "tests_dirname")
          {
            testsScriptsDirName = reader.text().toString();
          }
          else if(level_name == "py_exmask")
          {
            pythonExMask = reader.text().toString();
          }
          else if(level_name == "js_exmask")
          {
            jsExMask = reader.text().toString();
          }
          else if(level_name == "enable_log")
          {
              enableLog = reader.text().toString().toInt();
          }
          else if(level_name == "enable_sound")
          {
            enableSound = reader.text().toString().toInt();
          }
          else if(level_name == "enable_light")
          {
            enableLight = reader.text().toString().toInt();
          }
          else if(level_name == "enable_sig")
          {
            enableSig = reader.text().toString().toInt();
          }
          else if(level_name=="disable_cover_pause")
          {
            disablePauseOnCoverUP=reader.text().toString().toInt();
          }
          else if(level_name == "uv_off_interval")
          {
            uvOffInterval = reader.text().toString().toInt();
          }
          else if(level_name == "bc_script")
          {
            bcScriptProcFile = reader.text().toString();
          }
          else if(level_name == "logiclog_fname")
          {
            logicLogFN = reader.text().toString();
          }
          else if(level_name == "camera_IP")
          {
            cameraIPAddress = reader.text().toString();
          }
          else if(level_name == "enable_CANtest")
          {
            enableCANtest = reader.text().toString().toInt();
          }
            /*else if(level_name == "scanners_manufacturer")
            {
              BSManufacturer+= reader.text().toString();
            }
            else if(level_name == "scanners_product")
            {
              BSNames+= reader.text().toString();
            }*/
          else if(level_name == "dev_scanners")
          {
            scanners+= reader.text().toString();
          }
        }
    }
}
