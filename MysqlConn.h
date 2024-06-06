#ifndef MYSQLCONN
#define MYSQLCONN
#include<mysql/mysql.h>
#include<string>
#include<chrono>
using namespace std;
class MysqlConn{
public:
    //初始化数据库连接
    MysqlConn();
    //释放数据库连接
    ~MysqlConn();
    //连接数据库
    //创建成功返回true
    bool connect(string user, string passwd, string dbName,string ip,unsigned short port=3306);
    //更新数据库：insert，update，delete
    bool update(string sql);    
    //查询数据库
    bool query(string sql);
    //遍历查询得到的结果集
    bool next();
    //得到结果集中的字段值
    string value(int index);
    //事务操作
    bool transcation();
    //提交事务
    bool commit();
    //事务回滚
    bool rollback();
    //获取当前最后一次使用时间
    chrono::steady_clock::time_point getLastUseTime();
private:
    //清空结果
    void freeResult();
    //更新最后使用时间
    void updateLastUseTime();
    MYSQL* m_conn;
    MYSQL_RES* m_result;
    MYSQL_ROW m_row;
    chrono::steady_clock::time_point m_last_use_time;
};

MysqlConn::MysqlConn(){
    m_conn=mysql_init(nullptr);
    m_result=nullptr;
    mysql_set_character_set(m_conn,"utf8");
    m_last_use_time = chrono::steady_clock::now();
}

MysqlConn::~MysqlConn(){
    if(m_conn!=nullptr)mysql_close(m_conn);
    freeResult();
}
bool MysqlConn::connect(string user, string passwd, string dbName,string ip,unsigned short port){
    MYSQL* ptr=mysql_real_connect(m_conn,ip.c_str(),user.c_str(),passwd.c_str(),dbName.c_str(),port, nullptr,0);
    return ptr!=nullptr;
}

bool MysqlConn::update(string sql)
{
    if(m_conn==nullptr)return false;
    if(mysql_query(m_conn,sql.c_str())) return false;
    return true;
}

bool MysqlConn::query(string sql)
{
    freeResult();//清空上一次结果
    if(mysql_query(m_conn,sql.c_str()))return false;
    m_result=mysql_store_result(m_conn);
    return true;
}

bool MysqlConn::next()
{
    if(m_result!=nullptr){
        m_row=mysql_fetch_row(m_result);
        if(m_row!=nullptr)return true;
    }
    return false;
}

string MysqlConn::value(int index)
{
    int rowCount=mysql_num_fields(m_result);
    if(index>=rowCount||index<0){
        return string();
    }
    char *val=m_row[index];
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    return string(val,length);
}

bool MysqlConn::transcation()
{
    return mysql_autocommit(m_conn,false);
}

bool MysqlConn::commit()
{
    return mysql_commit(m_conn);
}

bool MysqlConn::rollback()
{
    return mysql_rollback(m_conn);
}

 chrono::steady_clock::time_point MysqlConn::getLastUseTime()
{
    return m_last_use_time;
}

void MysqlConn::freeResult()
{ 
    if(m_result){
        mysql_free_result(m_result);
        m_result=nullptr;
    }
}

void MysqlConn::updateLastUseTime()
{
    m_last_use_time=chrono::steady_clock::now();
}
#endif