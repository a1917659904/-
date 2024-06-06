# -
C++数据库连接池，封装了sql的c接口，使用C++智能指针、互斥锁、条件变量、多线程、STL队列、RAII思想实现，连接的生产和获取采用生产者-消费者模型

* 使用智能指针和STL队列封装数据库连接
* 使用unique_lock实现队列的互斥访问
* 使用条件变量实现生产者和消费者线程的阻塞和唤醒
* 基于RAII思想的MysqlRAII类获取连接资源，并自动释放和归还数据库连接。

testMysqlConn.cpp测试封装的Mysql是否能正常运行
TestConnectionPool.cpp测试线程池和非线程池性能差别
