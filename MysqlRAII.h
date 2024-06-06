#include"ConnectionPool.h"
#include<memory>

//创建对象的时候，在构造函数中传入的第一个参数是一个独占指针，我们申请到的连接会放在这个
//独占指针中
//第二个参数是我们的数据库连接池
class MysqlRAII{
public:
    MysqlRAII(shared_ptr<MysqlConn>&sql,ConnectionPool& connpool):_connpool(connpool){
        sql=connpool.getConnection();
        _sql=sql;
    }
    ~MysqlRAII(){
        //ConnectionPool::getConnectionPool().freeConnection(_sql);
       _connpool.freeConnection(_sql);
    }
private:
    shared_ptr<MysqlConn>_sql;
    ConnectionPool& _connpool;
};