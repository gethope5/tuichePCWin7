#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <boost/thread/mutex.hpp>
#include <QTimer>
#include <QMap>
#include <QComboBox>
#include "NetLib.h"
#include "serialport.h"
#include "VirtualKeyBoard.h"
#include "qcustomplot.h"
#define remote_table_name "branchNeijiang"
#include <qsqldatabase.h>
#define neijiang_modify 0
//1,2点测量500高差；0，4点测量500高差
#define gaocha500_2point 1
typedef struct
{
    QString tm;
    QString pole;
    QString railWidth;
    QString railHeight;
    QString wireWidth;
    QString wireHeight;
}remote_data;
#define tmIndex         0
#define railIndex       1
#define poleIndex       2
#define railWidthIndex  3
#define railHeightIndex 4
#define wireWidthIndex  5
#define wireHeightIndex 6
namespace Ui {
class MainWidget;
}
struct curveInfo
{
    int curveIndex;
    QVector<double> x;
    QVector<double> y;
    QColor color;
    curveInfo()
    {
        x.clear();
        y.clear();
        curveIndex=0;
        color=Qt::red;
    }
    void clear(void)
    {
        x.clear();
        y.clear();
    }
};

class SerialItem
{
public:
    std::string m_cmd;          //指令字符
    std::vector< std::pair<int/* 索引 */,int/* 值 */> > m_param;                  //参数表
    int m_check;                //校验字符
    std::string m_ori_str;      //原始字符
};
class QwtPlotCurve;
class QwtPlotMarker;
class MainWidget : public QWidget
{
    Q_OBJECT
public:
    enum MainPageType
    {//主页面的stack对应
        MPT_Main = 0,
        MPT_BaseMeasure = 1,
        MPT_DataManager = 2,
        MPT_SystemConfig = 3,
        MPT_DataView = 4,
        MPT_JDQJ = 5,//角度倾角界面
        MPT_GPS=6
    };
    enum MeasurePageType
    {//测量页面的stack对应
        MPT_JBCL = 0,//基本测量
        MPT_KZCL = 1,//
        MPT_CMJX = 2,
        MPT_ZXTY = 3,
        MPT_500GC = 4,
        MPT_HXGD = 5,
        MPT_JGGD = 6,
        MPT_FZ = 7,
        MPT_DXGD = 8,
        MPT_DWPD = 9,
        MPT_ZYCL = 10,//自由测量
        MPT_DXPD = 11,//导线坡度
        MPT_KJ=12
    };
public:
    explicit MainWidget(QWidget *parent = 0);
    ~MainWidget();

private slots:
    //主界面退出按钮
    void on_btnExit_clicked();

    //主界面点击基本测量按钮
    void on_btnMainMeasure_clicked();

    //主界面点击数据管理按钮
    void on_btnMainDataManager_clicked();

    //主界面点击系统设置按钮
    void on_btnMainSystemConfig_clicked();
public:
    /**
     * @brief 用于截获摄像头绘制事件
     * @param obj
     * @param event
     * @return
     */
    virtual bool eventFilter(QObject *obj, QEvent *event);
public:
    /**
     * @brief 当获取服务端的信息时调用该函数
     * @param msg 获取得到的信息
     */
    void OnReadNetMessage(boost::shared_ptr<MessageStu> msg);
signals:
    void DoGetInfo(SerialItem);
    void DoGetNetMsg(boost::shared_ptr<MessageStu>);
private:
    int convert_yuv_to_rgb_pixel(int y, int u, int v);
    int convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height);
    void AnlyzeReadBuffer();
    bool StringToSerialItem(const std::string &str, SerialItem &item);
    bool CheckSerialItem(const SerialItem &item);
    void InitCMDDic();
    //重置所有测量类的显示
    void ResetAllMeasureLabel();
    void SendSerialMsg(const std::string& str_msg);
    //请求数据同步
    void NetRequestDateUpdata();
    //初始化系统配置界面
    void InitSystemConfigPage();
    //显示组界面
    void ShowMain(bool show);
    //初始化站区选择
    void InitField();
    //用以辅助锚段选择的数据库查询函数
    QStringList GetSqliteField(const QString& xl="", const QString& xj="", const QString& cj="", const QString& gq="", const QString& zq="");
    //获取本地存储数据的列表
    QStringList GetDataPathList();
    //获取本地线路列表
    QStringList GetXLPathList();
    //通过网络端在服务器运行sql语句
    void RunSqlWithNet(const QString& cmd);
    //初始化数据管理页面
    void InitDataManager();
    //写入测量数据
    void WriteMeasureData(QString str_cmd);
    //写入电机运动
    void ControlMoto(bool direct/*方向*/, bool small_control/*微动*/);
    //初始化数据显示
    void InitDataView(QString xl_name);

    bool GetDiskUIsExist();
    //将当前的内容嫁到combox中
    void AddCurrentTextToComb(QComboBox* cmb);
    bool IsStringInComb(QComboBox *cmb, QString str);
    void FlushDataView();
