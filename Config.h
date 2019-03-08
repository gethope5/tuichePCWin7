#ifndef CONFIG_H
#define CONFIG_H
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <QString>
class CenterConfig
{
public:
    int m_h_pos;
    int m_v_pos;
    void Reset();
    void Load(boost::property_tree::ptree& pt);
    void Save(boost::property_tree::ptree& pt);
    CenterConfig()
    {

    }
};

class RecordConfig
{
public:
    enum CheckMode{
        CM_Common = 0,//普通模式
        CM_Record//记录模式
    };
    int m_check_mode;//检测模式
    QString m_xl_name;
    QString m_xj_name;
    QString m_cj_name;
    QString m_gq_name;
    QString m_zq_name;
    QString m_zzh_name;

    void Reset();
    void Load(boost::property_tree::ptree& pt);
    void Save(boost::property_tree::ptree& pt);
    RecordConfig()
    {

    }
};

class MotoConfig
{
public:
    int m_s_moto_speed;
    int m_b_moto_speed;
    int m_touch_scale;    //触摸屏像素与电机脉冲数之间的比例
    int m_moto_run_step;  //连续运动时的时间间隔
    void Reset();
    void Load(boost::property_tree::ptree& pt);
    void Save(boost::property_tree::ptree& pt);
    MotoConfig()
    {

    }
};

class Config
{
public:
    void Load();
    void Save();
public:
    CenterConfig m_center_config;
    RecordConfig m_record_config;
    MotoConfig m_moto_config;
public:
    static Config* GetInstance();
private:
    Config();
private:
    static Config* m_instance;
};

#endif // CONFIG_H
