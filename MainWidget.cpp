#include "MainWidget.h"
#include "ui_MainWidget.h"
#include <QEvent>
#include <QPainter>
#include <QDebug>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include "Config.h"
#define DB_File "./data.db"
#include <QFile>
#include <boost/thread.hpp>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTime>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_legend.h>
#include <qwt_point_data.h>
#include <qwt_symbol.h>
#include <qwt_plot_marker.h>
#include <QDir>
#include <QMessageBox>
#include <QDateTime>
const QString UPath="E:/";
//定义按键插入的回调函数的宏

#define DefKeyInsertPushButtonCallBack(name) \
    void OnInsert##name##CallBack(QPushButton* widget,QString key){\
    if(widget->objectName() == "btnSC_L1" ||\
    widget->objectName() == "btnSC_L2" ||\
    widget->objectName() == "btnSC_L3" ||\
    widget->objectName() == "btnSC_L4" ||\
    widget->objectName() == "btnSC_L5")\
{\
    if((widget->text()+key).toInt()>=20000)\
{\
    return;\
    }\
    }\
    widget->setText(widget->text()+key);\
    }
#define DefKeyDeletePushButtonCallBack(name) \
    void OnDelete##name##CallBack(QPushButton* widget){\
    widget->setText(widget->text().left(widget->text().length()-1));\
    }
//定义回调绑定的宏
#define KeyBindCallBack(name) \
    m_key_board.Init(boost::bind(OnInsert##name##CallBack, ui->##name,_1), boost::bind(OnDelete##name##CallBack, ui->##name))

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWidget),
    m_image(NULL)
{
    GetSqliteField();
    ui->setupUi(this);
    InitCMDDic();
    ui->StackedMain->setCurrentIndex(MPT_Main);
    ui->wgtCamara->installEventFilter(this);
    ui->wgtCamara_Config->installEventFilter(this);
    ui->wgtCamara_ac_Config->installEventFilter(this);
    m_connection.SetOnGetMsgCallBack(boost::bind(&MainWidget::OnReadNetMessage, this, _1));
    connect(&m_update_timer, SIGNAL(timeout()), this, SLOT(OnUpdateTimer()));
    qRegisterMetaType<SerialItem>("SerialItem");
    connect(this, SIGNAL(DoGetInfo(SerialItem)), this, SLOT(OnGetInfo(SerialItem)));
    qRegisterMetaType<boost::shared_ptr<MessageStu> >("boost::shared_ptr<MessageStu>");
    connect(this, SIGNAL(DoGetNetMsg(boost::shared_ptr<MessageStu>)), this, SLOT(OnGetNetMsg(boost::shared_ptr<MessageStu>)));
    m_update_timer.start(100);
    ResetAllMeasureLabel();
    //ShowMain(false);
    ShowMain(true);
    ui->edtMain->appendPlainText(QString::fromWCharArray(L"正在连接测量仪..."));
    QTimer::singleShot(1000, this, SLOT(Connect()));



    curve_lcz=new QwtPlotCurve("");
    curve_lcz->setStyle(QwtPlotCurve::Lines);//直线形式
    curve_lcz->setCurveAttribute(QwtPlotCurve::Fitted, true);//是曲线更光滑
    curve_lcz->setPen(QPen(Qt::blue));//设置画笔
    curve_lcz->attach(ui->qwtPlot_lcz);//把曲线附加到plot上


    curve_dg=new QwtPlotCurve("");
    curve_dg->setStyle(QwtPlotCurve::Lines);//直线形式
    curve_dg->setCurveAttribute(QwtPlotCurve::Fitted, true);//是曲线更光滑
    curve_dg->setPen(QPen(Qt::blue));//设置画笔
    curve_dg->attach(ui->qwtPlot_dg);//把曲线附加到plot上
    m_moto_timer.setInterval(Config::GetInstance()->m_moto_config.m_moto_run_step);
    connect(&m_moto_timer, SIGNAL(timeout()), this, SLOT(OnMotoTimerOut()));

    ui->wgtDataView->setSelectionMode(QAbstractItemView::SingleSelection);//单击选中整行
    ui->wgtDataView->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->label->hide();
    ui->cbMS_xl->hide();
    ui->label_7->hide();
    ui->cbMS_xj->hide();
    ui->label_3->hide();
    ui->cbMS_cj->hide();
    ui->label_4->hide();
    ui->cbMS_gq->hide();
    //隐藏中心投影
    ui->lblZXTY_ZXTY->setVisible(false);
    ui->label_46->setVisible(false);
    ui->label_47->setVisible(false);
    //将所有返回按钮集中到一个槽函数
    connect(ui->btnMenuBack,SIGNAL(clicked()),this,SLOT(slot_returnMainUI()));
    connect(ui->btnSC_back,SIGNAL(clicked()),this,SLOT(slot_returnMainUI()));
    connect(ui->btnDM_back,SIGNAL(clicked()),this,SLOT(slot_returnMainUI()));
#if neijiang_modify
    connect(ui->btn_GPS_Back,SIGNAL(clicked()),this,SLOT(slot_returnMainUI()));
    updateRemoteSet();
    gpsInitial();
#else
    ui->btnGPS->setVisible(false);
    ui->btnRemoteUpdate->setVisible(false);
    ui->widget_KJ2_3->setVisible(false);
    ui->lblRemoteDb->setVisible(false);
    ui->btnKJ->setVisible(false);
    ui->btn_plotView->setVisible(false);
#endif
    ui->StackedMain->move(0,0);
    ui->StackedMain->resize(this->size());
}
MainWidget::~MainWidget()
{
    delete ui;
}
void MainWidget::on_btnExit_clicked()
{
    //this->RunSqlWithNet(QString::fromWCharArray(L"delete from import where 序号=492692"));
    exit(0);
}
void MainWidget::on_btnMainMeasure_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_JBCL);
    ui->btnJBCL->setChecked(true);
    ui->StackedMain->setCurrentIndex(MPT_BaseMeasure);
}

void MainWidget::on_btnMainDataManager_clicked()
{
    ui->StackedMain->setCurrentIndex(MPT_DataManager);

    //设置筛选框内容
    ui->cmbFilter->clear();
    ui->cmbFilter->blockSignals(true);
    ui->cmbFilter->addItems(m_filed_map[QString::fromWCharArray(L"临时线路")][QString::fromWCharArray(L"临时行间")][QString::fromWCharArray(L"临时车间")][QString::fromWCharArray(L"临时工区")]);
    ui->cmbFilter->blockSignals(false);
    FlushDataView();
}

void MainWidget::on_btnMainSystemConfig_clicked()
{
    InitSystemConfigPage();
    ui->StackedMain->setCurrentIndex(MPT_SystemConfig);
}

int MainWidget::convert_yuv_to_rgb_pixel(int y, int u, int v)
{
    unsigned int pixel32 = 0;
    unsigned char *pixel = (unsigned char *)&pixel32;
    int r, g, b;
    r = y + (1.370705 * (v-128));
    g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
    b = y + (1.732446 * (u-128));
    if(r > 255) r = 255;
    if(g > 255) g = 255;
    if(b > 255) b = 255;
    if(r < 0) r = 0;
    if(g < 0) g = 0;
    if(b < 0) b = 0;
    pixel[0] = r * 220 / 256;
    pixel[1] = g * 220 / 256;
    pixel[2] = b * 220 / 256;
    return pixel32;
}

int MainWidget::convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
    unsigned int in, out = 0;
    unsigned int pixel_16;
    unsigned char pixel_24[3];
    unsigned int pixel32;
    int y0, u, y1, v;
    for(in = 0; in < width * height * 2; in += 4) {
        pixel_16 =
                yuv[in + 3] << 24 |
                               yuv[in + 2] << 16 |
                                              yuv[in + 1] <<  8 |
                                                              yuv[in + 0];
        y0 = (pixel_16 & 0x000000ff);
        u  = (pixel_16 & 0x0000ff00) >>  8;
        y1 = (pixel_16 & 0x00ff0000) >> 16;
        v  = (pixel_16 & 0xff000000) >> 24;
        pixel32 = convert_yuv_to_rgb_pixel(y0, u, v);
        pixel_24[0] = (pixel32 & 0x000000ff);
        pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
        pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
        rgb[out++] = pixel_24[0];
        rgb[out++] = pixel_24[1];
        rgb[out++] = pixel_24[2];
        pixel32 = convert_yuv_to_rgb_pixel(y1, u, v);
        pixel_24[0] = (pixel32 & 0x000000ff);
        pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
        pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
        rgb[out++] = pixel_24[0];
        rgb[out++] = pixel_24[1];
        rgb[out++] = pixel_24[2];
    }
    return 0;
}
void MainWidget::AnlyzeReadBuffer()
{
    int flag_len = 0;//连续#的个数
    std::string str_cmd;
    for(std::list<char>::iterator iter = m_wait_for_read.begin(); iter != m_wait_for_read.end(); iter++)
    {
        if(*iter == '#')
        {
            flag_len++;
        }
        else
        {
            flag_len = 0;
        }
        if(flag_len == 2)
        {
            iter++;
            str_cmd.assign(m_wait_for_read.begin(), iter);
            //            qDebug()<<"rec data"<<str_cmd.c_str();
            if(str_cmd.find('1') == 2)
            {
                int ddd = 0;
                ddd = 1;
            }
            m_wait_for_read.erase(m_wait_for_read.begin(), iter);
            //解析
            SerialItem item;
            if(StringToSerialItem(str_cmd, item))
            {//解析成功，发出去
                DoGetInfo(item);
            }
            flag_len = 0;
            break;
        }
    }
}
bool MainWidget::StringToSerialItem(const std::string &str, SerialItem &item)
{
    std::string str_cmd;// = str;
    int idx = str.find("R");
    if(idx == -1)
        return false;
    str_cmd = &str.c_str()[idx];
    item.m_ori_str = str_cmd;
    if(str_cmd.length() < 2)return false;
    //过滤掉最右侧2个#
    str_cmd = str_cmd.substr(0, str_cmd.length()-2);
    //获取命令
    if(str_cmd.length() < 4)return false;
    item.m_cmd = str_cmd.substr(0,4);
    str_cmd = str_cmd.substr(4);
    if(str_cmd.length() == 0)
    {//沒有则直接返回
        return true;
    }
    //获取参数
    std::string str_tmp;
    bool is_plus = false;
    for(int i = 0;;i++)
    {
        str_tmp = str_cmd.substr(0,2);
        if(str_cmd.length()>8)
        {//参数
            //正负号判断
            if(str_cmd.at(0) == '0')is_plus = true;
            else if(str_cmd.at(0) == '1') is_plus = false;
            else return false;
            //第几个参数
            if(i>=9)return false;
            if(boost::lexical_cast<std::string>(i+1) != (boost::format("%c")%str_cmd.at(1)).str())
            {
                return false;
            }
            item.m_param.push_back(
                        std::pair<int,int>(
                            boost::lexical_cast<int>(str_cmd.substr(0,2)),
                            boost::lexical_cast<int>(str_cmd.substr(2,6))*(is_plus?1:-1)
                            )
                        );
            str_cmd = str_cmd.substr(8);

        }
        else if(str_cmd.length() == 2)
        {//arm_linux校验
            item.m_check = boost::lexical_cast<int>(str_cmd);
            return CheckSerialItem(item);
        }
        else
        {//异常
            return false;
        }
    }
}

bool MainWidget::CheckSerialItem(const SerialItem &item)
{
    //判断校验正确性
    int check = 0;
    for(unsigned int i = 0; i < item.m_param.size(); i++)
    {
        check += item.m_param[i].first;
    }
    if(item.m_check != check)return false;
    //判断命令是能识别
    if(m_cmd_dic.find(item.m_cmd) == m_cmd_dic.end())
    {
        return false;
    }
    //判断命令参数个数是否相同
    if(m_cmd_dic[item.m_cmd] != item.m_param.size())
    {
        return false;
    }
    return true;
}

void MainWidget::InitCMDDic()
{
    m_cmd_dic["R001"] = 3;
    m_cmd_dic["R002"] = 2;
    m_cmd_dic["R003"] = 2;
    m_cmd_dic["R004"] = 1;
    m_cmd_dic["R005"] = 3;
    m_cmd_dic["R006"] = 2;
    m_cmd_dic["R007"] = 1;
    m_cmd_dic["R008"] = 1;
    m_cmd_dic["R009"] = 2;
    m_cmd_dic["R010"] = 1;
    m_cmd_dic["R011"] = 2;
    m_cmd_dic["R012"] = 2;
    m_cmd_dic["R013"] = 5;
    m_cmd_dic["R014"] = 1;
    m_cmd_dic["R114"] = 1;
    m_cmd_dic["R015"] = 1;
    m_cmd_dic["R115"] = 1;
    m_cmd_dic["R016"] = 1;
}

void MainWidget::ResetAllMeasureLabel()
{
    ui->lblJBCL_DGZ->setText("0");
    ui->lblJBCL_LCZ->setText("0");

    ui->lblKZCL_KZGD->setText("0");
    ui->lblKZCL_KZPY->setText("0");

    ui->lblCMJX_CMJX->setText("0");

    ui->lblZXTY_ZXTY->setText("0");
    ui->lblZXTY_NGJ->setText("0");
    ui->lblZXTY_PCZ->setText("0");
    ui->btn_ZXTY_param->setText("");
    ui->widgetZXTY_Param->show();
    ui->widgetZXTYMeasure->hide();

    ui->lbl_500_GC->setText("0");
    ui->lbl_500_JJ->setText("0");

    ui->lbl2Height->setText("0");
    ui->lbl2Width->setText("0");
#if gaocha500_2point
    ui->stackWdg500->setCurrentWidget(ui->pg2Point);
    ui->widget_6->hide();
    ui->wdg2Point1->show();
    ui->wdg2Point2->hide();
#else
    ui->stackWdg500->setCurrentWidget(ui->pg4Point);
    ui->widget_500_1->show();
    ui->widget_500_2->hide();
    ui->widget_500_3->hide();
    ui->widget_500_4->hide();
    ui->widget_500_5->hide();
#endif
    ui->lbl_HXGD_HXGD->setText("0");

    ui->widget_JGGD_1->show();
    ui->widget_JGGD_2->hide();
    ui->widget_JGGD_3->hide();

    ui->widget_FZ_1->show();
    ui->widget_FZ_2->hide();
    ui->widget_FZ_3->hide();

    ui->widget_SDXGD_1->show();
    ui->widget_SDXGD_2->hide();
    ui->widget_SDXGD_3->hide();

    ui->widget_DWPD_1->show();
    ui->widget_DWPD_2->hide();
    ui->widget_DWPD_3->hide();

    ui->widget_ZYCL_1->show();
    ui->widget_ZYCL_2->hide();
    ui->widget_ZYCL_3->hide();

    //ui->lblSC_JDConfig->hide();
    //ui->lblSC_QJConfig->hide();

    // 跨距测量
    ui->lblGJZ->setText("0");
    ui->lblJDZ->setText("0");
    ui->lblCGZ->setText("0");
    ui->lblKJ_L1->setText("0");
    ui->lblKJ_L2->setText("0");
    ui->lblKJ_Value->setText("0");
    ui->widget_KJ1->show();
    ui->widget_KJ2->hide();

    //导线坡度
    ui->lbl_PDJD_msg1->show();
    ui->lbl_PDJD_msg2->hide();
    ui->lblPDJD_PD->setText("");
    ui->wgtPDJDJJ->hide();
    ui->wgtPDMSG->hide();

}

