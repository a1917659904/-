#include"MysqlConn.h"
#include<iostream>
#include<string>
using namespace std;
int query(){
    MysqlConn conn;
    conn.connect("tao","242411","mydb","127.0.0.1");

    string sql="insert into user values('14163','29857')";
    if(conn.update(sql))cout<<"success"<<endl;
    sql="select * from user";
    conn.query(sql);
    while(conn.next()){
        cout<<conn.value(0)<<","<<conn.value(1)<<endl;
    }
    return 0;
}
int main(){
    query();
}