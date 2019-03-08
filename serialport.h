#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
//189382287
//#include "serialControlModules/win_qextserialport.h"
#include <qtimer.h>
#include <QSettings>
#include <QDebug>
#include <QString>
#include <qt_windows.h>
#include <QStringList>
#include "math.h"
#include <QMap>
#include <QtWidgets>
#include <qmessagebox.h>

#include <qthread.h>
#include <QtSerialPort>
#define SYS_COM 1
#define DEBUG_COM_0428 0
#define CARD_OUTPUT "Q1"
#define SENSOR_OUTPUT "Q2"
#define zbw_gps 1
#define else_gps 0
#define EARTH_RADIUS  6371.0;//km 地球半径 平均值，千米
#define PI 3.1415926

//gps数据格式
struct GPSInfo
{
    QString GPSTime;//GPS时间
    bool bStatus;//GPS状态 A=数据有效；V=数据无效
    double Latitude; //纬度
    QString location1;//南北半球
    double Longitude;//经度
    QString location2;//东西半球
    QString GPSDate;//GPS日期
    GPSInfo(){
        bStatus=false;
    }
};
class SerialPort : /*public QObject,*/public QThread
{
    Q_OBJECT
public:
    explicit SerialPort(QObject *parent = 0);
    ~SerialPort();

    bool isOpen(void);
    double gpsDistance(double lat1,double lon1, double lat2,double lon2);
    void setGpsEnable(bool);
private:
    bool connectDevice(bool);
    GPSInfo gpsinfo;
    void sendString(QString strSend);
    QString comName;
    bool m_bOpen;
    QByteArray recData;
    QSerialPort *serial;
    bool bPackageOk;
    void GpsAnaly(QByteArray recdata);  //1
    QTimer *timer;
    void getComSet(QString &name);
    //gps距离计算
    double ConvertRadiansToDegrees(double radian);
    double ConvertDegreesToRadians(double degrees);
    double HaverSin(double theta);
    QString convertGPSDot(QByteArray mm);
    int abs_speed;
    bool bSimGps;
signals:
    void updateGps(GPSInfo &);
public slots:
    void readComData();
    void slot_updateGps(void);
};

#endif // SERIALPORT_H
