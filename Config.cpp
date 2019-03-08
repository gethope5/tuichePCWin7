#include "Config.h"

#include <QDebug>
Config* Config::m_instance = NULL;

void Config::Load()
{
    try
    {
        boost::property_tree::ptree pt, pt_center_config, pt_record_config,pt_moto_config;
        boost::property_tree::read_xml("./config.xml", pt);
        pt_center_config = pt.get_child("Root.CenterConfig");
        pt_record_config = pt.get_child("Root.RecordConfig");
        pt_moto_config = pt.get_child("Root.MotoConfig");
        m_center_config.Load(pt_center_config);
        m_record_config.Load(pt_record_config);
        m_moto_config.Load(pt_moto_config);
    }
    catch(...)
    {
        m_center_config.Reset();
        m_record_config.Reset();
        m_moto_config.Reset();
        Save();
    }
}

void Config::Save()
{
    try
    {
        boost::property_tree::ptree pt, pt_center_config, pt_record_config, pt_moto_config;
        m_center_config.Save(pt_center_config);
        m_record_config.Save(pt_record_config);
        m_moto_config.Save(pt_moto_config);
        pt.add_child("Root.CenterConfig", pt_center_config);
        pt.add_child("Root.RecordConfig", pt_record_config);
        pt.add_child("Root.MotoConfig", pt_moto_config);
        boost ::property_tree :: write_xml(
                 "config.xml" , pt ,
                 std ::locale (),
                 boost ::property_tree :: xml_writer_make_settings <std :: string>( ' ' , 4 ));

    }
    catch(...)
    {
        m_center_config.Reset();
        m_record_config.Reset();
        m_moto_config.Reset();
    }
}

Config *Config::GetInstance()
{
    if(!m_instance)
    {
        m_instance = new Config();
    }
    return m_instance;
}

Config::Config()
{
    Load();
}

void CenterConfig::Reset()
{
    m_h_pos = 0;
    m_v_pos = 0;
}

void CenterConfig::Load(boost::property_tree::ptree &pt)
{
    m_h_pos = pt.get<int>("m_h_pos");
    m_v_pos = pt.get<int>("m_v_pos");
}

void CenterConfig::Save(boost::property_tree::ptree &pt)
{
    pt.put("m_h_pos", m_h_pos);
    pt.put("m_v_pos", m_v_pos);
}
//6791306193606##

void RecordConfig::Reset()
{
    m_check_mode = CM_Common;//检测模式
    m_xl_name = "";
    m_xj_name = "";
    m_cj_name = "";
    m_gq_name = "";
    m_zq_name = "";
    m_zzh_name = "";
}

void RecordConfig::Load(boost::property_tree::ptree &pt)
{
    m_check_mode = (CheckMode)pt.get<int>("m_check_mode");//检测模式
    m_xl_name = QString::fromUtf8(pt.get<std::string>("m_xl_name").c_str());
    m_xj_name = QString::fromUtf8(pt.get<std::string>("m_xj_name").c_str());
    m_cj_name = QString::fromUtf8(pt.get<std::string>("m_cj_name").c_str());
    m_gq_name = QString::fromUtf8(pt.get<std::string>("m_gq_name").c_str());
    m_zq_name = QString::fromUtf8(pt.get<std::string>("m_zq_name").c_str());
    m_zzh_name = QString::fromUtf8(pt.get<std::string>("m_zzh_name").c_str());
}

void RecordConfig::Save(boost::property_tree::ptree &pt)
{
    pt.put("m_check_mode", (int)m_check_mode);//检测模式
    pt.put("m_xl_name", std::string(m_xl_name.toUtf8().data()));
    pt.put("m_xj_name", std::string(m_xj_name.toUtf8().data()));
    pt.put("m_cj_name", std::string(m_cj_name.toUtf8().data()));
    pt.put("m_gq_name", std::string(m_gq_name.toUtf8().data()));
    pt.put("m_zq_name", std::string(m_zq_name.toUtf8().data()));
    pt.put("m_zzh_name", std::string(m_zzh_name.toUtf8().data()));
}

void MotoConfig::Reset()
{
    m_s_moto_speed = 100;
    m_b_moto_speed = 500;
    m_touch_scale = 10;
    m_moto_run_step = 500;
}

void MotoConfig::Load(boost::property_tree::ptree &pt)
{
    m_s_moto_speed = pt.get<int>("m_s_moto_speed");
    m_b_moto_speed = pt.get<int>("m_b_moto_speed");
    m_touch_scale = pt.get<int>("m_touch_scale");
    m_moto_run_step = pt.get<int>("m_moto_run_step");
}

void MotoConfig::Save(boost::property_tree::ptree &pt)
{
    pt.put("m_s_moto_speed", m_s_moto_speed);
    pt.put("m_b_moto_speed", m_b_moto_speed);
    pt.put("m_touch_scale", m_touch_scale);
    pt.put("m_moto_run_step", m_moto_run_step);
}