void MainWidget::SendSerialMsg(const std::string& str_msg)
{
    boost::shared_ptr<MessageStu> msg = boost::shared_ptr<MessageStu>(new MessageStu());
    msg->m_id = 1;
    msg->m_len = str_msg.length();
    msg->m_msg = new char[msg->m_len];
    memcpy(msg->m_msg, str_msg.c_str(), msg->m_len);
    m_connection.Send(msg);
}

void MainWidget::NetRequestDateUpdata()
{
    boost::shared_ptr<MessageStu> msg = boost::shared_ptr<MessageStu>(new MessageStu());
    msg->m_id = 3;
    msg->m_len = 1;
    msg->m_msg = new char[1];
    m_connection.Send(msg);
}

void MainWidget::InitSystemConfigPage()
{
    SYSTEMTIME time;
    GetLocalTime(&time);
    ui->btnSC_year->setText(QString().sprintf("%04d",time.wYear));
    ui->btnSC_month->setText(QString().sprintf("%02d",time.wMonth));
    ui->btnSC_day->setText(QString().sprintf("%02d",time.wDay));
    ui->btnSC_hour->setText(QString().sprintf("%02d",time.wHour));
    ui->btnSC_min->setText(QString().sprintf("%02d",time.wMinute));
    ui->btnSC_second->setText(QString().sprintf("%02d",time.wSecond));

    SendSerialMsg("R013##");
    if(Config::GetInstance()->m_record_config.m_check_mode == RecordConfig::CM_Common)
    {
        ui->cbSC_clms->setChecked(true);
        ui->cbSC_jlms->setChecked(false);
    }
    else
    {
        ui->cbSC_clms->setChecked(false);
        ui->cbSC_jlms->setChecked(true);
    }

    ui->slr_sm_moto->setValue(Config::GetInstance()->m_moto_config.m_s_moto_speed);
    ui->slr_big_moto->setValue(Config::GetInstance()->m_moto_config.m_b_moto_speed);
    ui->slr_touch_scale->setValue(Config::GetInstance()->m_moto_config.m_touch_scale);
    ui->slr_moto_step->setValue(Config::GetInstance()->m_moto_config.m_moto_run_step);
}

void MainWidget::ShowMain(bool show)
{
    ui->btnMainDataManager->setVisible(show);
    ui->btnMainMeasure->setVisible(show);
    ui->btnMainSystemConfig->setVisible(show);
}

void MainWidget::InitField()
{
    /*m_is_load_xl = true;
    ui->cbMS_xl->clear();
    QStringList list_for_push = GetSqliteField();
    ui->cbMS_xl->addItems(list_for_push);
    m_is_load_xl = false;*/
    m_filed_map.clear();
    QFile file(QString::fromWCharArray(L"线路.csv"));
    if(file.open(QFile::ReadOnly))
    {
        QTextStream stream(&file);
        while(!stream.atEnd())
        {
            QString str_line = stream.readLine();
            QStringList str_list = str_line.split(",");
            if(str_list.size() == 5)
            {
                m_filed_map[str_list[0]][str_list[1]][str_list[2]][str_list[3]].push_back(str_list[4]) ;
            }

        }
        file.close();
    }
    ui->cbMS_xl->clear();
    ui->cbMS_xj->clear();
    ui->cbMS_cj->clear();
    ui->cbMS_gq->clear();
    ui->cbMS_zq->clear();
    ui->cbMS_md->clear();
    ui->cbMS_xl->addItems(m_filed_map.keys());
    //ui->cbMS_xj->addItems(m_filed_map[ui->cbMS_xl->currentText()].keys());
    //ui->cbMS_cj->addItems(m_filed_map[ui->cbMS_xl->currentText()][ui->cbMS_xj->currentText()].keys());
    //ui->cbMS_gq->addItems(m_filed_map[ui->cbMS_xl->currentText()][ui->cbMS_xj->currentText()][ui->cbMS_cj->currentText()].keys());
    //ui->cbMS_zq->addItems(m_filed_map[ui->cbMS_xl->currentText()][ui->cbMS_xj->currentText()][ui->cbMS_cj->currentText()][ui->cbMS_gq->currentText()]);

    for(int i = 0; i < 2000; i++)
    {
        ui->cbMS_md->addItem(QString().sprintf("%d", i));
    }

}

QStringList MainWidget::GetSqliteField(const QString &xl, const QString &xj, const QString &cj, const QString &gq, const QString &zq)
{

    if(xl == "")
    {
        return m_filed_map.keys();
    }
    else if(xj == "")
    {
        return m_filed_map[xl].keys();
    }
    else if(cj == "")
    {
        return m_filed_map[xl][xj].keys();
    }
    else if(gq == "")
    {
        return m_filed_map[xl][xj][cj].keys();
    }
    else if(zq == "")
    {
        return m_filed_map[xl][xj][cj][gq];
    }
    else
    {
        QStringList rtn_list;
        for(int i = 0; i < 2000; i++)
        {
            //ui->cbMS_md->addItem(QString().sprintf("%d", i));
            rtn_list.push_back(QString().sprintf("%d",i));
        }
        return rtn_list;
    }
}

QStringList MainWidget::GetDataPathList()
{
    QStringList rtn_list;
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(DB_File);
    database.open();
    QString cmd_str = QString::fromWCharArray(L"select * from data group by 线路");
    QSqlQuery query = database.exec(cmd_str);
    while(query.next())
    {
        rtn_list.push_back(query.value(QString::fromWCharArray(L"线路")).toString());
    }
    database.close();
    return rtn_list;
}

QStringList MainWidget::GetXLPathList()
{
    QStringList rtn_list;
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(DB_File);
    database.open();
    QString cmd_str = QString::fromWCharArray(L"select * from import group by 路线");
    QSqlQuery query = database.exec(cmd_str);
    while(query.next())
    {
        rtn_list.push_back(query.value(QString::fromWCharArray(L"路线")).toString());
    }
    database.close();
    return rtn_list;
}

void MainWidget::RunSqlWithNet(const QString &cmd)
{
    boost::shared_ptr<MessageStu> msg = boost::shared_ptr<MessageStu>(new MessageStu());
    msg->m_id = 4;
    QByteArray utf8_array = cmd.toUtf8();
    msg->m_len = utf8_array.length();
    msg->m_msg = new char[msg->m_len];
    memcpy(msg->m_msg, utf8_array.data(), msg->m_len);
    m_connection.Send(msg);
}

void MainWidget::InitDataManager()
{

}

void MainWidget::WriteMeasureData(QString str_cmd)
{
    if(Config::GetInstance()->m_record_config.m_check_mode == RecordConfig::CM_Common)return;
    QFile file(QString::fromWCharArray(L"./data.csv"));
    if(file.open(QFile::WriteOnly|QFile::Append))
    {
        QString title;
        QString data;
        //                                  1     2   3    4   5   6    7    8   9
        title = QString::fromWCharArray(L"测量时间,线路,行间,车间,工区,站区,支柱号,轨距,超高");
        data += QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"); //1
        data += ","+ui->cbMS_xl->currentText(); //2
        data += ","+ui->cbMS_xj->currentText(); //3
        data += ","+ui->cbMS_cj->currentText(); //4
        data += ","+ui->cbMS_gq->currentText(); //5
        data += ","+ui->cbMS_zq->currentText(); //6
        data += ","+ui->cbMS_md->currentText(); //7
        data += ","+ui->lblGJZ->text();    //8
        data += ","+ui->lblCGZ->text();    //9
        if(str_cmd == "R002")
        {
            title += ","+QString::fromWCharArray(L"拉出值");
            title += ","+QString::fromWCharArray(L"导高值");
            data += ","+ui->lblJBCL_LCZ->text();
            data += ","+ui->lblJBCL_DGZ->text();
        }
        else if(str_cmd == "R003")
        {
            title += ","+QString::fromWCharArray(L"跨中高度");
            title += ","+QString::fromWCharArray(L"跨中偏移");
            data += ","+ui->lblKZCL_KZPY->text();
            data += ","+ui->lblKZCL_KZGD->text();
        }
        else if(str_cmd == "R004")
        {
            title += ","+QString::fromWCharArray(L"侧面界限");
            data += ","+ui->lblCMJX_CMJX->text();
        }
        else if(str_cmd == "R005")
        {
            title += ","+QString::fromWCharArray(L"内轨距");
            title += ","+QString::fromWCharArray(L"偏差值");
            data += ","+ui->lblZXTY_ZXTY->text();
            data += ","+ui->lblZXTY_NGJ->text();
            data += ","+ui->lblZXTY_PCZ->text();
        }
        else if(str_cmd == "R006")
        {
            title += ","+QString::fromWCharArray(L"500高差");
            title += ","+QString::fromWCharArray(L"500间距");
            if(ui->stackWdg500->currentWidget()==ui->pg4Point)
            {
                data += ","+ui->lbl_500_GC->text();
                data += ","+ui->lbl_500_JJ->text();
            }
            else if(ui->stackWdg500->currentWidget()==ui->pg2Point)
            {
                data += ","+ui->lbl2Width->text();
                data += ","+ui->lbl2Height->text();
            }
        }
        else if(str_cmd == "R007")
        {
            title += ","+QString::fromWCharArray(L"红线高度");
            data += ","+ui->lbl_HXGD_HXGD->text();
        }
        else if(str_cmd == "R008")
        {
            title += ","+QString::fromWCharArray(L"结构高度");
            data += ","+ui->lbl_JGGD_JGGD->text();
        }
        else if(str_cmd == "R009")
        {
            title += ","+QString::fromWCharArray(L"非支间距");
            title += ","+QString::fromWCharArray(L"非支高度");
            data += ","+ui->lbl_FZ_JJ->text();
            data += ","+ui->lbl_FZ_GD->text();
        }
        else if(str_cmd == "R010")
        {
            title += ","+QString::fromWCharArray(L"输电线高差");
            data += ","+ui->lbl_SDXGD_SDXGC->text();
        }
        else if(str_cmd == "R011")
        {
            title += ","+QString::fromWCharArray(L"定位器坡度");
            title += ","+QString::fromWCharArray(L"定位器角度");
            data += ","+ui->lbl_DWPD_PD->text();
            data += ","+ui->lbl_DWPD_JD->text();
        }
        else if(str_cmd == "R012")
        {
            title += ","+QString::fromWCharArray(L"水平间距");
            title += ","+QString::fromWCharArray(L"垂直间距");
            title += ","+QString::fromWCharArray(L"直线距离");
            title += ","+QString::fromWCharArray(L"垂直角度");
            data += ","+ui->lbl_ZYCL_SPJJ->text();
            data += ","+ui->lbl_ZYCL_CZJJ->text();
            data += ","+ui->lbl_ZYCL_ZXJL->text();
            data += ","+ui->lbl_ZYCL_CZJD->text();
        }
        else if(str_cmd == "xxx")
        {
            title += ","+QString::fromWCharArray(L"导线坡度");
            data += ","+ui->lblPDJD_PD->text();
        }
        title += "\n";
        data += "\n";
        file.write(title.toLocal8Bit());
        file.write(data.toLocal8Bit());
        file.close();
    }
}

void MainWidget::ControlMoto(bool direct, bool small_control)
{
    boost::shared_ptr<MessageStu> msg = boost::shared_ptr<MessageStu>(new MessageStu());
    msg->m_id = 8;
    msg->m_len = 5;
    msg->m_msg = new char[5];
    msg->m_msg[0] = direct?0:1;
    *((int*)&msg->m_msg[1]) = small_control?(Config::GetInstance()->m_moto_config.m_s_moto_speed):(Config::GetInstance()->m_moto_config.m_b_moto_speed);
    //memcpy(msg->m_msg, str_msg.c_str(), msg->m_len);
    m_connection.Send(msg);
}

