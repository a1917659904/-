#include"ConnectionPool.h"
#include"MysqlConn.h"
#include"MysqlRAII.h"
#include<iostream>
#include<chrono>
using namespace std;
using namespace chrono;

//单线程
void op1(int begin,int end){
    for(int i=begin;i<end;i++){
        MysqlConn conn;
        conn.connect("tao","242411","mydb","127.0.0.1");
        char sql[1024]={0};
        sprintf(sql,"insert into user values('%d','%d')",i,i);
        if(conn.update(sql));
    }
}

void op2(ConnectionPool&pool, int begin,int end){
    for(int i=begin;i<end;i++){
        //shared_ptr<MysqlConn> conn=pool.getConnection();
        shared_ptr<MysqlConn> conn;
        MysqlRAII raii(conn,pool);
        char sql[1024]={0};
        sprintf(sql,"insert into user values('%d','%d')",i,i);
        if(conn->update(sql));
    }
}

void test1(){
    steady_clock::time_point begin=steady_clock::now();

    op1(3500,4000);
    steady_clock::time_point end=steady_clock::now();
    auto length=end-begin;
    cout<<"单线程"<<length.count()<<endl;
}

void test2(){
    ConnectionPool &connpool=ConnectionPool::getConnectionPool();
    steady_clock::time_point begin=steady_clock::now();
    op2(connpool,4001,4500);
    steady_clock::time_point end=steady_clock::now();
    auto length=end-begin;
    cout<<"连接池"<<length.count()<<endl;
}
int main(){

    test1();
    test2();
}