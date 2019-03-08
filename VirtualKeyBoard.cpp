#include "VirtualKeyBoard.h"
#include "ui_VirtualKeyBoard.h"

VirtualKeyBoard::VirtualKeyBoard(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VirtualKeyBoard)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);//去掉标题栏
}

VirtualKeyBoard::~VirtualKeyBoard()
{
    delete ui;
}

void VirtualKeyBoard::Init(boost::function<void (QString)> text_out_callback, boost::function<void ()> delete_callback)
{
    m_on_text_out = text_out_callback;
    m_on_delete = delete_callback;
}

void VirtualKeyBoard::on_btnOK_clicked()
{
    this->hide();
}

void VirtualKeyBoard::on_btn_delete_clicked()
{
    if(m_on_delete)
    {
        m_on_delete();
    }
}

void VirtualKeyBoard::on_btn1_clicked(){
    if(m_on_text_out)m_on_text_out("1");
}
void VirtualKeyBoard::on_btn2_clicked(){
    if(m_on_text_out)m_on_text_out("2");
}
void VirtualKeyBoard::on_btn3_clicked(){
    if(m_on_text_out)m_on_text_out("3");
}
void VirtualKeyBoard::on_btn4_clicked(){
    if(m_on_text_out)m_on_text_out("4");
}
void VirtualKeyBoard::on_btn5_clicked(){
    if(m_on_text_out)m_on_text_out("5");
}
void VirtualKeyBoard::on_btn6_clicked(){
    if(m_on_text_out)m_on_text_out("6");
}
void VirtualKeyBoard::on_btn7_clicked(){
    if(m_on_text_out)m_on_text_out("7");
}
void VirtualKeyBoard::on_btn8_clicked(){
    if(m_on_text_out)m_on_text_out("8");
}
void VirtualKeyBoard::on_btn9_clicked(){
    if(m_on_text_out)m_on_text_out("9");
}
void VirtualKeyBoard::on_btn0_clicked(){
    if(m_on_text_out)m_on_text_out("0");
}
