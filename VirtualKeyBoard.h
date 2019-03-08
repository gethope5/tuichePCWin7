#ifndef VIRTUALKEYBOARD_H
#define VIRTUALKEYBOARD_H

#include <QWidget>
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#endif
namespace Ui {
class VirtualKeyBoard;
}

class VirtualKeyBoard : public QWidget
{
    Q_OBJECT

public:
    explicit VirtualKeyBoard(QWidget *parent = 0);
    ~VirtualKeyBoard();
public:
    void Init(boost::function<void(QString)> text_out_callback, boost::function<void()> delete_callback);
private slots:
    void on_btnOK_clicked();
    void on_btn_delete_clicked();
    void on_btn1_clicked();
    void on_btn2_clicked();
    void on_btn3_clicked();
    void on_btn4_clicked();
    void on_btn5_clicked();
    void on_btn6_clicked();
    void on_btn7_clicked();
    void on_btn8_clicked();
    void on_btn9_clicked();
    void on_btn0_clicked();
private:
    Ui::VirtualKeyBoard *ui;
    boost::function<void(QString)> m_on_text_out;
    boost::function<void()> m_on_delete;
};

#endif // VIRTUALKEYBOARD_H
