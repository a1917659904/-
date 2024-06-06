#ifndef CONNECTIONPOOL
#define CONNECTIONPOOL
#include<json/json.h>
#include<queue>
#include<mutex>
#include<fstream>
#include<memory>
#include<thread>
#include<condition_variable>
#include<chrono>
#include<atomic>
#include"MysqlConn.h"
using namespace Json;
using namespace std;

class ConnectionPool{
public:
    ~ConnectionPool();
    static ConnectionPool& getConnectionPool();
    shared_ptr<MysqlConn>getConnection();//从连接池中取出一个连接
    void freeConnection(shared_ptr<MysqlConn>conn);
private:
    ConnectionPool();
    ConnectionPool(const ConnectionPool&)=delete;
    ConnectionPool& operator=(const ConnectionPool&)=delete;
    bool parseJsonFile();//解析json
    void produceConnection();//动态创建连接
    void recycleConnection();//动态销毁连接
    void addConnection();//创建连接
    string m_ip;//ip地址
    string m_user;//用户名
    string m_passwd;//数据库密码
    string m_db_name;//数据库名
    unsigned short m_port;//端口号
    int m_min_size;//最小连接数
    int m_max_size;//最大连接数
    int m_timeout;//超时时长,也就是排队时长
    int m_max_idle_time;//最大空闲时长
    //下次超时的绝对时间,每次取出数据库连接后，替换为新的队头元素的最后一次使用时间+最大空闲时间
    chrono::steady_clock::time_point m_next_timeout;
    queue<shared_ptr<MysqlConn>>m_connectionQ;//连接队列
    mutex m_mutex;//互斥锁
    condition_variable m_produce_cv;//生产者条件变量
    condition_variable m_consume_cv;//消费者条件变量
    atomic<bool>m_stop;
};
ConnectionPool::ConnectionPool()
{   
    //加载数据对象
    m_stop.store(false);
    if(!parseJsonFile()){
        return;
    }
    for(int i=0;i<m_min_size;i++){
        addConnection();
    }
    m_next_timeout=chrono::steady_clock::now();
    thread producer(&ConnectionPool::produceConnection,this);
    thread recycler(&ConnectionPool::recycleConnection,this);
    producer.detach();
    recycler.detach();
}
ConnectionPool &ConnectionPool::getConnectionPool()
{
    static ConnectionPool instance;
    return instance;
}
shared_ptr<MysqlConn> ConnectionPool::getConnection()
{   
    {
        unique_lock<mutex>locker(m_mutex);
        if(m_connectionQ.empty()){
            m_produce_cv.notify_one();
            m_consume_cv.wait_for(locker,chrono::milliseconds(m_timeout));
            if(m_connectionQ.empty())
                //可以添加判空逻辑如果为空可以在这里添加日志
                return nullptr;
        } 
    }
    shared_ptr<MysqlConn> sql;
    {
        lock_guard<mutex>locker(m_mutex);
        sql=m_connectionQ.front();
        m_connectionQ.pop();
    }
    return sql;
}
void ConnectionPool::freeConnection(shared_ptr<MysqlConn>conn)
{
    unique_lock<mutex>locker(m_mutex);
    if(m_connectionQ.size()<m_max_size){
        m_connectionQ.emplace(conn);
    }
}
bool ConnectionPool::parseJsonFile()
{
    ifstream ifs("dbconfig.json");
    Reader rd;
    Value root;
    //把读取到的ifs中的内容存到root
    rd.parse(ifs,root);
    if(root.isObject()){
        m_ip=root["ip"].asString();
        m_port=root["port"].asInt();
        m_user=root["username"].asString();
        m_passwd=root["password"].asString();
        m_db_name=root["dbName"].asString();
        m_min_size=root["minSize"].asInt();
        m_max_size=root["maxSize"].asInt();
        m_max_idle_time=root["maxIdleTime"].asInt();
        m_timeout=root["timeout"].asInt();
        return true;
    }
    return false;
}

ConnectionPool::~ConnectionPool()
{
    m_stop.store(true);
    m_consume_cv.notify_all();
    m_produce_cv.notify_all();
    while(!m_connectionQ.empty()){
        m_connectionQ.pop();
    }
}


void ConnectionPool::produceConnection()
{
    while(!m_stop){
        unique_lock<mutex>locker(m_mutex);
        //如果当前连接数量小于最小值直接创建，如果大于的话，等待通知在创建
        if(m_connectionQ.size()<m_min_size){
            addConnection();
        }
        else{
            m_produce_cv.wait(locker,[this](){
                return m_connectionQ.size()<m_max_size;
            });
            for(int i=0;i<500&&m_connectionQ.size()<m_max_size;i++){
                addConnection();
            }
            
        }
        //生产者生产以后，通知所有消费者，也就是销毁连接的线程，销毁函数会判断当前是否需要销毁。
        m_consume_cv.notify_one();
        
    }
}

void ConnectionPool::recycleConnection()
{
    while(!m_stop){
        
        /*if(m_connectionQ.size()>m_min_size&&m_next_timeout<=chrono::steady_clock::now()){
            //取出当前节点
            shared_ptr<MysqlConn>ptr=move(m_connectionQ.front());
            m_connectionQ.pop();
            m_next_timeout=m_connectionQ.front()->getLastUseTime()+chrono::milliseconds(m_max_idle_time);
        }*/
        //当前连接小于最小连接数就阻塞线程，大于的话等待超时通知再销毁
        this_thread::sleep_for(chrono::steady_clock::now()-m_next_timeout);
        unique_lock<mutex>locker(m_mutex);
        if(m_connectionQ.size()>m_min_size&&m_next_timeout<=chrono::steady_clock::now()){
            m_connectionQ.pop();
            m_next_timeout=m_connectionQ.front()->getLastUseTime()+chrono::milliseconds(m_max_idle_time);
        }
        
    }
}

void ConnectionPool::addConnection()
{
        shared_ptr<MysqlConn> conn=make_unique<MysqlConn>();
        conn->connect(m_user,m_passwd,m_db_name,m_ip,m_port);
        m_connectionQ.push(conn);
}


#endif