private slots:
    //连接服务器，为了界面不卡
    void Connect();
    void OnUpdateTimer();
    void OnMotoTimerOut();
    //點擊屏幕是進行運動
    void MotoRun(int dis);
    /**
     * @brief 当收到串口信息
     * @param item
     */
    void OnGetInfo(SerialItem item);

    void OnGetNetMsg(boost::shared_ptr<MessageStu> msg);
    //测量界面页面切换按钮的响应
    void on_btnJBCL_clicked();
    void on_btnKZCL_clicked();
    void on_btnCMJX_clicked();
    void on_btnZXTY_clicked();
    void on_btn500GC_clicked();
    void on_btnHXGD_clicked();
    void on_btnJGGD_clicked();
    void on_btnFZ_clicked();
    void on_btnDXGD_clicked();
    void on_btnDWPD_clicked();
    void on_btnZYCL_clicked();
    void on_btnDXPD_clicked();
    void on_btnMeasure_clicked();

    void on_btn_ZXTY_param_clicked();

    void on_btnZXTY_OK_clicked();
    //处理系统配置中的时间问题
    void on_btnSC_year_clicked();
    void on_btnSC_month_clicked();
    void on_btnSC_day_clicked();
    void on_btnSC_hour_clicked();
    void on_btnSC_min_clicked();
    void on_btnSC_second_clicked();

    //保存时间按钮
    void on_btnSC_save_time_clicked();

    //L1~L5的输入处理
    void on_btnSC_L1_clicked();
    void on_btnSC_L2_clicked();
    void on_btnSC_L3_clicked();
    void on_btnSC_L4_clicked();
    void on_btnSC_L5_clicked();
    //L保存的处理
    void on_btnSC_SaveL_clicked();
    //角度和倾角配置
    //void on_btnSC_JDConfig_clicked();
    //void on_btnSC_QJConfig_clicked();

    //光标位置的调整
    void on_btnSY_up_clicked();
    void on_btnSY_left_clicked();
    void on_btnSY_right_clicked();
    void on_btnSY_down_clicked();
    //测量模式设置
    void on_cbSC_clms_clicked();
    void on_cbSC_jlms_clicked();
    void on_btnLaser_clicked();

    //对锚段选择的操作
    void on_cbMS_xl_currentIndexChanged(const QString &arg1);
    void on_cbMS_xj_currentIndexChanged(const QString &arg1);
    void on_cbMS_cj_currentIndexChanged(const QString &arg1);
    void on_cbMS_gq_currentIndexChanged(const QString &arg1);
    void on_cbMS_zq_currentIndexChanged(const QString &arg1);
    void on_cbMS_md_currentIndexChanged(const QString &arg1);

    //上下某段切换
    void on_btnNextM_clicked();
    void on_btnLastM_clicked();

    void on_btnViewData_clicked();

    void on_btnDV_Back_clicked();

    void on_btn_listView_clicked();

    void on_btn_plotView_clicked();

    void on_slr_sm_moto_valueChanged(int value);

    void on_slr_big_moto_valueChanged(int value);

    void on_btn_left_clicked();

    void on_btn_left_small_clicked();

    void on_btn_right_clicked();

    void on_btn_right_small_clicked();


    void on_btnPDJDOK_clicked();
    void on_btnSC_JDQJConfig_clicked();

    void on_btn_AC_left_clicked();

    void on_btnACLaser_clicked();

    void on_btn_AC_right_clicked();

    void on_btn_AC_left_small_clicked();

    void on_btn_AC_right_small_clicked();

    void on_btn_AC_Back_clicked();

    void on_btnSC_JDConfig_2_clicked();

    void on_btnSC_QJConfig_2_clicked();

    void on_btnConfigMeasure_clicked();
    void on_btnSC_jd_clear_2_clicked();
    void on_btnSC_qj_clear_2_clicked();
