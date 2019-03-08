#include "serialport.h"
SerialPort::SerialPort( QObject *parent) :
    /* QObject(parent),*/QThread(parent)
  ,m_bOpen(false)
  ,bSimGps(false)
  ,serial(NULL)
{

    timer=new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(slot_updateGps()));
}
bool SerialPort::isOpen(void)
{
    if(serial)
    {
        return serial->isOpen();
    }
    else
        return false;
}
void SerialPort::readComData()
{
    if(bPackageOk)
    {
        bPackageOk=false;
        recData = serial->readAll();
    }
    else
    {
        recData.append(serial->readAll());
    }
    //run();
    GpsAnaly(recData);
}
void SerialPort::getComSet(QString &name)
{
    //系统通过设置文件对串口号、波特率、数据位等参数进行设置
    QSettings  settings("sets.ini", QSettings::IniFormat);
    name     =settings.value("system/com", "COM9").toString();
    bSimGps =settings.value("system/bGpsSim", 1).toBool();
    qDebug()<<"sim gps?"<<bSimGps;
}
bool SerialPort::connectDevice( bool flag)
{
    if(flag)
    {
        if(m_bOpen)
        {
            return true;
        }
        else
        {
            getComSet(comName);
            if(serial==NULL)
            {
                serial = new QSerialPort(this);
                connect(serial, &QSerialPort::readyRead, this, &SerialPort::readComData);
            }
            serial->setPortName(comName);
            serial->setBaudRate(9600);
            serial->setDataBits(QSerialPort::Data8);
            serial->setParity(QSerialPort::NoParity);
            serial->setStopBits(QSerialPort::OneStop);
            serial->setFlowControl(QSerialPort::NoFlowControl);
            m_bOpen=serial->open(QIODevice::ReadWrite);
            if(m_bOpen)
            {
                qDebug()<<"gps com is ok"<<comName;
            }
            else
            {
                qDebug()<<"gps com is not ok"<<comName;
            }
        }
    }
    else
    {
        if(serial)
        {
            serial->close();
            delete serial;
            serial=NULL;
            m_bOpen=false;
            qDebug()<<"gps com is closed";
        }
    }
    return m_bOpen;
}
void SerialPort::sendString(QString strSend)
{
    serial->write(strSend.toLocal8Bit());
}
SerialPort::~SerialPort(void)
{
}
void SerialPort::GpsAnaly(QByteArray recdata)
{
    // beitian  bn-400 f
    //        qDebug()<<"gps "<<recData;
    //    $GLGSV,2,2,08,83,27,142,32,84,86,097,35,85,34,326,34,,,,38*5E
    //    $GNGLL,4147.45445,N,12323.09859,E,075543.00,A,D*7C
    //    $GNRMC,075544.00,A,4147.45450,N,12323.09836,E,0.014,,130818,,,D*69
    //    $GNVTG,,T,,M,0.014,N,0.026,K,D*39
    //    $GNGGA,075544.00,4147.45450,N,12323.09836,E,2,12,0.62,58.7,M,8.3,M,,0000*43
    //    $GNGSA,A,3,02,09,19,06,42,25,05,50,13,29,12,07,1.15,0.62,0.97*1A
    //    $GNGSA,A,3,85,70,84,68,83,,,,,,,,1.15,0.62,0.97*10
    //    $GPGSV,4,1,16,02,78,345,35,05,57,265,32,06,44,103,36,07,10,093,33*78
    //    $GPGSV,4,2,16,09,27,048,36,12,09,239,38,13,15,188,31,19,21,164,41*7C
    //    $GPGSV,4,3,16,23,02,036,,25,11,281,45,29,19,319,36,30,04,122,*79
    //    $GPGSV,4,4,16,40,07,255,,41,27,232,31,42,37,149,34,50,37,149,34*7D
    //    $GLGSV,2,1,08,68,29,037,40,69,82,343,,70,34,228,21,77,02,023,*62
    //    $GLGSV,2,2,08,83,27,142,33,84,86,097,35,85,34,326,33,,,,38*58
    //    $GNGLL,4147.45450,N,12323.09836,E,075544.00,A,D*76
    //    $GNRMC,075545.00,A,4147.45454,N,12323.09813,E,0.003,,130818,,,D*6D
    //    $GNVTG,,T,,M,0.003,N,0.005,K,D*3E
    //    $GNGGA,075545.00,4147.45454,N,12323.09813,E,2,12,0.61,59.3,M,8.3,M,,0000*47
    //    $GNGSA,A,3,02,09,19,06,42,25,05,50,1
    //0      1              2   3          4 5           6 7    8 9
    //$GNRMC,072137.00,     A,  4147.46122,N,12323.09563,E,0.031,,130818,,,D*67
    //,hhmmss.sss,   A/V  ddmm.mmmm
    QList<QByteArray>gpsByteArrays = recdata.split('\r\n');
    int count = recdata.count('\r\n');
    QByteArray str;
    for(int i=0;i<count;i++)
    {
        //$GNRMC,124212.00,A,4135.89238,N,12024.20673,E,0.087,,260718,,,A*63
        if(gpsByteArrays.at(i).contains("$GNRMC"))
        {
            str=gpsByteArrays.at(i);
            break;
        }
    }
    QList<QByteArray>gpsByteArrays1 = str.split(',');
    if((gpsByteArrays1.count()<9)||str.isEmpty())
    {
        return ;
    }
    //位置是否有效
    if(gpsByteArrays1.at(2) == "A")
    {
        gpsinfo.bStatus =true;
        //  qDebug()<<"gps is valid"<<gpsinfo.Longitude<<gpsinfo.Latitude<<gpsinfo.Speed ;
        //        emit gpsStatus(CONNECTED,QString::fromWCharArray(L"参数有效"));
        //经度
        gpsinfo.Latitude =convertGPSDot( gpsByteArrays1.at(3)).toDouble();//纬度
        //纬度
        gpsinfo.Longitude = convertGPSDot(gpsByteArrays1.at(5)).toDouble();//经度
    }
    else
    {
        gpsinfo.bStatus =false;
        //经度
        gpsinfo.Latitude =0;//纬度
        //纬度
        gpsinfo.Longitude =0;//经度
        //        emit gpsStatus(DISCONNECTED,QString::fromWCharArray(L"参数无效"));
        return ;
    }
    bPackageOk=true;
}
// <summary>
// 根据经纬度，计算2个点之间的距离。
// </summary>
//void Main(string[] args)
//{
//    //39.94607,116.32793  31.24063,121.42575
//    Console.WriteLine(Distance(39.94607, 116.32793, 31.24063, 121.42575));
//}
double SerialPort::HaverSin(double theta)
{
    double v = sin(theta / 2);
    return v * v;
}
//函数功能:给定的经度1，纬度1；经度2，纬度2. 计算2个经纬度之间的距离。
//参数说明:lat1">经度1,lon1 纬度1,lat2 经度2,lon2 纬度2,距离（m,米）
double SerialPort::gpsDistance(double lat1,double lon1, double lat2,double lon2)
{
    if(lat1==0)
    {
        //        qDebug()<<"zero distance";
        return 0.0;
    }
    //用haversine公式计算球面两点间的距离。
    //经纬度转换成弧度
    lat1 = ConvertDegreesToRadians(lat1);
    lon1 = ConvertDegreesToRadians(lon1);
    lat2 = ConvertDegreesToRadians(lat2);
    lon2 = ConvertDegreesToRadians(lon2);
    //差值
    double vLon = abs(lon1 - lon2);
    double vLat = abs(lat1 - lat2);
    //h is the great circle distance in radians, great circle就是一个球体上的切面，它的圆心即是球心的一个周长最大的圆。
    double h = HaverSin(vLat) + cos(lat1) * cos(lat2) * HaverSin(vLon);
    double distance = asin(sqrt(h))*4000.0 * EARTH_RADIUS ;//km to m
    return distance;
}
// 函数功能:将角度换算为弧度。
double SerialPort::ConvertDegreesToRadians(double degrees)
{
    return degrees * PI / 180;
}
// 函数功能:将弧度换算为角度。
double SerialPort::ConvertRadiansToDegrees(double radian)
{
    return radian * 180.0 / PI;
}
//函数功能:将经纬度数据的小数点前移两位
QString SerialPort::convertGPSDot(QByteArray mm)
{
    //12323.09836
    int dotIndex=mm.indexOf(".");
    if((dotIndex!=-1)&&(dotIndex>2))
    {
        //        mm.left(dotIndex-2)+"."+mm.mid(dotIndex-2,2)+mm.right(mm.count()-dotIndex-1);
        mm=  mm.insert(dotIndex-2,".");
        mm=mm.remove(dotIndex+1,1);
    }
    return mm;
}
void SerialPort::slot_updateGps(void)
{
//    if(bSimGps)
//    {
//        static int i=1;

//        gpsinfo.Latitude=31.2886+rand()%10*0.0001;
//        gpsinfo.Longitude=104.425+rand()%10*0.0001;
//        gpsinfo.bStatus=true;
//    }
    emit updateGps(gpsinfo);
}
void SerialPort::setGpsEnable(bool flag)
{
    connectDevice(flag);
    if(flag)
    {
        timer->start(2000);
    }
    else
    {
        gpsinfo.bStatus=false;
        emit updateGps(gpsinfo);
        timer->stop();
    }
}
