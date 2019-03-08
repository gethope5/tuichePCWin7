#include "NetLib.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/lock_guard.hpp>
#include <QDebug>
#include <QString>
QString str = QString::fromWCharArray(L"1234");
#define DebugOut() qDebug()<<__FILE__<<":"<<__FUNCTION__<<":"

#define QS(x)      QString::fromWCharArray(L#x)



class NetLibServer::Impl
{
public:
    Impl();
    ~Impl(){
        delete m_ios_work;
        m_ios.stop();
        m_ios_thread.join();
    }
public:
    bool Run(int port);
public:
    void OnAccept(const boost::system::error_code& ec, boost::shared_ptr<boost::asio::ip::tcp::socket> socket);
    boost::shared_ptr<NetConnection> GetConnection();
    boost::function<void (boost::shared_ptr<MessageStu>)> OnGetMsgCallBack;//当接收到新消息是调用
    void OnGetMsgCallBackFunc(boost::shared_ptr<MessageStu> msg);
private:
    boost::asio::io_service m_ios;                  //asio
    boost::asio::io_service::work* m_ios_work;       //保证asio不退出
    boost::thread m_ios_thread;                     //asio线程
    boost::thread m_lession_thread;
    boost::asio::ip::tcp::acceptor m_ios_accpetor;
    boost::shared_ptr<NetConnection> m_connecttion;

};

NetLibServer::Impl::Impl():m_ios_work(new boost::asio::io_service::work(m_ios)),m_ios_accpetor(m_ios)
{
    m_ios_thread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_ios));
    m_lession_thread = boost::thread(boost::bind(&NetLibServer::Impl::Run, this, 2211));
}

