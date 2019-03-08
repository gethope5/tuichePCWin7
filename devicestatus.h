#ifndef DEVICESTATUS_H
#define DEVICESTATUS_H
#include <qtextcodec.h>
#include <QPushButton>
#include "syshead.h"
#include <qdebug.h>
class DeviceStatus : public QPushButton
{
    Q_OBJECT
public:
    explicit DeviceStatus(QString deviceName,DEVICE_CONNECT_STATUS initialFlag,QWidget *parent = 0);
    void setDeviceStatusInitial(QString deviceName,DEVICE_CONNECT_STATUS initialFlag);
    void setCurStatus(DEVICE_CONNECT_STATUS flag);//flag=true,设备连接；false，设备掉线
signals:

public slots:
    void slot_deviceStatusChange(DEVICE_CONNECT_STATUS flag);//flag=true,设备连接；false，设备掉线
};

#endif // DEVICESTATUS_H
