/*
TcpServer
TcpClient
业务代码：用户连接和断开、用户数据读写
*/
#include<muduo/net/TcpClient.h>
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<functional>
#include<string>
using namespace std;
using namespace std::placeholders;
/*
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确参数，输出ChatServer的构造函数
4.注册连接的回调函数和处理数据的回调函数
5.设置线程数
*/

class ChatServer{
    public:
    ChatServer(muduo::net::EventLoop* loop,
        const muduo::net::InetAddress& listenAddr,
        const string& nameArg)
        :_server(loop,listenAddr,nameArg)
        ,_loop(loop){
            //给服务器构造用户连接的创建和断开回调
            _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
            //给服务器注册用户读写事件回调
            _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
            //设置服务器的线程数量
            _server.setThreadNum(4);
        }

    void start(){
        _server.start();
    }
    
    private:
    //处理用户的连接创建和断开
    void onConnection(const muduo::net::TcpConnectionPtr& conn){
        if(conn->connected()){
        cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<"  state:online"<<endl;
        }
        else         {
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<"  state:outline"<<endl;
            conn->shutdown();

    }
        }
    //处理用户的读写
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                    muduo::net::Buffer* buf,
                    muduo::Timestamp time)
                {
                    string buffer = buf->retrieveAllAsString();
                    cout<<"receive data:"<<buffer<<"  time:"<<time.toString()<<endl;
                    conn->send(buffer);
                }

    muduo::net::TcpServer _server;  //#1
    muduo::net::EventLoop *_loop;   //#2  epoll
};

int main(){
    muduo::net::EventLoop loop;
    muduo::net::InetAddress add("127.0.0.1",6000);
    ChatServer server(&loop,add,"Cw");
    server.start();
    loop.loop();
    return 0;
}