void MainWidget::InitDataView(QString xl_name)
{
    ui->btn_listView->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);
    {//删除表格所有行
        int iLen = ui->tbWidget->rowCount();
        for(int i=0;i<iLen;i++)
        {
            ui->tbWidget->removeRow(0);
        }
    }
    QwtPlot* qwtPlot_lcz = ui->qwtPlot_lcz;
    QwtPlot* qwtPlot_dg = ui->qwtPlot_dg;
    qwtPlot_lcz->insertLegend(new QwtLegend(), QwtPlot::RightLegend);
    qwtPlot_dg->insertLegend(new QwtLegend(), QwtPlot::RightLegend);
    (void) new QwtPlotMagnifier( qwtPlot_lcz->canvas() );
    (void) new QwtPlotPanner( qwtPlot_lcz->canvas() );
    (void) new QwtPlotMagnifier( qwtPlot_dg->canvas() );
    (void) new QwtPlotPanner( qwtPlot_dg->canvas() );
    QVector<double> xs_lcz,xs_dg;
    QVector<double> ys_lcz,ys_dg;

    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(DB_File);
    if(!database.open())
    {
        return;
    }
    QString str_cmd = QString::fromWCharArray(L"select * from data where 线路='%1'").arg(xl_name);
    QSqlQuery query = database.exec(str_cmd);

    int cout_idx = 0;
    while(query.next())
    {
        int idx = ui->tbWidget->rowCount();
        ui->tbWidget->insertRow(ui->tbWidget->rowCount());
        ui->tbWidget->setItem(idx, 0, new QTableWidgetItem(query.value(QString::fromWCharArray(L"线路")).toString()));
        ui->tbWidget->setItem(idx, 1, new QTableWidgetItem(query.value(QString::fromWCharArray(L"行别")).toString()));
        ui->tbWidget->setItem(idx, 2, new QTableWidgetItem(query.value(QString::fromWCharArray(L"车间")).toString()));
        ui->tbWidget->setItem(idx, 3, new QTableWidgetItem(query.value(QString::fromWCharArray(L"工区")).toString()));
        ui->tbWidget->setItem(idx, 4, new QTableWidgetItem(query.value(QString::fromWCharArray(L"站区")).toString()));

        ui->tbWidget->setItem(idx, 5, new QTableWidgetItem(query.value(QString::fromWCharArray(L"支柱号")).toString()));
        QString date = query.value(QString::fromWCharArray(L"测量日期")).toString();
        QString time = query.value(QString::fromWCharArray(L"测量时间")).toString();
        ui->tbWidget->setItem(idx, 6, new QTableWidgetItem(date));
        ui->tbWidget->setItem(idx, 7, new QTableWidgetItem(time));
        ui->tbWidget->setItem(idx, 8, new QTableWidgetItem(query.value(QString::fromWCharArray(L"轨距")).toString()));
        ui->tbWidget->setItem(idx, 9, new QTableWidgetItem(query.value(QString::fromWCharArray(L"超高")).toString()));

        ui->tbWidget->setItem(idx, 10, new QTableWidgetItem(query.value(QString::fromWCharArray(L"角度")).toString()));
        ui->tbWidget->setItem(idx, 11, new QTableWidgetItem(query.value(QString::fromWCharArray(L"拉出值")).toString()));
        ui->tbWidget->setItem(idx, 12, new QTableWidgetItem(query.value(QString::fromWCharArray(L"导高")).toString()));
        ui->tbWidget->setItem(idx, 13, new QTableWidgetItem(query.value(QString::fromWCharArray(L"跨中偏离")).toString()));
        ui->tbWidget->setItem(idx, 14, new QTableWidgetItem(query.value(QString::fromWCharArray(L"跨中高度")).toString()));

        ui->tbWidget->setItem(idx, 15, new QTableWidgetItem(query.value(QString::fromWCharArray(L"侧面界限")).toString()));
        ui->tbWidget->setItem(idx, 16, new QTableWidgetItem(query.value(QString::fromWCharArray(L"中心投影")).toString()));
        ui->tbWidget->setItem(idx, 17, new QTableWidgetItem(query.value(QString::fromWCharArray(L"内轨距")).toString()));
        ui->tbWidget->setItem(idx, 18, new QTableWidgetItem(query.value(QString::fromWCharArray(L"偏差值")).toString()));
        ui->tbWidget->setItem(idx, 19, new QTableWidgetItem(query.value(QString::fromWCharArray(L"高差")).toString()));

        ui->tbWidget->setItem(idx, 20, new QTableWidgetItem(query.value(QString::fromWCharArray(L"间距")).toString()));
        ui->tbWidget->setItem(idx, 21, new QTableWidgetItem(query.value(QString::fromWCharArray(L"红线高度")).toString()));
        ui->tbWidget->setItem(idx, 22, new QTableWidgetItem(query.value(QString::fromWCharArray(L"结构高度")).toString()));
        ui->tbWidget->setItem(idx, 23, new QTableWidgetItem(query.value(QString::fromWCharArray(L"接触线")).toString()));
        ui->tbWidget->setItem(idx, 24, new QTableWidgetItem(query.value(QString::fromWCharArray(L"非支间距")).toString()));
        ui->tbWidget->setItem(idx, 25, new QTableWidgetItem(query.value(QString::fromWCharArray(L"非支高度")).toString()));

        ui->tbWidget->setItem(idx, 26, new QTableWidgetItem(query.value(QString::fromWCharArray(L"输电线高差")).toString()));
        ui->tbWidget->setItem(idx, 27, new QTableWidgetItem(query.value(QString::fromWCharArray(L"定位坡度")).toString()));
        ui->tbWidget->setItem(idx, 28, new QTableWidgetItem(query.value(QString::fromWCharArray(L"坡度角度")).toString()));
        ui->tbWidget->setItem(idx, 29, new QTableWidgetItem(query.value(QString::fromWCharArray(L"水平间距")).toString()));
        ui->tbWidget->setItem(idx, 30, new QTableWidgetItem(query.value(QString::fromWCharArray(L"垂直间距")).toString()));

        ui->tbWidget->setItem(idx, 31, new QTableWidgetItem(query.value(QString::fromWCharArray(L"直线间距")).toString()));
        ui->tbWidget->setItem(idx, 32, new QTableWidgetItem(query.value(QString::fromWCharArray(L"垂直角度")).toString()));

        QString lcz = query.value(QString::fromWCharArray(L"拉出值")).toString();
        QString dg = query.value(QString::fromWCharArray(L"导高")).toString();
        if(lcz !="")
        {
            xs_lcz.push_back(cout_idx);
            ys_lcz.push_back(lcz.toDouble());
            {//绘制标记
                QwtPlotMarker *mY = new QwtPlotMarker();
                mY->setLabel(QString::fromWCharArray(L"支柱:\r\n")+query.value(QString::fromWCharArray(L"支柱号")).toString());
                mY->setLabelAlignment(Qt::AlignHCenter|Qt::AlignBottom);
                mY->setLineStyle(QwtPlotMarker::NoLine);
                mY->setXValue(cout_idx);
                mY->setYValue(lcz.toDouble());
                mY->attach(qwtPlot_lcz);
                m_maker_list.push_back(mY);
            }

        }
        if(dg != "")
        {
            xs_dg.push_back(cout_idx);
            ys_dg.push_back(dg.toDouble());
            {//绘制标记
                QwtPlotMarker *mY = new QwtPlotMarker();
                mY->setLabel(QString::fromWCharArray(L"支柱:")+query.value(QString::fromWCharArray(L"支柱号")).toString());
                mY->setLabelAlignment(Qt::AlignHCenter|Qt::AlignBottom);
                mY->setLineStyle(QwtPlotMarker::NoLine);
                mY->setXValue(cout_idx);
                mY->setYValue(dg.toDouble());
                mY->attach(qwtPlot_dg);
                m_maker_list.push_back(mY);
            }
            cout_idx++;
        }




    }


    QwtPointArrayData* data_lcz = new QwtPointArrayData(xs_lcz, ys_lcz);
    QwtPointArrayData* data_dg = new QwtPointArrayData(xs_dg, ys_dg);

    curve_lcz->setData(data_lcz);//设置数据
    curve_dg->setData(data_dg);//设置数据

    qwtPlot_lcz->replot();
    qwtPlot_dg->replot();

    database.close();
}

bool MainWidget::GetDiskUIsExist()
{
    QDir dir(UPath);
    if(dir.exists())
    {
        return true;
    }
    return false;
}

void MainWidget::AddCurrentTextToComb(QComboBox *cmb)
{
    bool finded = false;
    for(int i = 0; i < cmb->count(); i++)
    {
        if(cmb->itemText(i) == cmb->currentText())
        {
            finded = true;
            break;
        }
    }
    if(!finded)
    {
        cmb->addItem(cmb->currentText());
        cmb->setCurrentIndex(cmb->count()-1);
    }
}

bool MainWidget::IsStringInComb(QComboBox *cmb, QString str)
{
    bool finded = false;
    for(int i = 0; i < cmb->count(); i++)
    {
        if(cmb->itemText(i) == str)
        {
            finded = true;
            break;
        }
    }
    if(!finded)
    {
        return false;
    }
    return true;
}