#if warning_signal
    //角度和倾角清0设置
    void on_btnSC_jd_clear_clicked();
    void on_btnSC_qj_clear_clicked();
    void on_btnACLaser_2_clicked();
#endif
    void on_btn_left_pressed();

    void on_btn_left_released();

    void on_btn_right_pressed();

    void on_btn_right_released();

    void on_btn_left_small_pressed();

    void on_btn_right_small_released();

    void on_btn_left_small_released();

    void on_btn_right_small_pressed();

    void on_btn_AC_left_pressed();

    void on_btn_AC_left_released();

    void on_btn_AC_right_pressed();

    void on_btn_AC_right_released();

    void on_btn_AC_left_small_pressed();

    void on_btn_AC_left_small_released();

    void on_btn_AC_right_small_pressed();

    void on_btn_AC_right_small_released();

    void on_slr_touch_scale_valueChanged(int value);

    void on_slr_moto_step_valueChanged(int value);


    void on_edt_PDJD_JL_clicked();

    void on_cbMS_xl_editTextChanged(const QString &arg1);

    void on_cbMS_xj_editTextChanged(const QString &arg1);

    void on_cbMS_cj_editTextChanged(const QString &arg1);

    void on_cbMS_gq_editTextChanged(const QString &arg1);

    void on_cbMS_zq_editTextChanged(const QString &arg1);

    void on_btnDataExport_clicked();

    void on_cmbFilter_currentTextChanged(const QString &arg1);

    void on_btn_sm_sub_clicked();

    void on_btn_sm_add_clicked();

    void on_btn_big_sub_clicked();

    void on_btn_big_add_clicked();

    void on_btn_touch_sub_clicked();

    void on_btn_touch_add_clicked();

    void on_btn_moto_step_sub_clicked();

    void on_btn_moto_step_add_clicked();

    void on_btnRemoteUpdate_clicked();

    void on_btnKJ_clicked();

    void on_btnGPS_clicked();
    void slot_returnMainUI(void);
    void slot_drawGps(GPSInfo &gpsValue);
    void on_chbGps_clicked();

private:
    Ui::MainWidget *ui;
    NetConnection m_connection;
    QImage* m_image;
    boost::mutex m_img_mutex;
    QTimer m_update_timer;
    //缓存串口数据
    std::list<char> m_wait_for_read;
    boost::unordered_map<std::string/*cmd_name*/, unsigned /*param_count*/> m_cmd_dic;//命令同参数个数的对应
    VirtualKeyBoard m_key_board;
    //用于判断当前是否正在加载线路列表
    bool m_is_load_xl;
    QwtPlotCurve* curve_lcz;
    QwtPlotCurve* curve_dg;
    std::vector<QwtPlotMarker *> m_maker_list;
    float m_dxpd_dg1;//导线坡度第一次导高值
    float m_dxpd_dg2;//导线坡度第二次导高值
    QTimer m_moto_timer;
    bool m_b_moto_direct;
    bool m_b_moto_small;
    QMap<QString, QMap<QString, QMap<QString, QMap<QString, QStringList> > > >  m_filed_map;


    //远程数据同步
    void connect2MysqlDb();
    QSqlDatabase db_mysql;
    bool bRemoteDb;     //远程数据库使用标志位
    bool remoteDbStatus;
    bool insertRemoteDb(QStringList);
    bool getRemoteDatas(QStringList &);
    void updateRemoteSet(void);
    QStringList alreadyTms;//利用时间判别数据是否重复
    //gps数据记录
    curveInfo add1Curve(QCustomPlot *drawWire,QColor color);
    curveInfo gpsDraw;       //3
    SerialPort *gpsCom;
    double maxLongitude;
    double minLongitude;
    double maxLatitude;
    double minLatitude;
    void setDeviceStatus(QLabel *lbl,bool flag);
    void gpsInitial(void);
    float first_dwpd1 ;
    float first_dwjd1 ;
};

#endif // MAINWIDGET_H
