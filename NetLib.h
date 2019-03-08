#ifndef NETLIB_H__
#define NETLIB_H__
#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

class MessageStu
{
public:
    unsigned int m_len;
    unsigned int m_id;//1 串口  2视频 3 数据同步(machine->pc) 4 执行SQL语句
                      //5 获取可导入的路线   6 导入路线
                      //7 导出数据 8电机控制
    char* m_msg;
    MessageStu()
    {
        m_len = 0;
        m_id = 0;
        m_msg = 0;
    }
    ~MessageStu()
    {
        if(m_msg)
            delete[] m_msg;
    }
};
class NetConnection;
/**
 * @brief 服务端
 */
class NetLibServer
{
public:
    NetLibServer();
    ~NetLibServer();
public:
    /**
     * @brief 启动服务端
     * @param port 监听的端口号
     * @return 是否启动成功
     */
    bool  Run(int port);

    boost::shared_ptr<NetConnection> GetConnection();

    /**
     * @brief 设置当收到消息后调用的回调函数
     * @param cb 回调函数
     * @return 回调连接器
     */
    void SetOnGetMsgCallBack(boost::function<void(boost::shared_ptr<MessageStu> )> cb);
    static NetLibServer* GetInstance();
private:
    static NetLibServer* m_instance;
    class  Impl;
    Impl* m_impl;
};

/**
 * @brief 网络连接
 */
class   NetConnection
{
public:
    /**
     * @brief 构造函数与析构函数
     */
    NetConnection();
    NetConnection(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);
    ~NetConnection();
    void Disconnect();

public:

    /**
     * @brief 获取当前是否已经连接
     * @return
     */
    bool GetIsConnected();
    bool ConnectTo(const std::string& ip);

public:
    /**
     * @brief 设置当收到消息后调用的回调函数
     * @param cb 回调函数
     * @return 回调连接器
     */
    void SetOnGetMsgCallBack(boost::function<void(boost::shared_ptr<MessageStu> )> cb);
    bool Send(boost::shared_ptr<MessageStu> msg);
public:
    /**
     * @brief 开启读取线程
     */
    void StartReadThread();
private:
    class Impl;
    Impl* m_impl;
};



#endif // NETLIB_H