void MainWidget::FlushDataView()
{//更新展示数据
    QString xl_name = ui->cmbFilter->currentText();
    while(ui->wgtDataView->rowCount())ui->wgtDataView->removeRow(0);
    QFile file(QString::fromWCharArray(L"./data.csv"));
    QVector<QMap<QString, QString> > record;
    if(file.open(QFile::ReadOnly))
    {
        QTextStream stream(&file);

        while(!stream.atEnd())
        {
            QString str_line1 = stream.readLine();
            str_line1.left(str_line1.length()-1);
            QStringList str_list1 = str_line1.split(",");

            QString str_line2 = stream.readLine();
            str_line2.left(str_line2.length()-1);
            QStringList str_list2 = str_line2.split(",");


            QMap<QString, QString> one_map;
            for(int i = 0; i < str_list1.count(); i++)
            {
                one_map[str_list1[i]] = str_list2[i];
            }
            record.push_back(one_map);
        }
        file.close();
    }
    QStringList all_title;
    all_title
            <<QString::fromWCharArray(L"测量时间")
           <<QString::fromWCharArray(L"站区")
          <<QString::fromWCharArray(L"支柱号")
         <<QString::fromWCharArray(L"轨距")
        <<QString::fromWCharArray(L"超高")
       <<QString::fromWCharArray(L"拉出值")
      <<QString::fromWCharArray(L"导高值")
     <<QString::fromWCharArray(L"跨中高度")
    <<QString::fromWCharArray(L"跨中偏移")
    <<QString::fromWCharArray(L"侧面界限")
    <<QString::fromWCharArray(L"中心投影")
    <<QString::fromWCharArray(L"内轨距")
    <<QString::fromWCharArray(L"偏差值")
    <<QString::fromWCharArray(L"500高差")
    <<QString::fromWCharArray(L"500间距")
    <<QString::fromWCharArray(L"红线高度")
    <<QString::fromWCharArray(L"结构高度")
    <<QString::fromWCharArray(L"非支间距")
    <<QString::fromWCharArray(L"非支高度")
    <<QString::fromWCharArray(L"定位器坡度")
    <<QString::fromWCharArray(L"定位器角度")
    <<QString::fromWCharArray(L"水平间距")
    <<QString::fromWCharArray(L"垂直间距")
    <<QString::fromWCharArray(L"直线距离")
    <<QString::fromWCharArray(L"垂直角度")
    <<QString::fromWCharArray(L"输电线高差")
    <<QString::fromWCharArray(L"导线坡度");
    QStringList title_list;
    for(int i = 0; i < all_title.size(); i++)
    {
        for(int j = 0; j < record.size(); j++)
        {
            if(record[j][QString::fromWCharArray(L"站区")] == xl_name && record[j].find(all_title[i]) != record[j].end())
            {
                title_list<<all_title[i];
                break;
            }
        }
    }
    {//在titlelist中增加单位
        QStringList title_list_tmp = title_list;
        for(int i = 0; i < title_list_tmp.size(); i++)
        {
            if(title_list_tmp[i] == QString::fromWCharArray(L"站区"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"线路");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"轨距"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"轨距(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"超高"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"超高(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"拉出值"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"拉出值(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"导高值"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"导高值(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"跨中高度"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"跨中高度(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"跨中偏移"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"跨中高度(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"侧面界限"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"侧面界限(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"中心投影"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"中心投影(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"内轨距"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"内轨距(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"偏差值"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"偏差值(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"500高差"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"500高差(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"500间距"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"500间距(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"红线高度"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"红线高度(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"结构高度"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"结构高度(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"非支间距"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"非支间距(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"非支高度"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"非支高度(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"定位器坡度"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"定位器坡度");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"定位器角度"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"定位器角度(°)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"水平间距"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"水平间距(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"垂直间距"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"垂直间距(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"直线距离"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"直线距离(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"垂直角度"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"垂直角度(°)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"输电线高差"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"输电线高差(mm)");
            }
            else if(title_list_tmp[i] == QString::fromWCharArray(L"导线坡度"))
            {
                title_list_tmp[i] = QString::fromWCharArray(L"导线坡度");
            }
        }
        ui->wgtDataView->setColumnCount(title_list_tmp.size());
        ui->wgtDataView->setHorizontalHeaderLabels(title_list_tmp);//设置表头内容
        ui->wgtDataView->setColumnWidth(0,130);
    }

    //ui->wgtDataView->setHorizontalHeaderLabels(title_list);
    for(int i = 0; i < record.size(); i++)
    {
        if(record[i][QString::fromWCharArray(L"站区")] == xl_name)
        {
            ui->wgtDataView->insertRow(ui->wgtDataView->rowCount());
            for(int j = 0; j < title_list.size(); j++)
            {
                if(record[i].find(title_list[j]) != record[i].end())
                {
                    ui->wgtDataView->setItem(ui->wgtDataView->rowCount()-1, j ,new QTableWidgetItem(record[i][title_list[j]]));
                }
            }
        }
    }
}

void MainWidget::Connect()
{
    if(m_connection.ConnectTo(""))
    {
        ShowMain(true);
        ui->edtMain->appendPlainText(QString::fromWCharArray(L"测量仪连接成功..."));
        ui->edtMain->setVisible(false);
    }
    else
    {
        ShowMain(false);
        ui->edtMain->appendPlainText(QString::fromWCharArray(L"测量仪连接失败..."));
    }
    //ui->edtMain->appendPlainText(QString::fromWCharArray(L"正在请求数据同步..."));

    //NetRequestDateUpdata();
    InitField();
    ShowMain(true);


}

void MainWidget::OnUpdateTimer()
{
    /*if(GetDiskUIsExist())
    {
        ui->lblDMUYes->show();
        ui->lblDMUNo->hide();
    }
    else
    {
        ui->lblDMUYes->hide();
        ui->lblDMUNo->show();
    }*/
    this->repaint();
}
void MainWidget::OnMotoTimerOut()
{
    ControlMoto(m_b_moto_direct, m_b_moto_small);
}

void MainWidget::MotoRun(int dis)
{
    boost::shared_ptr<MessageStu> msg = boost::shared_ptr<MessageStu>(new MessageStu());
    msg->m_id = 8;
    msg->m_len = 5;
    msg->m_msg = new char[5];
    msg->m_msg[0] = (dis<0)?0:1;
    *((int*)&msg->m_msg[1]) = (int)(qAbs(dis)*Config::GetInstance()->m_moto_config.m_touch_scale/10.0f);
    //memcpy(msg->m_msg, str_msg.c_str(), msg->m_len);
    m_connection.Send(msg);
}
void MainWidget::OnGetInfo(SerialItem item)
{
    if(item.m_cmd == "R001"){


        ui->lblJBCL_GJZ_2->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
        ui->lblJBCL_CGZ_2->setText(QString().sprintf("%.1f", item.m_param[1].second/10.0f));
        ui->lblJBCL_JDZ_2->setText(QString().sprintf("%.2f", item.m_param[2].second/100.0f));

        ui->lblGJZ->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
        ui->lblCGZ->setText(QString().sprintf("%.1f", item.m_param[1].second/10.0f));
        ui->lblJDZ->setText(QString().sprintf("%.2f", item.m_param[2].second/100.0f));
    }
    else if(item.m_cmd == "R002"){
        static int first_data = 0;
        if(ui->StackMeasure->currentIndex()==MPT_DXPD)
        {   //坡度测量时，特殊处理
            if(ui->lbl_PDJD_msg1->isVisible() == true||ui->wgtPDMSG->isVisible() == true)
            {//收到第一次测量值
                m_dxpd_dg1 = item.m_param[1].second;
                ui->lbl_PDJD_msg1->setVisible(false);
                ui->lbl_PDJD_msg2->setVisible(true);
                ui->wgtPDMSG->setVisible(false);
                ui->wgtPDMSG->setVisible(false);
            }
            else if(ui->lbl_PDJD_msg2->isVisible() == true)
            {//收到第二次测量值
                m_dxpd_dg2 = item.m_param[1].second;
                ui->lbl_PDJD_msg1->setVisible(false);
                ui->lbl_PDJD_msg2->setVisible(false);
                ui->wgtPDMSG->setVisible(false);
                ui->wgtPDJDJJ->setVisible(true);
                ui->edt_PDJD_JL->setText("");
            }
        }
        else if(ui->StackMeasure->currentIndex()==MPT_KJ)
        {
            //跨距测量
            if(ui->widget_KJ1->isVisible())
            {
                ui->widget_KJ1->setVisible(false);
                ui->widget_KJ2->setVisible(true);
                ui->lblKJ_L1->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
            }
            else if(ui->widget_KJ2->isVisible())
            {
                ui->widget_KJ1->setVisible(true);
                ui->widget_KJ2->setVisible(false);
                ui->lblKJ_L2->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
                ui->lblKJ_Value->setText(QString::number(abs(ui->lblKJ_L2->text().toFloat())+abs(ui->lblKJ_L1->text().toFloat())));
            }
        }
        else
        {
            ui->lblJBCL_LCZ->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
            ui->lblJBCL_DGZ->setText(QString().sprintf("%.1f", item.m_param[1].second/10.0f));
            ui->lblJBCL_LCZ_2->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
            ui->lblJBCL_DGZ_2->setText(QString().sprintf("%.1f", item.m_param[1].second/10.0f));
            WriteMeasureData("R002");//基本测量数据记录
        }
    }
    else if(item.m_cmd == "R003"){
        ui->lblKZCL_KZGD->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
        ui->lblKZCL_KZPY->setText(QString().sprintf("%.1f", item.m_param[1].second/10.0f));
        {//记录测量结果
            WriteMeasureData("R003");
        }
    }
    else if(item.m_cmd == "R004"){
        ui->lblCMJX_CMJX->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
        {//记录测量结果
            WriteMeasureData("R004");
        }
    }
    else if(item.m_cmd == "R005")
    {
        ui->lblZXTY_ZXTY->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
        ui->lblZXTY_NGJ->setText(QString().sprintf("%.1f", item.m_param[1].second/10.0f));
        ui->lblZXTY_PCZ->setText(QString().sprintf("%.1f", item.m_param[2].second/10.0f));
        {//记录测量结果
            WriteMeasureData("R005");
        }
    }
    else if(item.m_cmd == "R006")
    {
        //500高差
        static QPair<float, float> point[4];
#if gaocha500_2point
        if(ui->wdg2Point2->isVisible()==false)
        {
            ui->wdg2Point1->hide();
            ui->wdg2Point2->show();
            ui->widget_6->hide();
            point[0].first = item.m_param[1].second/10.0;//wire width
            point[0].second = item.m_param[0].second/10.0;//wire height
        }
        else
        {
            ui->wdg2Point1->hide();
            ui->wdg2Point2->hide();
            ui->widget_6->show();

            point[1].first = item.m_param[1].second/10.0;
            point[1].second = item.m_param[0].second/10.0;
            ui->lbl2Height->setText(QString().sprintf("%.2f", abs(point[0].first-point[1].first)));
            ui->lbl2Width->setText(QString().sprintf("%.2f", abs(point[0].second-point[1].second)));
        }
#else
        if(ui->widget_500_1->isVisible() == true || ui->widget_500_3->isVisible() == true)
        {
            ui->widget_500_1->hide();
            ui->widget_500_2->show();
            ui->widget_500_3->hide();
            ui->widget_500_4->hide();
            ui->widget_500_5->hide();
            point[0].first = item.m_param[1].second/10.0;
            point[0].second = item.m_param[0].second/10.0;
        }
        else if(ui->widget_500_2->isVisible() == true)
        {
            ui->widget_500_1->hide();
            ui->widget_500_2->hide();
            ui->widget_500_3->hide();
            ui->widget_500_4->show();
            ui->widget_500_5->hide();
            point[1].first = item.m_param[1].second/10.0;
            point[1].second = item.m_param[0].second/10.0;
        }
        else if(ui->widget_500_4->isVisible() == true)
        {
            ui->widget_500_1->hide();
            ui->widget_500_2->hide();
            ui->widget_500_3->hide();
            ui->widget_500_4->hide();
            ui->widget_500_5->show();
            point[2].first = item.m_param[1].second/10.0;
            point[2].second = item.m_param[0].second/10.0;
        }
        else if(ui->widget_500_5->isVisible() == true)
        {
            ui->widget_500_1->hide();
            ui->widget_500_2->hide();
            ui->widget_500_3->show();
            ui->widget_500_4->hide();
            ui->widget_500_5->hide();
            point[3].first = item.m_param[1].second/10.0;
            point[3].second = item.m_param[0].second/10.0;
            {//开始计算
                qDebug()<<"================";
                for(int i = 0; i < 4; i++)
                {
                    qDebug()<<point[i].first<<point[i].second;
                }
                std::pair<float, float> A,B,C,D;
                A = point[0];
                B = point[1];
                C = point[2];
                D = point[3];
                float L1,L2,M1,M2,N1,N2,H1 = 0,H2 = 0;
                L1 = qAbs(A.first - B.first);
                L2 = qAbs(C.first - D.first);
                M1 = qMax(A.second, C.second);
                M2 = qMax(B.second, D.second);
                N1 = qMin(A.second, C.second);
                N2 = qMin(B.second, D.second);
                if((L1 < 500 && L2 > 500) || (L1 > 500 && L2 < 500))
                {
                    H1 = qAbs(qAbs((500-L1)/(L2-L1))*(M1+N2-N1-M2) +N1-N2);
                }
                else if((L1 < 500 && L2 < 500) || (L1 > 500 && L2 > 500))
                {
                    H1 = qAbs(qAbs((500-L1)/(L2-500))*(M1+N2-N1-M2)+N1-N2);
                }

                std::swap(A,C);
                std::swap(B,D);
                L1 = qAbs(A.first - B.first);
                L2 = qAbs(C.first - D.first);
                M1 = qMax(A.second, C.second);
                M2 = qMax(B.second, D.second);
                N1 = qMin(A.second, C.second);
                N2 = qMin(B.second, D.second);
                if((L1 < 500 && L2 > 500) || (L1 > 500 && L2 < 500))
                {
                    H2 = qAbs(qAbs((500-L1)/(L2-L1))*(M1+N2-N1-M2) +N1-N2);
                }
                else if((L1 < 500 && L2 < 500) || (L1 > 500 && L2 > 500))
                {
                    H2 = qAbs(qAbs((500-L1)/(L2-500))*(M1+N2-N1-M2)+N1-N2);
                }
                if(H1 == 0 || H2 == 0)
                {//测量失败
                    ui->lbl_500_GC->setText(QString::fromWCharArray(L"测量失败"));
                    return;
                }
                else
                {
                    ui->lbl_500_GC->setText(QString().sprintf("%.2f", (H1+H2)/2));
                    WriteMeasureData("R006");
                }
            }
        }
#endif
        WriteMeasureData("R006");
    }
    else if(item.m_cmd == "R007")
    {
        ui->lbl_HXGD_HXGD->setText(QString().sprintf("%.1f", item.m_param[0].second/10.0f));
        {//记录测量结果
            WriteMeasureData("R007");
        }
    }
    else if(item.m_cmd == "R008")
        //结构高度
    {
        static int first_jggd = 0;
        if(ui->widget_JGGD_2->isVisible() == false)
        {
            ui->widget_JGGD_1->hide();
            ui->widget_JGGD_2->show();
            ui->widget_JGGD_3->hide();
            first_jggd = item.m_param[0].second;
        }
        else
        {
            ui->widget_JGGD_1->hide();
            ui->widget_JGGD_2->hide();
            ui->widget_JGGD_3->show();
            ui->lbl_JGGD_JGGD->setText(QString().sprintf("%.1f", qAbs(item.m_param[0].second - first_jggd)/10.0f));

            {//记录测量结果
                WriteMeasureData("R008");
            }
        }
    }
    else if(item.m_cmd == "R009")
        //非支测量
    {
        static int first_fzjj = 0, first_fzgd = 0;
        if(ui->widget_FZ_2->isVisible() == false)
        {
            ui->widget_FZ_1->hide();
            ui->widget_FZ_2->show();
            ui->widget_FZ_3->hide();
            first_fzjj = item.m_param[0].second;
            first_fzgd = item.m_param[1].second;
        }
        else
        {
            ui->widget_FZ_1->hide();
            ui->widget_FZ_2->hide();
            ui->widget_FZ_3->show();
            ui->lbl_FZ_JJ->setText(QString().sprintf("%.1f", qAbs(item.m_param[0].second - first_fzjj)/10.0f));
            ui->lbl_FZ_GD->setText(QString().sprintf("%.1f", qAbs(item.m_param[1].second - first_fzgd)/10.0f));

            {//记录测量结果
                WriteMeasureData("R009");
            }
        }
    }
    else if(item.m_cmd == "R010"){
        static int first_sdxgc = 0;
        if(ui->widget_SDXGD_2->isVisible() == false)
        {
            ui->widget_SDXGD_1->hide();
            ui->widget_SDXGD_2->show();
            ui->widget_SDXGD_3->hide();
            first_sdxgc = item.m_param[0].second;
        }
        else
        {
            ui->widget_SDXGD_1->hide();
            ui->widget_SDXGD_2->hide();
            ui->widget_SDXGD_3->show();
            ui->lbl_SDXGD_SDXGC->setText(QString().sprintf("%.1f", qAbs(item.m_param[0].second - first_sdxgc)/10.0f));

            {//记录测量结果
                WriteMeasureData("R010");
            }
        }
    }
    else if(item.m_cmd == "R011"){

        if(ui->widget_DWPD_2->isVisible() == false)
        {
            ui->widget_DWPD_1->hide();
            ui->widget_DWPD_2->show();
            ui->widget_DWPD_3->hide();
            first_dwpd1 = item.m_param[0].second;
            first_dwjd1 = item.m_param[1].second;
        }
        else
        {
            int first_dwpd2 = item.m_param[0].second;
            int first_dwjd2 = item.m_param[1].second;
            ui->widget_DWPD_1->hide();
            ui->widget_DWPD_2->hide();
            ui->widget_DWPD_3->show();
#if 0

            float angle = (first_dwpd1-first_dwpd2==0)?0:(first_dwpd1-first_dwpd2)==0?0:atan( (first_dwjd1-first_dwjd2)*1.0f/(first_dwpd1-first_dwpd2))*180/3.1415926;
            float cg = ui->lblCGZ->text().toFloat();
            float angle_plus = asin(cg/1505.0)*180.0/3.1415926;
            angle += angle_plus;
            if(angle > 180)angle = 360-angle;
            if(angle < -180)angle = 360+angle;
            float fz = tan(angle*3.1415926/180) == 0?0:1/tan(angle*3.1415926/180);
            ui->lbl_DWPD_PD->setText(QString().sprintf("1/%.2f", qAbs(fz)));
            ui->lbl_DWPD_JD->setText(QString().sprintf("%.2f", qAbs(angle)));
#else
            float dwpd=qAbs(first_dwpd1-first_dwpd2);    //拉出值绝对值
            float pdjd = qAbs(first_dwjd2-first_dwjd1);
            //            ui->label_79->setText(QString::number(item.m_param[0].second/10.0f));
            //            ui->label_74->setText(QString::number(item.m_param[1].second/10.0f));
            //定位器坡度
            float fChaogao=ui->lblCGZ->text().toFloat();//超高值

            float fChaoGaoAngle=asin(fChaogao/1505);
            //            ui->label_79->setText(QString::number(fChaoGaoAngle));
            //            ui->label_74-   >setText(QString::number(dwpd==0?0:atan((pdjd/dwpd))));
            bool flag;
            if(abs(first_dwpd1)>abs(first_dwpd2))
            {
                if(first_dwpd1<0)
                    flag=true;
                else
                    flag=false;
            }
            else
            {
                if(first_dwpd2<0)
                    flag=true;
                else
                    flag=false;
            }
            float wireAngle=(dwpd==0?0:(atan(pdjd/dwpd)));
            fChaoGaoAngle=fChaoGaoAngle*(-1);
            if((flag)^(fChaoGaoAngle<0))
            {
                if(fChaoGaoAngle<0)
                    fChaoGaoAngle=fChaoGaoAngle*(-1);
                fChaoGaoAngle=(wireAngle-fChaoGaoAngle);//
            }
            else
            {
                if(fChaoGaoAngle<0)
                    fChaoGaoAngle=fChaoGaoAngle*(-1);
                fChaoGaoAngle=(wireAngle+fChaoGaoAngle);//
            }

            if(fChaoGaoAngle<0)
                fChaoGaoAngle=fChaoGaoAngle*(-1);


            float tmp=dwpd==0?0:(1.0f/tan(fChaoGaoAngle));
            //角度坡度
            ui->lbl_DWPD_JD->setText(QString().sprintf("%.2f",(fChaoGaoAngle)*180/3.1415926)+QString::fromWCharArray(L"°"));
            //分数坡度
            ui->lbl_DWPD_PD->setText(QString().sprintf("1/%.2f",tmp));
#endif
            {//记录测量结果
                WriteMeasureData("R011");
            }
        }
    }
    else if(item.m_cmd == "R012"){
        static int res_spjj1 = 0, res_spjj2 = 0,
                res_czjj1 = 0, res_czjj2 = 0;
        if(ui->widget_ZYCL_2->isVisible() == false)
        {
            ui->widget_ZYCL_1->hide();
            ui->widget_ZYCL_2->show();
            ui->widget_ZYCL_3->hide();
            res_spjj1 = item.m_param[0].second;
            res_czjj1 = item.m_param[1].second;
        }
        else
        {
            ui->widget_ZYCL_1->hide();
            ui->widget_ZYCL_2->hide();
            ui->widget_ZYCL_3->show();
            res_spjj2 = item.m_param[0].second;
            res_czjj2 = item.m_param[1].second;

            float spjj = abs(res_spjj1-res_spjj2)/10.0f;
            float czjj = abs(res_czjj1-res_czjj2)/10.0f;
            float len = sqrt(spjj*spjj+czjj*czjj);
            ui->lbl_ZYCL_SPJJ->setText(QString().sprintf("%.1f", spjj));
            ui->lbl_ZYCL_CZJJ->setText(QString().sprintf("%.1f", czjj));
            ui->lbl_ZYCL_ZXJL->setText(QString().sprintf("%.1f", len));
            ui->lbl_ZYCL_CZJD->setText(QString().sprintf("%.1f", asin(czjj/len)*180/3.1415926));

            {//记录测量结果
                WriteMeasureData("R012");
            }
        }
    }
    else if(item.m_cmd == "R013"){
        ui->btnSC_L1->setText(QString::number(item.m_param[0].second));
        ui->btnSC_L2->setText(QString::number(item.m_param[1].second));
        ui->btnSC_L3->setText(QString::number(item.m_param[2].second));
        ui->btnSC_L4->setText(QString::number(item.m_param[3].second));
        ui->btnSC_L5->setText(QString::number(item.m_param[4].second));
    }
    else if(item.m_cmd == "R017")
    {
        switch(ui->StackMeasure->currentIndex())
        {
        case MPT_JBCL:
            ui->lblJBCL_LCZ->setText(QString::fromWCharArray(L"测量失败"));
            ui->lblJBCL_DGZ->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_KZCL:
            ui->lblKZCL_KZGD->setText(QString::fromWCharArray(L"测量失败"));
            ui->lblKZCL_KZPY->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_CMJX:
            ui->lblCMJX_CMJX->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_ZXTY:
            ui->lblZXTY_ZXTY->setText(QString::fromWCharArray(L"测量失败"));
            ui->lblZXTY_NGJ->setText(QString::fromWCharArray(L"测量失败"));
            ui->lblZXTY_PCZ->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_500GC:
            ui->lbl_500_GC->setText(QString::fromWCharArray(L"测量失败"));
            ui->lbl_500_JJ->setText(QString::fromWCharArray(L"测量失败"));
            ui->lbl2Height->setText(QString::fromWCharArray(L"测量失败"));
            ui->lbl2Width->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_HXGD:
            ui->lbl_HXGD_HXGD->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_JGGD:
            ui->lbl_JGGD_JGGD->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_FZ:
            ui->lbl_FZ_JJ->setText(QString::fromWCharArray(L"测量失败"));
            ui->lbl_FZ_GD->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_DXGD:
            ui->lbl_SDXGD_SDXGC->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_DWPD:
            ui->lbl_DWPD_JD->setText(QString::fromWCharArray(L"测量失败"));
            ui->lbl_DWPD_PD->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_ZYCL:
            ui->lbl_ZYCL_CZJD->setText(QString::fromWCharArray(L"测量失败"));
            ui->lbl_ZYCL_CZJJ->setText(QString::fromWCharArray(L"测量失败"));
            ui->lbl_ZYCL_SPJJ->setText(QString::fromWCharArray(L"测量失败"));
            ui->lbl_ZYCL_ZXJL->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_DXPD:
            ui->lbl_DWPD_JD->setText(QString::fromWCharArray(L"测量失败"));
            ui->lbl_DWPD_PD->setText(QString::fromWCharArray(L"测量失败"));
            break;
        case MPT_KJ:
            ui->lblKJ_L1->setText(QString::fromWCharArray(L"测量失败"));
            ui->lblKJ_L2->setText(QString::fromWCharArray(L"测量失败"));
            ui->lblKJ_Value->setText(QString::fromWCharArray(L"测量失败"));
            break;
        default:break;
        }
    }
}

void MainWidget::OnGetNetMsg(boost::shared_ptr<MessageStu> msg)
{
    if(msg->m_id == 2)
    {
#if 0
        unsigned char* data = (unsigned char*)msg->m_msg;
        unsigned char* rgb24 = (unsigned char*)malloc(640*480*4);
        convert_yuv_to_rgb_buffer(data,rgb24,640,480);

        QImage* img = new QImage(rgb24, 640,480,QImage::Format_RGB888);
        //bool ddd = img->loadFromData((uchar *)rgb24,640*480*3,"BMP");
        *img = img->scaled(ui->wgtCamara->width(),ui->wgtCamara->height(),Qt::IgnoreAspectRatio,
                           Qt::SmoothTransformation);
        m_img_mutex.lock();
        if(m_image)delete m_image;
        m_image = img;
        m_img_mutex.unlock();
        free(rgb24);
        //this->repaint();
        ui->wgtCamara->repaint();
#else
        unsigned char* data = (unsigned char*)msg->m_msg;
        unsigned char* rgb24 = (unsigned char*)malloc(640*480*4);
        convert_yuv_to_rgb_buffer(data,rgb24,640,480);

        QImage* img = new QImage(rgb24, 640,480,QImage::Format_RGB888);
        //bool ddd = img->loadFromData((uchar *)rgb24,640*480*3,"BMP");
        *img=img->mirrored(false,false);
        QMatrix cc;
        cc.rotate(0);
        *img=img->transformed(cc);
        *img = img->scaled(ui->wgtCamara->width(),ui->wgtCamara->height(),Qt::IgnoreAspectRatio,
                           Qt::SmoothTransformation);
        m_img_mutex.lock();
        if(m_image)delete m_image;
        m_image = img;
        m_img_mutex.unlock();
        free(rgb24);
        //this->repaint();
        ui->wgtCamara->repaint();
#endif
    }
    else if(msg->m_id == 1)
    {
        for(int i = 0; i < msg->m_len; i++)
        {
            m_wait_for_read.push_back(msg->m_msg[i]);
        }
        /*qDebug()<<"====================";
        for(std::list<char>::iterator iter = m_wait_for_read.begin(); iter != m_wait_for_read.end(); iter++)
        {
            qDebug()<<*iter;
        }*/
        AnlyzeReadBuffer();
    }
    else if(msg->m_id == 3)
    {//数据同步

        QFile::remove(DB_File);
        if(msg->m_len == 0)
        {
            if(ui->StackedMain->currentIndex()==MPT_Main)
            {//首页
                ui->edtMain->appendPlainText(QString::fromWCharArray(L"数据同步失败！测量仪端异常！"));
            }
        }
        else
        {
            QFile file(DB_File);
            if(file.open(QFile::ReadWrite))
            {//成功
                if(ui->StackedMain->currentIndex()==MPT_Main)
                {//首页
                    ui->edtMain->appendPlainText(QString::fromWCharArray(L"数据同步成功！"));
                }
                file.write(msg->m_msg, msg->m_len);
                file.close();
                ui->edtMain->hide();
                InitField();
                ShowMain(true);
            }
            else
            {
                if(ui->StackedMain->currentIndex()==MPT_Main)
                {//首页
                    ui->edtMain->appendPlainText(QString::fromWCharArray(L"数据同步失败！服务器端异常！"));
                }
            }
        }
    }
    else if(msg->m_id == 11)
    {//收到导出文件
        QDir dir(UPath);
        dir.mkdir(QString::fromWCharArray(L"数据导出"));
        QString file_name;
        int _idx = 0;
        for(int i = 0; i < msg->m_len; i++)
        {
            if(msg->m_msg[i] == ';')
            {
                file_name = QString::fromUtf8(msg->m_msg, i);
                _idx = i++;
                break;
            }
        }
        file_name.replace(".xls","");
        QFile file(QString(UPath)+ QString::fromWCharArray(L"数据导出/")+file_name+"_"+QDateTime::currentDateTime().toString("yyyy_MM_dd__hh_mm_ss")+".xls");
        file.open(QFile::ReadWrite);
        file.write(&msg->m_msg[_idx+1], msg->m_len-_idx-1);
        file.flush();
        file.close();
        qDebug()<<file_name;
    }
    /*else if(msg->m_id == 5)
    {
        ui->lstPathImportFrom->clear();
        QStringList lst;
        QString src = QString::fromUtf8(msg->m_msg, msg->m_len);
        lst = src.split(";");
        foreach(QString str, lst)
        {
            if(str != "")
            {
                ui->lstPathImportFrom->addItem(str);
            }
        }

    }*/
}

bool MainWidget::eventFilter(QObject *obj, QEvent *event)
{
    QWidget* obj_need;
    if((obj_need=dynamic_cast<QWidget*>(obj)) == ui->wgtCamara && event->type() == QEvent::Paint ||
            (obj_need=dynamic_cast<QWidget*>(obj)) == ui->wgtCamara_Config && event->type() == QEvent::Paint ||
            (obj_need=dynamic_cast<QWidget*>(obj)) == ui->wgtCamara_ac_Config && event->type() == QEvent::Paint)
    {
        QPainter pt(obj_need);
        //pt.drawLine(0,0,100,100);
        pt.fillRect(obj_need->rect(), QColor(0,0,0));

        if(m_image)
        {
            m_img_mutex.lock();
            pt.drawImage(0,0,*m_image);
            m_img_mutex.unlock();
        }

        /*绘制准心*/
        int line_len = 20;
        int center_len = 5;
        QPen pen;
        pen.setWidth(2);
        pen.setColor(qRgb(255,0,0));
        pt.setPen(pen);
        int center_x = ui->wgtCamara->width()/2;
        int center_y = ui->wgtCamara->height()/2;
        int h_pos = Config::GetInstance()->m_center_config.m_h_pos;
        int v_pos = Config::GetInstance()->m_center_config.m_v_pos;
        //pt.drawLine(QPoint(179+h_pos,135+v_pos-line_len-center_len), QPoint(179+h_pos,135+v_pos-center_len));
        //pt.drawLine(QPoint(179+h_pos,135+v_pos+line_len+center_len), QPoint(179+h_pos,135+v_pos+center_len));
        pt.drawLine(QPoint(center_x+h_pos+line_len+center_len,center_y+v_pos), QPoint(center_x+h_pos+center_len,center_y+v_pos));
        pt.drawLine(QPoint(center_x+h_pos-line_len-center_len,center_y+v_pos), QPoint(center_x+h_pos-center_len,center_y+v_pos));

        pt.drawLine(QPoint(center_x+h_pos,center_y+v_pos+line_len), QPoint(center_x+h_pos,center_y+v_pos));
        pen.setWidth(2);
        pen.setColor(qRgb(0,0,255));
        pt.setPen(pen);
        pt.drawLine(QPoint(center_x+h_pos-center_len,center_y+v_pos-line_len-2), QPoint(center_x+h_pos-center_len,center_y+v_pos-2));
        pt.drawLine(QPoint(center_x+h_pos+center_len,center_y+v_pos-line_len-2), QPoint(center_x+h_pos+center_len,center_y+v_pos-2));

        return true;
    }
    else if((obj_need=dynamic_cast<QWidget*>(obj)) == ui->wgtCamara && event->type() == QEvent::MouseButtonPress ||
            (obj_need=dynamic_cast<QWidget*>(obj)) == ui->wgtCamara_Config && event->type() == QEvent::MouseButtonPress ||
            (obj_need=dynamic_cast<QWidget*>(obj)) == ui->wgtCamara_ac_Config && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
        MotoRun(mouse_event->pos().rx() - obj_need->rect().width()/2);
    }
    return false;
}

void MainWidget::OnReadNetMessage(boost::shared_ptr<MessageStu> msg)
{
    emit DoGetNetMsg(msg);
}

void MainWidget::on_btnJBCL_clicked(){
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_JBCL);
}
void MainWidget::on_btnKZCL_clicked(){
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_KZCL);
}
void MainWidget::on_btnCMJX_clicked(){
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_CMJX);
}
void MainWidget::on_btnZXTY_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_ZXTY);
}
void MainWidget::on_btn500GC_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_500GC);
}
void MainWidget::on_btnHXGD_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_HXGD);
}
void MainWidget::on_btnJGGD_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_JGGD);
}
//函数功能：非支测量
void MainWidget::on_btnFZ_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_FZ);
}
void MainWidget::on_btnDXGD_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_DXGD);
}
void MainWidget::on_btnDWPD_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_DWPD);
}
void MainWidget::on_btnZYCL_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_ZYCL);
}
void MainWidget::on_btnDXPD_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_DXPD);
}
//函数功能：测量按钮，根据不同的测量类型，发送不同的测量命令
void MainWidget::on_btnMeasure_clicked()
{
    QFile file(QString::fromWCharArray(L"./线路.csv"));
    if(file.open(QFile::WriteOnly|QFile::Append))
    {
        //cbMS_xl    cbMS_zq cbMS_md
        if(m_filed_map[ui->cbMS_xl->currentText()][ui->cbMS_xj->currentText()][ui->cbMS_cj->currentText()][ui->cbMS_gq->currentText()].indexOf(ui->cbMS_zq->currentText()) == -1)
        {
            m_filed_map[ui->cbMS_xl->currentText()][ui->cbMS_xj->currentText()][ui->cbMS_cj->currentText()][ui->cbMS_gq->currentText()].push_back(ui->cbMS_zq->currentText());
            file.write((ui->cbMS_xl->currentText()+",").toLocal8Bit());
            file.write((ui->cbMS_xj->currentText()+",").toLocal8Bit());
            file.write((ui->cbMS_cj->currentText()+",").toLocal8Bit());
            file.write((ui->cbMS_gq->currentText()+",").toLocal8Bit());
            file.write((ui->cbMS_zq->currentText()+"\n").toLocal8Bit());
        }

        file.close();
        AddCurrentTextToComb(ui->cbMS_xl);
        AddCurrentTextToComb(ui->cbMS_xj);
        AddCurrentTextToComb(ui->cbMS_cj);
        AddCurrentTextToComb(ui->cbMS_gq);
        AddCurrentTextToComb(ui->cbMS_zq);
    }
    if(ui->StackMeasure->currentIndex()==MPT_JBCL)//基本测量
    {
        SendSerialMsg("R002##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_KZCL)
    {
        SendSerialMsg("R003##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_CMJX)
    {
        SendSerialMsg("R004##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_ZXTY)
    {
        SendSerialMsg("R005##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_500GC)
    {
        SendSerialMsg("R006##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_HXGD)
    {
        SendSerialMsg("R007##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_JGGD)
    {
        SendSerialMsg("R008##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_FZ)
    {
        SendSerialMsg("R009##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_DXGD)
    {
        SendSerialMsg("R010##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_DWPD)
    {
        SendSerialMsg("R011##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_ZYCL)
    {
        SendSerialMsg("R012##");
    }
    else if(ui->StackMeasure->currentIndex()==MPT_DXPD)
    {
        SendSerialMsg("R002##");//导线坡度
    }
    else if(ui->StackMeasure->currentIndex()==MPT_KJ)
    {
        SendSerialMsg("R002##");//跨距测量
    }
}


DefKeyInsertPushButtonCallBack(btn_ZXTY_param)
DefKeyDeletePushButtonCallBack(btn_ZXTY_param)

void MainWidget::on_btn_ZXTY_param_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect btn_rect = ui->btn_ZXTY_param->rect();
    QPoint btn_left_bottom = ui->btn_ZXTY_param->mapToGlobal(QPoint(btn_rect.left(), btn_rect.bottom()));
    m_key_board.move(btn_left_bottom);
    KeyBindCallBack(btn_ZXTY_param);
}

void MainWidget::on_btnZXTY_OK_clicked()
{
    if(ui->btn_ZXTY_param->text().toInt() < 999 &&
            ui->btn_ZXTY_param->text().toInt() > 100)
    {
        ui->widgetZXTY_Param->hide();
        ui->widgetZXTYMeasure->show();
        SendSerialMsg(std::string("W102")+"01"+
                      ui->btn_ZXTY_param->text().sprintf("%06d",
                                                         ui->btn_ZXTY_param->text().toInt()).toStdString()+"01##");


    }
}

DefKeyInsertPushButtonCallBack(btnSC_year)
DefKeyDeletePushButtonCallBack(btnSC_year)
void MainWidget::on_btnSC_year_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect keyboard_rect = m_key_board.rect();
    QPoint btn_left_top = ui->btnSC_year->mapToGlobal(QPoint(0, 0));
    btn_left_top.setY(btn_left_top.y() - keyboard_rect.height());
    m_key_board.move(btn_left_top);
    KeyBindCallBack(btnSC_year);
}

DefKeyInsertPushButtonCallBack(btnSC_month)
DefKeyDeletePushButtonCallBack(btnSC_month)
void MainWidget::on_btnSC_month_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect keyboard_rect = m_key_board.rect();
    QPoint btn_left_top = ui->btnSC_month->mapToGlobal(QPoint(0, 0));
    btn_left_top.setY(btn_left_top.y() - keyboard_rect.height());
    m_key_board.move(btn_left_top);
    KeyBindCallBack(btnSC_month);
}

DefKeyInsertPushButtonCallBack(btnSC_day)
DefKeyDeletePushButtonCallBack(btnSC_day)
void MainWidget::on_btnSC_day_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect keyboard_rect = m_key_board.rect();
    QPoint btn_left_top = ui->btnSC_year->mapToGlobal(QPoint(0, 0));
    btn_left_top.setY(btn_left_top.y() - keyboard_rect.height());
    m_key_board.move(btn_left_top);
    KeyBindCallBack(btnSC_day);
}

DefKeyInsertPushButtonCallBack(btnSC_hour)
DefKeyDeletePushButtonCallBack(btnSC_hour)
void MainWidget::on_btnSC_hour_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect keyboard_rect = m_key_board.rect();
    QPoint btn_left_top = ui->btnSC_hour->mapToGlobal(QPoint(0, 0));
    btn_left_top.setY(btn_left_top.y() - keyboard_rect.height());
    m_key_board.move(btn_left_top);
    KeyBindCallBack(btnSC_hour);
}

DefKeyInsertPushButtonCallBack(btnSC_min)
DefKeyDeletePushButtonCallBack(btnSC_min)
void MainWidget::on_btnSC_min_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect keyboard_rect = m_key_board.rect();
    QPoint btn_left_top = ui->btnSC_min->mapToGlobal(QPoint(0, 0));
    btn_left_top.setY(btn_left_top.y() - keyboard_rect.height());
    m_key_board.move(btn_left_top);
    KeyBindCallBack(btnSC_min);
}

DefKeyInsertPushButtonCallBack(btnSC_second)
DefKeyDeletePushButtonCallBack(btnSC_second)
void MainWidget::on_btnSC_second_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect keyboard_rect = m_key_board.rect();
    QPoint btn_left_top = ui->btnSC_year->mapToGlobal(QPoint(0, 0));
    btn_left_top.setY(btn_left_top.y() - keyboard_rect.height());
    m_key_board.move(btn_left_top);
    KeyBindCallBack(btnSC_second);
}

void MainWidget::on_btnSC_save_time_clicked()
{
    SYSTEMTIME curtime;//声明结构体变量;
    curtime.wYear = ui->btnSC_year->text().toInt();
    curtime.wMonth = ui->btnSC_month->text().toInt();;
    curtime.wDay = ui->btnSC_day->text().toInt();;
    curtime.wHour = ui->btnSC_hour->text().toInt();;
    curtime.wMinute = ui->btnSC_min->text().toInt();;
    curtime.wSecond = ui->btnSC_second->text().toInt();;
    SetLocalTime(&curtime);
}

DefKeyInsertPushButtonCallBack(btnSC_L1)
DefKeyDeletePushButtonCallBack(btnSC_L1)
void MainWidget::on_btnSC_L1_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect btn_rect = ui->btnSC_L1->rect();
    QPoint btn_left_bottom = ui->btnSC_L1->mapToGlobal(QPoint(btn_rect.left(), btn_rect.bottom()));
    m_key_board.move(btn_left_bottom);
    KeyBindCallBack(btnSC_L1);
}

DefKeyInsertPushButtonCallBack(btnSC_L2)
DefKeyDeletePushButtonCallBack(btnSC_L2)
void MainWidget::on_btnSC_L2_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect btn_rect = ui->btnSC_L2->rect();
    QPoint btn_left_bottom = ui->btnSC_L2->mapToGlobal(QPoint(btn_rect.left(), btn_rect.bottom()));
    m_key_board.move(btn_left_bottom);
    KeyBindCallBack(btnSC_L2);
}
DefKeyInsertPushButtonCallBack(btnSC_L3)
DefKeyDeletePushButtonCallBack(btnSC_L3)
void MainWidget::on_btnSC_L3_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect btn_rect = ui->btnSC_L3->rect();
    QPoint btn_left_bottom = ui->btnSC_L3->mapToGlobal(QPoint(btn_rect.left(), btn_rect.bottom()));
    m_key_board.move(btn_left_bottom);
    KeyBindCallBack(btnSC_L3);
}
DefKeyInsertPushButtonCallBack(btnSC_L4)
DefKeyDeletePushButtonCallBack(btnSC_L4)
void MainWidget::on_btnSC_L4_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect btn_rect = ui->btnSC_L4->rect();
    QPoint btn_left_bottom = ui->btnSC_L4->mapToGlobal(QPoint(btn_rect.left(), btn_rect.bottom()));
    m_key_board.move(btn_left_bottom);
    KeyBindCallBack(btnSC_L4);
}
DefKeyInsertPushButtonCallBack(btnSC_L5)
DefKeyDeletePushButtonCallBack(btnSC_L5)
void MainWidget::on_btnSC_L5_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect btn_rect = ui->btnSC_L5->rect();
    QPoint btn_left_bottom = ui->btnSC_L5->mapToGlobal(QPoint(btn_rect.left(), btn_rect.bottom()));
    m_key_board.move(btn_left_bottom);
    KeyBindCallBack(btnSC_L5);
}

void MainWidget::on_btnSC_SaveL_clicked()
{
    QString for_send="W101";
    for_send+= "01"+QString().sprintf("%06d", ui->btnSC_L1->text().toInt());
    for_send+= "02"+QString().sprintf("%06d", ui->btnSC_L2->text().toInt());
    for_send+= "03"+QString().sprintf("%06d", ui->btnSC_L3->text().toInt());
    for_send+= "04"+QString().sprintf("%06d", ui->btnSC_L4->text().toInt());
    for_send+= "05"+QString().sprintf("%06d", ui->btnSC_L5->text().toInt());
    for_send+= "15##";
    this->SendSerialMsg(for_send.toStdString());
}

/*void MainWidget::on_btnSC_JDConfig_clicked()
{
    if(ui->lblSC_JDConfig->isVisible())
    {
        SendSerialMsg("R114##");
    }
    else
    {
        SendSerialMsg("R014##");
    }
}*

/*void MainWidget::on_btnSC_QJConfig_clicked()
{
    if(ui->lblSC_QJConfig->isVisible())
    {
        SendSerialMsg("R115##");
    }
    else
    {
        SendSerialMsg("R015##");
    }
}*/


void MainWidget::on_btnSY_up_clicked()
{
    Config::GetInstance()->m_center_config.m_v_pos = Config::GetInstance()->m_center_config.m_v_pos-1;
    Config::GetInstance()->Save();
}

void MainWidget::on_btnSY_left_clicked()
{
    Config::GetInstance()->m_center_config.m_h_pos = Config::GetInstance()->m_center_config.m_h_pos-1;
    Config::GetInstance()->Save();
}

void MainWidget::on_btnSY_right_clicked()
{
    Config::GetInstance()->m_center_config.m_h_pos = Config::GetInstance()->m_center_config.m_h_pos+1;
    Config::GetInstance()->Save();
}

void MainWidget::on_btnSY_down_clicked()
{
    Config::GetInstance()->m_center_config.m_v_pos = Config::GetInstance()->m_center_config.m_v_pos+1;
    Config::GetInstance()->Save();
}

void MainWidget::on_cbSC_clms_clicked()
{
    ui->cbSC_clms->setChecked(true);
    ui->cbSC_jlms->setChecked(false);
    Config::GetInstance()->m_record_config.m_check_mode = RecordConfig::CM_Common;
    Config::GetInstance()->Save();
}

void MainWidget::on_cbSC_jlms_clicked()
{
    ui->cbSC_clms->setChecked(false);
    ui->cbSC_jlms->setChecked(true);
    Config::GetInstance()->m_record_config.m_check_mode = RecordConfig::CM_Record;
    Config::GetInstance()->Save();
}

void MainWidget::on_btnLaser_clicked()
{
    this->SendSerialMsg("C001##");
}
#if warning_signal
void MainWidget::on_btnSC_jd_clear_clicked()
{
    this->SendSerialMsg("C004##");
}

void MainWidget::on_btnSC_qj_clear_clicked()
{
    this->SendSerialMsg("C003##");
}

void MainWidget::on_btnACLaser_2_clicked()
{
    this->SendSerialMsg("C001##");
}

#endif
void MainWidget::on_cbMS_xl_currentIndexChanged(const QString &arg1)
{
    ui->cbMS_xj->clear();
    QStringList list_for_push = GetSqliteField(ui->cbMS_xl->currentText());
    ui->cbMS_xj->addItems(list_for_push);
    if(m_is_load_xl == true)
    {
        if(Config::GetInstance()->m_record_config.m_xl_name != "")
        {
            int idx;
            if((idx = ui->cbMS_xl->findText(Config::GetInstance()->m_record_config.m_xl_name)) != -1)
            {
                ui->cbMS_xl->setCurrentIndex(idx);
            }
        }
    }
}

void MainWidget::on_cbMS_xj_currentIndexChanged(const QString &)
{
    ui->cbMS_cj->clear();
    QStringList list_for_push = GetSqliteField(ui->cbMS_xl->currentText(), ui->cbMS_xj->currentText());
    ui->cbMS_cj->addItems(list_for_push);
    if(m_is_load_xl == true)
    {
        if(Config::GetInstance()->m_record_config.m_xj_name != "")
        {
            int idx;
            if((idx = ui->cbMS_xj->findText(Config::GetInstance()->m_record_config.m_xj_name)) != -1)
            {
                ui->cbMS_xj->setCurrentIndex(idx);
            }
        }
    }
}

void MainWidget::on_cbMS_cj_currentIndexChanged(const QString &)
{
    ui->cbMS_gq->clear();
    QStringList list_for_push = GetSqliteField(ui->cbMS_xl->currentText(), ui->cbMS_xj->currentText(), ui->cbMS_cj->currentText());
    ui->cbMS_gq->addItems(list_for_push);
    if(m_is_load_xl == true)
    {
        if(Config::GetInstance()->m_record_config.m_cj_name != "")
        {
            int idx;
            if((idx = ui->cbMS_cj->findText(Config::GetInstance()->m_record_config.m_cj_name)) != -1)
            {
                ui->cbMS_cj->setCurrentIndex(idx);
            }
        }
    }
}

void MainWidget::on_cbMS_gq_currentIndexChanged(const QString &)
{
    ui->cbMS_zq->clear();
    ui->cbMS_zq->addItems(GetSqliteField(ui->cbMS_xl->currentText(), ui->cbMS_xj->currentText(), ui->cbMS_cj->currentText(), ui->cbMS_gq->currentText()));
    if(m_is_load_xl == true)
    {
        if(Config::GetInstance()->m_record_config.m_gq_name != "")
        {
            int idx;
            if((idx = ui->cbMS_gq->findText(Config::GetInstance()->m_record_config.m_gq_name)) != -1)
            {
                ui->cbMS_gq->setCurrentIndex(idx);
            }
        }
    }
}

void MainWidget::on_cbMS_zq_currentIndexChanged(const QString &)
{
    ui->cbMS_md->clear();
    ui->cbMS_md->addItems(GetSqliteField(ui->cbMS_xl->currentText(), ui->cbMS_xj->currentText(), ui->cbMS_cj->currentText(), ui->cbMS_gq->currentText(), ui->cbMS_zq->currentText()));
    if(m_is_load_xl == true)
    {
        if(Config::GetInstance()->m_record_config.m_zq_name != "")
        {
            int idx;
            if((idx = ui->cbMS_zq->findText(Config::GetInstance()->m_record_config.m_zq_name)) != -1)
            {
                ui->cbMS_zq->setCurrentIndex(idx);
            }
        }
    }
}

void MainWidget::on_cbMS_md_currentIndexChanged(const QString &)
{
    if(m_is_load_xl == true)
    {
        if(Config::GetInstance()->m_record_config.m_zzh_name != "")
        {
            int idx;
            if((idx = ui->cbMS_md->findText(Config::GetInstance()->m_record_config.m_zzh_name)) != -1)
            {
                ui->cbMS_md->setCurrentIndex(idx);
            }
        }
    }
    else
    {
        Config::GetInstance()->m_record_config.m_xl_name = ui->cbMS_xl->currentText();
        Config::GetInstance()->m_record_config.m_xj_name = ui->cbMS_xj->currentText();
        Config::GetInstance()->m_record_config.m_cj_name = ui->cbMS_cj->currentText();
        Config::GetInstance()->m_record_config.m_gq_name = ui->cbMS_gq->currentText();
        Config::GetInstance()->m_record_config.m_zq_name = ui->cbMS_zq->currentText();
        Config::GetInstance()->m_record_config.m_zzh_name = ui->cbMS_md->currentText();
        Config::GetInstance()->Save();
    }
}

void MainWidget::on_btnNextM_clicked()
{
    ui->cbMS_md->setCurrentIndex((ui->cbMS_md->currentIndex()+1>ui->cbMS_md->count()-1)?(ui->cbMS_md->count()-1):(ui->cbMS_md->currentIndex()+1));
}

void MainWidget::on_btnLastM_clicked()
{
    ui->cbMS_md->setCurrentIndex((ui->cbMS_md->currentIndex()-1<0)?0:(ui->cbMS_md->currentIndex()-1));
}


void MainWidget::on_btnViewData_clicked()
{
    if(QMessageBox::information(NULL, QString::fromWCharArray(L"提示"), QString::fromWCharArray(L"是否确认删除该线路数据?"), QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
    {
        return;
    }
    QFile file(QString::fromWCharArray(L"./data.csv"));
    QVector<QVector<std::pair<QString, QString> > > record;
    if(file.open(QFile::ReadOnly))
    {
        QTextStream stream(&file);

        while(!stream.atEnd())
        {
            QString str_line1 = stream.readLine();
            str_line1.left(str_line1.length()-1);
            QStringList str_list1 = str_line1.split(",");

            QString str_line2 = stream.readLine();
            str_line2.left(str_line2.length()-1);
            QStringList str_list2 = str_line2.split(",");


            QVector<std::pair<QString, QString> > one_map;
            for(int i = 0; i < str_list1.count(); i++)
            {
                //one_map[str_list1[i]] = str_list2[i];
                one_map.push_back(std::pair<QString, QString>(str_list1[i], str_list2[i]));
            }
            record.push_back(one_map);
        }
        file.close();
    }
    for(int i = 0; i < record.size(); i++)
    {
        if(record[i][5].second == ui->cmbFilter->currentText())
        {
            record.removeAt(i);
            i--;
        }
    }
    QFile file_out(QString::fromWCharArray(L"./data1.csv"));
    file_out.remove();
    if(file_out.open(QFile::WriteOnly))
    {
        for(int i = 0; i < record.size(); i++)
        {
            for(QVector<std::pair<QString, QString> >::iterator iter = record[i].begin(); iter != record[i].end(); iter++)
            {
                file_out.write(iter->first.toLocal8Bit());
                QVector<std::pair<QString, QString> >::iterator iter_tmp = iter;iter_tmp++;
                if(iter_tmp != record[i].end())
                {
                    file_out.write(",");
                }
            }
            file_out.write("\n");
            for(QVector<std::pair<QString, QString> >::iterator iter = record[i].begin(); iter != record[i].end(); iter++)
            {
                file_out.write(iter->second.toLocal8Bit());
                QVector<std::pair<QString, QString> >::iterator iter_tmp = iter;iter_tmp++;
                if(iter_tmp != record[i].end())
                {
                    file_out.write(",");
                }
            }
            file_out.write("\n");
        }
        file_out.close();
        file.remove();
        file_out.rename(QString::fromWCharArray(L"./data.csv"));
        while(ui->wgtDataView->rowCount())ui->wgtDataView->removeRow(0);
    }
}

void MainWidget::on_btnDV_Back_clicked()
{
    ui->StackedMain->setCurrentIndex(MPT_DataManager);

}

void MainWidget::on_btn_listView_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWidget::on_btn_plotView_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWidget::on_slr_sm_moto_valueChanged(int value)
{
    Config::GetInstance()->m_moto_config.m_s_moto_speed = value;
    Config::GetInstance()->Save();
}

void MainWidget::on_slr_big_moto_valueChanged(int value)
{
    Config::GetInstance()->m_moto_config.m_b_moto_speed = value;
    Config::GetInstance()->Save();
}

void MainWidget::on_btn_left_clicked()
{
    
}

void MainWidget::on_btn_left_small_clicked()
{
    ControlMoto(true, true);
}

void MainWidget::on_btn_right_clicked()
{

}

void MainWidget::on_btn_right_small_clicked()
{
    ControlMoto(false, true);
}

void MainWidget::on_btnSC_JDQJConfig_clicked()
{
    ui->StackedMain->setCurrentIndex(MPT_JDQJ);
}

void MainWidget::on_btn_AC_left_clicked()
{
    on_btn_left_clicked();
}

void MainWidget::on_btnACLaser_clicked()
{
    on_btnLaser_clicked();
}

void MainWidget::on_btn_AC_right_clicked()
{
    on_btn_right_clicked();
}

void MainWidget::on_btn_AC_left_small_clicked()
{
    //on_btn_left_small_clicked();
}

void MainWidget::on_btn_AC_right_small_clicked()
{
    //on_btn_right_small_clicked();
}

void MainWidget::on_btn_AC_Back_clicked()
{
    ui->StackedMain->setCurrentIndex(MPT_SystemConfig);
}

void MainWidget::on_btnSC_JDConfig_2_clicked()
{
    if(ui->btnSC_JDConfig_2->text() == QString::fromWCharArray(L"返回"))
    {
        ui->btnSC_JDConfig_2->setText(QString::fromWCharArray(L"拉出值校准"));
        ui->lblACMessage->setVisible(false);
    }
    else if(ui->btnSC_JDConfig_2->text() == QString::fromWCharArray(L"拉出值校准"))
    {
        ui->btnSC_QJConfig_2->setText(QString::fromWCharArray(L"超高校准"));
        ui->btnSC_JDConfig_2->setText(QString::fromWCharArray(L"返回"));
        ui->lblACMessage->setVisible(true);
        ui->lblACMessage->setText(QString::fromWCharArray(L"拉出值校准第一次测量"));
    }

    /*if(ui->lblSC_JDConfig->isVisible())
    {
        SendSerialMsg("R114##");
    }
    else
    {
        SendSerialMsg("R014##");
    }*/
}

void MainWidget::on_btnSC_QJConfig_2_clicked()
{
    if(ui->btnSC_QJConfig_2->text() == QString::fromWCharArray(L"返回"))
    {
        ui->btnSC_QJConfig_2->setText(QString::fromWCharArray(L"超高校准"));
        ui->lblACMessage->setVisible(false);
    }
    else if(ui->btnSC_QJConfig_2->text() == QString::fromWCharArray(L"超高校准"))
    {
        ui->btnSC_JDConfig_2->setText(QString::fromWCharArray(L"拉出值校准"));
        ui->btnSC_QJConfig_2->setText(QString::fromWCharArray(L"返回"));
        ui->lblACMessage->setVisible(true);
        ui->lblACMessage->setText(QString::fromWCharArray(L"超高校准第一次测量"));
    }
}

void MainWidget::on_btnConfigMeasure_clicked()
{
    if(ui->lblACMessage->text() == QString::fromWCharArray(L"拉出值校准第一次测量"))
    {
        SendSerialMsg("R014##");
        ui->lblACMessage->setText(QString::fromWCharArray(L"拉出值校准第二次测量"));
    }
    else if(ui->lblACMessage->text() == QString::fromWCharArray(L"拉出值校准第二次测量"))
    {
        SendSerialMsg("R114##");
        ui->lblACMessage->setText("");
        ui->btnSC_JDConfig_2->setText(QString::fromWCharArray(L"拉出值校准"));
    }
    else if(ui->lblACMessage->text() == QString::fromWCharArray(L"超高校准第一次测量"))
    {
        SendSerialMsg("R015##");
        ui->lblACMessage->setText(QString::fromWCharArray(L"超高校准第二次测量"));
    }
    else if(ui->lblACMessage->text() == QString::fromWCharArray(L"超高校准第二次测量"))
    {
        SendSerialMsg("R115##");
        ui->lblACMessage->setText("");
        ui->btnSC_QJConfig_2->setText(QString::fromWCharArray(L"超高校准"));
    }
    else
    {
        SendSerialMsg("R002##");//参数设置
    }
}

void MainWidget::on_btnSC_jd_clear_2_clicked()
{
    this->SendSerialMsg("C004##");
}

void MainWidget::on_btnSC_qj_clear_2_clicked()
{
    this->SendSerialMsg("C003##");
}

void MainWidget::on_btn_left_pressed()
{
    m_b_moto_direct = true;
    m_b_moto_small = false;
    OnMotoTimerOut();
    m_moto_timer.start();
}

void MainWidget::on_btn_left_released()
{
    m_moto_timer.stop();
}

void MainWidget::on_btn_right_pressed()
{
    m_b_moto_direct = false;
    m_b_moto_small = false;
    OnMotoTimerOut();
    m_moto_timer.start();
}

void MainWidget::on_btn_right_released()
{
    m_moto_timer.stop();
}

void MainWidget::on_btn_left_small_pressed()
{
    m_b_moto_direct = true;
    m_b_moto_small = true;
    OnMotoTimerOut();
    m_moto_timer.start();
}

void MainWidget::on_btn_left_small_released()
{
    m_moto_timer.stop();
}

void MainWidget::on_btn_right_small_pressed()
{
    m_b_moto_direct = false;
    m_b_moto_small = true;
    OnMotoTimerOut();
    m_moto_timer.start();
}

void MainWidget::on_btn_right_small_released()
{
    m_moto_timer.stop();
}



void MainWidget::on_btn_AC_left_pressed()
{
    on_btn_left_pressed();
}

void MainWidget::on_btn_AC_left_released()
{
    on_btn_left_released();
}

void MainWidget::on_btn_AC_right_pressed()
{
    on_btn_right_pressed();
}

void MainWidget::on_btn_AC_right_released()
{
    on_btn_right_released();
}

void MainWidget::on_btn_AC_left_small_pressed()
{
    on_btn_left_small_pressed();
}

void MainWidget::on_btn_AC_left_small_released()
{
    on_btn_left_small_released();
}

void MainWidget::on_btn_AC_right_small_pressed()
{
    on_btn_right_small_pressed();
}

void MainWidget::on_btn_AC_right_small_released()
{
    on_btn_right_small_released();
}

void MainWidget::on_slr_touch_scale_valueChanged(int value)
{
    Config::GetInstance()->m_moto_config.m_touch_scale = value;
    Config::GetInstance()->Save();
}

void MainWidget::on_slr_moto_step_valueChanged(int value)
{
    Config::GetInstance()->m_moto_config.m_moto_run_step = value;
    Config::GetInstance()->Save();
}




void MainWidget::on_btnPDJDOK_clicked()
{//开始计算导线坡度
    //m_dxpd_dg2 = item.m_param[1].second;
    ui->lbl_PDJD_msg1->setVisible(false);
    ui->lbl_PDJD_msg2->setVisible(false);
    ui->wgtPDMSG->setVisible(true);
    ui->wgtPDJDJJ->setVisible(false);
    float pd = qAbs((m_dxpd_dg2-m_dxpd_dg1)/10.0f/ui->edt_PDJD_JL->text().toFloat());
    ui->lblPDJD_PD->setText(QString().sprintf("%.2f", pd*1000));
    if(pd > 0.0015f)
    {
        ui->lblPDJD_PD->setStyleSheet("color:rgb(255,0,0)");
    }
    else if(pd > 0.001f)
    {
        ui->lblPDJD_PD->setStyleSheet("color:rgb(255,125,0)");
    }
    else
    {
        ui->lblPDJD_PD->setStyleSheet("color:rgb(0,255,0)");
    }
    {//记录测量结果
        QString str_cmd = QString::fromWCharArray(
                    L"insert into data(线路,行别,车间,工区,站区,支柱号,测量日期,测量时间,轨距,超高,角度,接触线) "
                    "values('%1','%2','%3','%4','%5','%6','%7','%8','%9','%10','%11','%12')")
                .arg(ui->cbMS_xl->currentText())
                .arg(ui->cbMS_xj->currentText())
                .arg(ui->cbMS_cj->currentText())
                .arg(ui->cbMS_gq->currentText())
                .arg(ui->cbMS_zq->currentText())
                .arg(ui->cbMS_md->currentText())
                .arg(QDate::currentDate().toString("yyyy-MM-dd"))
                .arg(QTime::currentTime().toString("HH:mm:ss"))
                .arg(ui->lblGJZ->text())
                .arg(ui->lblCGZ->text())
                .arg(ui->lblJDZ->text())
                .arg(ui->lblPDJD_PD->text());
        WriteMeasureData("xxx");
    }
}
DefKeyInsertPushButtonCallBack(edt_PDJD_JL)
DefKeyDeletePushButtonCallBack(edt_PDJD_JL)
void MainWidget::on_edt_PDJD_JL_clicked()
{
    m_key_board.show();
    m_key_board.activateWindow();
    QRect btn_rect = ui->edt_PDJD_JL->rect();
    QPoint btn_left_bottom = ui->edt_PDJD_JL->mapToGlobal(QPoint(btn_rect.left(), btn_rect.bottom()));
    m_key_board.move(btn_left_bottom);
    KeyBindCallBack(edt_PDJD_JL);
}



void MainWidget::on_cbMS_xl_editTextChanged(const QString &arg1)
{
    if(!IsStringInComb(ui->cbMS_xl, arg1))
    {
        ui->cbMS_xj->clear();
    }
}

void MainWidget::on_cbMS_xj_editTextChanged(const QString &arg1)
{
    if(!IsStringInComb(ui->cbMS_xj, arg1))
    {
        ui->cbMS_cj->clear();
    }
}

void MainWidget::on_cbMS_cj_editTextChanged(const QString &arg1)
{
    if(!IsStringInComb(ui->cbMS_cj, arg1))
    {
        ui->cbMS_gq->clear();
    }
}

void MainWidget::on_cbMS_gq_editTextChanged(const QString &arg1)
{
    if(!IsStringInComb(ui->cbMS_gq, arg1))
    {
        ui->cbMS_zq->clear();
    }
}

void MainWidget::on_cbMS_zq_editTextChanged(const QString &arg1)
{

}
#include <map>
void MainWidget::on_btnDataExport_clicked()
{
    QDir dir2(UPath);
    dir2.mkdir(QString::fromWCharArray(L"数据导出"));
    QDir dir1("D:/");
    dir1.mkdir(QString::fromWCharArray(L"数据导出"));

    QString file_whole_name = ui->cmbFilter->currentText()+QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss")+".csv";
    QString file_name = UPath+QString::fromWCharArray(L"数据导出/")+file_whole_name;
    QString file_name2 = QString("D:/")+QString::fromWCharArray(L"数据导出/")+file_whole_name;
    QFile file_out(file_name2);
    file_out.remove();
    if(file_out.open(QFile::WriteOnly))
    {
        for(int i = 0; i < ui->wgtDataView->horizontalHeader()->count(); i++)
        {
            file_out.write(ui->wgtDataView->horizontalHeaderItem(i)->text().toLocal8Bit());
            if(i != ui->wgtDataView->horizontalHeader()->count()-1)
            {
                file_out.write(",");
            }

        }
        file_out.write("\n");
        for(int i = 0; i < ui->wgtDataView->rowCount(); i++)
        {
            for(int j = 0; j < ui->wgtDataView->horizontalHeader()->count(); j++)
            {
                if(ui->wgtDataView->item(i,j))
                {
                    file_out.write(ui->wgtDataView->item(i,j)->text().toLocal8Bit());
                }
                if(j != ui->wgtDataView->horizontalHeader()->count()-1)
                {
                    file_out.write(",");
                }
            }
            file_out.write("\n");
        }
        file_out.close();
        QMessageBox::information(NULL, QString::fromWCharArray(L"提示"), QString::fromWCharArray(L"数据导出到%1成功").arg(file_name2), QMessageBox::Ok);

        return;
    }
    QMessageBox::information(NULL, QString::fromWCharArray(L"提示"), QString::fromWCharArray(L"数据导出失败"), QMessageBox::Ok);
}

void MainWidget::on_cmbFilter_currentTextChanged(const QString &arg1)
{
    FlushDataView();
}

void MainWidget::on_btn_sm_sub_clicked()
{
    ui->slr_sm_moto->setValue(ui->slr_sm_moto->value()-10);
}

void MainWidget::on_btn_sm_add_clicked()
{
    ui->slr_sm_moto->setValue(ui->slr_sm_moto->value()+10);
}

void MainWidget::on_btn_big_sub_clicked()
{
    ui->slr_big_moto->setValue(ui->slr_big_moto->value()-20);
}

void MainWidget::on_btn_big_add_clicked()
{
    ui->slr_big_moto->setValue(ui->slr_big_moto->value()+20);
}

void MainWidget::on_btn_touch_sub_clicked()
{
    ui->slr_touch_scale->setValue(ui->slr_touch_scale->value()-10);
}
void MainWidget::on_btn_touch_add_clicked()
{
    ui->slr_touch_scale->setValue(ui->slr_touch_scale->value()+10);
}
void MainWidget::on_btn_moto_step_sub_clicked()
{
    ui->slr_moto_step->setValue(ui->slr_moto_step->value()-20);
}
void MainWidget::on_btn_moto_step_add_clicked()
{
    ui->slr_moto_step->setValue(ui->slr_moto_step->value()+20);
}
//函数功能：对测量数据进行远程同步
void MainWidget::on_btnRemoteUpdate_clicked()
{
    if(remoteDbStatus)
    {
        getRemoteDatas(alreadyTms);
        //        qDebug()<<"already data time lists="<<alreadyTms;
        int updateCount=0;
        for(int i = 0; i < ui->wgtDataView->rowCount(); i++)
        {
            remote_data mm;
            QStringList tmpData;
            for(int j = 0; j <7; j++)
            {
                //                qDebug()<<;
                if(ui->wgtDataView->item(i,j))
                {
                    wchar_t mm[100];
                    mm[ui->wgtDataView->item(i,j)->text().toWCharArray(mm)]=0;

                    //                    tmpData.fromStdWString(mm).append(",");
                    tmpData.append(ui->wgtDataView->item(i,j)->text().toLocal8Bit());
                    //                    file_out.write(ui->wgtDataView->item(i,j)->text().toLocal8Bit());
                }
                else
                    tmpData.append("0");
                //                if(j != ui->wgtDataView->horizontalHeader()->count()-1)
                //                {
                //                    file_out.write(",");
                //                }
            }
            //            qDebug()<<"measure data"<<i<<tmpData.count()<<tmpData<<"insert ="<<
            if(insertRemoteDb(tmpData))
            {
                updateCount++;
            }
        }
        QMessageBox::about(NULL, QString::fromWCharArray(L"提示"),
                           QString::fromWCharArray(L"成功更新数据%1条...").arg(updateCount));
    }
}
//函数功能：跨距测量
void MainWidget::on_btnKJ_clicked()
{
    ResetAllMeasureLabel();
    ui->StackMeasure->setCurrentIndex(MPT_KJ);
}
#include <qsqlerror.h>
void MainWidget::connect2MysqlDb()
{
    //本地测试
    db_mysql =QSqlDatabase::addDatabase("QMYSQL");
    db_mysql.setHostName("120.25.233.115");
    db_mysql.setPort(3306);
    db_mysql.setDatabaseName("electricdata");
    db_mysql.setUserName("root");
    db_mysql.setPassword("mytianjun-mysql");
    if(db_mysql.open())
    {
        remoteDbStatus=true;
        //        this->statusBar()->showMessage(QString::fromWCharArray(L"连接数据库成功......"));
        qDebug()<<"remote database is connected";
    }
    else
    {
        remoteDbStatus=false;
        //        this->statusBar()->showMessage(QString::fromWCharArray(L"连接数据库失败请检查网络连接......")+db_mysql.lastError().text());
        qDebug()<<"error remote db"<<db_mysql.lastError().text();
    }
    setDeviceStatus(ui->lblRemoteDb,remoteDbStatus);
}
#include <qstring>
#include <QtSql>
bool MainWidget::getRemoteDatas(QStringList &tms)
{
    if(db_mysql.isValid())
    {
        QSqlQuery query(db_mysql);
        QString strSql=QString("select tm from %1").arg(remote_table_name); //1
        bool ok=query.exec(strSql);
        if(ok)
        {
            while(query.next())
            {
                QString tmp=query.value("tm").toString();
                tmp= tmp.replace("T"," ");
                tms<<tmp;
            }
        }
        else
        {
            qDebug()<<"error get already tm lists"<<strSql;
        }
        return ok;
    }
    else
        return false;
}
//函数功能：向远程数据库中插入一条记录
bool MainWidget::insertRemoteDb(QStringList tmp)
{
    if(db_mysql.isValid()/*&&(tmp.count()<5)*/)
    {
        if(alreadyTms.contains(tmp.at(tmIndex))&&(alreadyTms.count()))
        {
            qDebug()<<"cur data is already"<<tmp.at(tmIndex);
            return false;
        }
        //        branchTest
        //        id` bigint(20) NOT NULL AUTO_INCREMENT,
        //          `tm` datetime NOT NULL,
        //          `deviceNo` text,
        //          `AVoltage` float(8,0) DEFAULT NULL COMMENT '温度',
        //          `ACurrent` float DEFAULT NULL COMMENT '湿度',
        //          `BVoltage` float DEFAULT NULL,
        //          `BCurrent` float DEFAULT NULL COMMENT '气压',
        //          `CVoltage` float DEFAULT NULL COMMENT '仪表内电压',
        //          `CCurrent` text COMMENT '风速',
        //          `temperature1` float DEFAULT NULL COMMENT '环境温度',
        //          `humidity1` float DEFAULT NULL COMMENT '环境湿度',
        //          `tmp1` text COMMENT '雨量',
        //          `tmp2` text COMMENT '备注1',
        //          `remark` text COMMENT '备注',
        //        ACurrent(轨距),BVoltage(超高),BCurrent(拉出值),CVoltage(导高)
        QSqlQuery query(db_mysql);

        QString strSql=QString("insert into %1 "
                               "(tm,deviceNo,AVoltage,ACurrent,BVoltage,BCurrent,CVoltage)"
                               " values(\'%2\',\'%3\',%4,%5,%6,%7,%8)")
                .arg(remote_table_name) //1
                .arg(tmp.at(tmIndex))     //2 tm              text
                //                .arg(QDateTime::currentDateTime().toString(" yyyy-MM-dd-hh-mm-ss"))     //2 tm              text
                .arg("0101010102")              //3 deviceNo        text
                .arg(tmp.at(poleIndex))         //4 AVoltage        float pole
                .arg(tmp.at(railWidthIndex))    //5 ACurrent        float railwidth
                .arg(tmp.at(railHeightIndex))   //6 BVoltage        float railheight
                .arg(tmp.at(wireWidthIndex))    //7 BCurrent        float wirewidth
                .arg(tmp.at(wireHeightIndex));  //8 CVoltage        float wireheight
        bool ok=query.exec(strSql);
        if(!ok)
            qDebug()<<"error update remote data,sql"<<ok<<tmp.count()<<strSql;
        return ok;
    }
    return false;
}
#include "QSettings"
void MainWidget::updateRemoteSet(void)
{
    QSettings  settings("sets.ini", QSettings::IniFormat);

    bRemoteDb=settings.value("system/remotedb", 1).toBool();
    if(bRemoteDb)
    {
        remoteDbStatus=false;
        connect2MysqlDb();
    }
    else
    {
        qDebug()<<"not need to remote db" ;
    }
}
void MainWidget::on_btnGPS_clicked()
{
    ui->StackedMain->setCurrentIndex(MPT_GPS);
}
void MainWidget::slot_returnMainUI(void)
{
    ui->StackedMain->setCurrentIndex(MPT_Main);
}
curveInfo MainWidget::add1Curve(QCustomPlot *drawWire,QColor color)
{
    curveInfo tmp;
    if(!tmp.curveIndex)
    {
        drawWire->addGraph();
        //        qDebug()<<"cur graph count="<<drawWire->graphCount();
        tmp.curveIndex=drawWire->graphCount()-1;
        drawWire->graph(tmp.curveIndex)->setPen(QPen(color));
    }
    return tmp;
}
#define absGPS 100
//gps图像发达系数
#define constGPS 1000
void MainWidget::slot_drawGps(GPSInfo &gpsValue)
{
    if(gpsValue.bStatus)
    {
        double jingdu=gpsValue.Longitude*constGPS;
        double weidu=gpsValue.Latitude*constGPS;
        if(jingdu)
        {
            if(maxLongitude<jingdu)
                maxLongitude=jingdu;
            if(minLongitude>jingdu)
                minLongitude=jingdu;
        }
        if(weidu)
        {
            if(maxLatitude<weidu)
                maxLatitude=weidu;
            if(minLatitude>weidu)
                minLatitude=weidu;
        }
        ui->lblLatitude->setText(QString::number(gpsValue.Latitude));
        ui->lblLongitude->setText(QString::number(gpsValue.Longitude));
        gpsDraw.x<<jingdu;
        gpsDraw.y<<weidu;
        ui->wdgDrawGPS->graph(gpsDraw.curveIndex)->setData(gpsDraw.x,gpsDraw.y);

        ui->wdgDrawGPS->yAxis->setRange(minLatitude-absGPS,maxLatitude+absGPS);
        ui->wdgDrawGPS->xAxis->setRange(minLongitude-absGPS,maxLongitude+absGPS);
        ui->wdgDrawGPS->replot();
        qDebug()<<"cur gps"<<jingdu<<weidu<<gpsDraw.x.count()<<ui->wdgDrawGPS->xAxis->range()<<ui->wdgDrawGPS->yAxis->range();
    }
    setDeviceStatus(ui->lblGPSStatus,gpsValue.bStatus);
}
//函数功能：用于指示GPS及远程数据库的状态
void MainWidget::setDeviceStatus(QLabel *lbl,bool flag)
{
    if(flag)
    {
        lbl->setStyleSheet("border-image: url(:/res/deviceOn.png)");
    }
    else
    {
        lbl->setStyleSheet("border-image: url(:/res/deviceOff.png)");
    }
}
//函数功能：初始化GPS绘制相关
void MainWidget::gpsInitial(void)
{
    gpsCom=new SerialPort;
    gpsDraw=add1Curve(ui->wdgDrawGPS,Qt::red);
    connect(gpsCom,SIGNAL(updateGps(GPSInfo &)),this,SLOT(slot_drawGps(GPSInfo &)));
    setDeviceStatus(ui->lblGPSStatus,false);
    ui->chbGps->setChecked(false);
    maxLongitude=0;
    minLongitude=10000*constGPS;
    maxLatitude=0;
    minLatitude=10000*constGPS;
    //    ui->wdgDrawGPS->yAxis->setVisible(false);
    //    ui->wdgDrawGPS->xAxis->setVisible(false);
    ui->wdgDrawGPS->setInteractions(QCP::iRangeDrag| QCP::iSelectAxes| QCP::iRangeZoom/**/);
    ui->stkGps->setCurrentIndex(0);
}
void MainWidget::on_chbGps_clicked()
{
    QSettings  settings("sets.ini", QSettings::IniFormat);
    if(settings.value("system/bGpsSim", 1).toBool())
    {
        ui->stkGps->setCurrentIndex(1);
        ui->lblSimGps->setStyleSheet("border-image: url(:/res/simgps.png)");
    }
    else
    {
        qDebug()<<"real gps";
        ui->stkGps->setCurrentIndex(0);
        //        ui->wdgDrawGPS->dr->setStyleSheet("");
        gpsCom->setGpsEnable(ui->chbGps->isChecked());
    }
}