bool NetLibServer::Impl::Run(int port)
{
    try
    {
        /*boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
        m_ios_accpetor.open(endpoint.protocol());
        m_ios_accpetor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        m_ios_accpetor.bind(endpoint);
        m_ios_accpetor.listen();*/

        boost::system::error_code ec;
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
        //const boost::asio::ip::tcp::protocol_type protocol = endpoint.protocol();
        m_ios_accpetor.open(endpoint.protocol(), ec);
        boost::asio::detail::throw_error(ec, "open");
        //if (reuse_addr)
        {
          m_ios_accpetor.set_option(
              boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
          boost::asio::detail::throw_error(ec, "set_option");
        }
        m_ios_accpetor.bind(endpoint, ec);
        boost::asio::detail::throw_error(ec, "bind");

        m_ios_accpetor.listen(
            boost::asio::ip::tcp::acceptor::max_connections, ec);
        boost::asio::detail::throw_error(ec, "listen");



        boost::shared_ptr<boost::asio::ip::tcp::socket> socket =
                boost::shared_ptr<boost::asio::ip::tcp::socket>(new boost::asio::ip::tcp::socket(m_ios));
        m_ios_accpetor.async_accept(*socket, boost::bind(&NetLibServer::Impl::OnAccept, this, boost::asio::placeholders::error, socket));
    }
    catch(boost::system::error_code& ec)
    {
        qDebug()<<"NetLibServer start faild:"<<ec.message().c_str();
        return false;
    }
    catch(...)
    {
        qDebug()<<"NetLibServer start faild";
        return false;
    }
    return true;
}

void NetLibServer::Impl::OnAccept(const boost::system::error_code &ec, boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    if(ec)
    {
        qDebug()<<"NetLibServer accept error:"<<QString::fromUtf8(ec.message().c_str());
        return;
    }
    //接收到套接字，将其放入连接管理器中
    socket->set_option(boost::asio::ip::tcp::no_delay(true));
    //m_connect_manager.AddConnection(socket);
    if(m_connecttion)
        m_connecttion.reset();
    m_connecttion = boost::shared_ptr<NetConnection>(new NetConnection(socket));
    m_connecttion->SetOnGetMsgCallBack(OnGetMsgCallBack);
    boost::shared_ptr<boost::asio::ip::tcp::socket> new_socket =
            boost::shared_ptr<boost::asio::ip::tcp::socket>(new boost::asio::ip::tcp::socket(m_ios));
    //boost::system::error_code err;

    m_ios_accpetor.async_accept(*new_socket, boost::bind(&NetLibServer::Impl::OnAccept, this, boost::asio::placeholders::error, new_socket));
    //m_ios_accpetor.async_accept(*socket, boost::bind(&NetLibServer::Impl::OnAccept, this, boost::asio::placeholders::error, socket));
}

boost::shared_ptr<NetConnection> NetLibServer::Impl::GetConnection()
{
    return m_connecttion;
}

void NetLibServer::Impl::OnGetMsgCallBackFunc(boost::shared_ptr<MessageStu> msg)
{
    if(OnGetMsgCallBack)
        OnGetMsgCallBack(msg);
}



NetLibServer* NetLibServer::m_instance = NULL;
NetLibServer::NetLibServer()
{
    m_impl = new Impl();
}

NetLibServer::~NetLibServer()
{
    delete m_impl;
    m_impl = NULL;
}

bool NetLibServer::Run(int port)
{
    return m_impl->Run(port);
}

boost::shared_ptr<NetConnection> NetLibServer::GetConnection()
{
    return m_impl->GetConnection();
}

void NetLibServer::SetOnGetMsgCallBack(boost::function<void (boost::shared_ptr<MessageStu>)> cb)
{
    m_impl->OnGetMsgCallBack = cb;
}

NetLibServer *NetLibServer::GetInstance()
{
    if(!m_instance)m_instance = new NetLibServer;
    return m_instance;
}






//========================================================
//client
//========================================================
class NetConnection::Impl
{
public:
    Impl();
    Impl(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);
    ~Impl();
    void Disconnect();
    bool Connect(const std::string &ip_addr);
    bool GetIsConnected();

    bool Send(boost::shared_ptr<MessageStu> msg);
    void StartReadThread();
public:
    boost::function<void(boost::shared_ptr<MessageStu> ) > OnGetMsgCb;
private:
    void ReadThread();
    void StopThread();
    void OnError(const QString& str);
private:
    boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
    boost::asio::io_service m_ios;
    boost::thread m_read_thread;
    boost::mutex m_send_mutex_map_mutex;//m_send_mutex_map访问锁
    boost::mutex m_send_mutex;//发送函数同时只能进行一个

};


NetConnection::Impl::Impl()
{
    m_socket = boost::shared_ptr<boost::asio::ip::tcp::socket>(new boost::asio::ip::tcp::socket(m_ios));
}

NetConnection::Impl::Impl(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    m_socket = socket;
    m_read_thread = boost::thread(boost::bind(&NetConnection::Impl::ReadThread, this));
}

NetConnection::Impl::~Impl()
{
    m_socket->close();
    StopThread();
}


bool NetConnection::Impl::GetIsConnected()
{
    return m_socket->is_open();
}

void NetConnection::Impl::Disconnect()
{
    m_socket->close();
}

bool NetConnection::Impl::Connect(const std::string &ip_addr)
{
    boost::asio::ip::tcp::endpoint end_point(boost::asio::ip::address::from_string("192.168.230.101"), 2211);
    //boost::asio::ip::tcp::endpoint end_point(boost::asio::ip::address::from_string("192.168.199.186"), 2211);
    try
    {
        m_socket->connect(end_point);
        m_socket->set_option(boost::asio::ip::tcp::no_delay(true));
        m_read_thread = boost::thread(boost::bind(&NetConnection::Impl::ReadThread, this));
        return true;
    }
    catch(...)
    {
        return false;
    }
}

bool NetConnection::Impl::Send(boost::shared_ptr<MessageStu> msg)
{
    int msg_len = sizeof(MessageStu) - 4 + msg->m_len;
    int send_idx = 0;
    char * buf = new char[msg_len];
    memcpy(buf, msg.get(), sizeof(MessageStu) - 4);
    memcpy(buf+sizeof(MessageStu) - 4, msg->m_msg, msg->m_len);
    try
    {
        boost::lock_guard<boost::mutex> lk(m_send_mutex);
        while(send_idx != msg_len)
        {
            send_idx+=boost::asio::write(*m_socket,  boost::asio::buffer(buf+send_idx, msg_len-send_idx));
        }
    }
    catch(boost::system::error_code& ec)
    {
        //OnError(QString::fromWCharArray(L"发送信息失败,错误为:")+ec.message().c_str());
        return false;
    }
    catch(...)
    {
        //OnError(QString::fromWCharArray(L"发送信息失败"));
        return false;
    }
    return true;
}


void NetConnection::Impl::ReadThread()
{
    while(1)
    {
        try
        {
            //读取消息头
            int len = sizeof(MessageStu)-sizeof(char*);
            int idx = 0;
            boost::shared_ptr<MessageStu> buf = boost::shared_ptr<MessageStu>(new MessageStu());
            while(idx != len)
            {
                idx += m_socket->read_some(boost::asio::buffer((char*)buf.get()+idx, len-idx));
            }
            buf->m_msg = new char[buf->m_len];
            //读取消息内容
            len = buf->m_len;
            idx = 0;
            while(idx != len)
            {
                idx += m_socket->read_some(boost::asio::buffer((char*)buf->m_msg+idx, len-idx));
            }

            if(OnGetMsgCb)OnGetMsgCb(buf);
        }
        catch(...)
        {
            m_socket->close();
            //线程即将结束，也不会继续用到类里的资源，为了避免死锁，先释放线程
            m_read_thread.detach();
            break;
        }
    }
}

void NetConnection::Impl::StopThread()
{
    m_socket->close();
    this->m_read_thread.join();
    m_socket.reset();
}

NetConnection::NetConnection()
{
    m_impl = new Impl();
}

NetConnection::NetConnection(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    m_impl = new Impl(socket);
}

NetConnection::~NetConnection()
{
    delete m_impl;
    m_impl = NULL;
}

void NetConnection::Disconnect()
{
    return m_impl->Disconnect();
}

bool NetConnection::GetIsConnected()
{
    return m_impl->GetIsConnected();
}

bool NetConnection::ConnectTo(const std::string &ip)
{
    return m_impl->Connect(ip);
}


bool NetConnection::Send(boost::shared_ptr<MessageStu> msg)
{
    return m_impl->Send(msg);
}

void NetConnection::SetOnGetMsgCallBack(boost::function<void(boost::shared_ptr<MessageStu> )> cb)
{
    m_impl->OnGetMsgCb = (cb);
}





