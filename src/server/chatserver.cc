#include"chatserver.hpp"
#include"json.hpp"
#include"chatservice.hpp"
#include<functional>
#include<string>
using json = nlohmann::json;
ChatServer::ChatServer(EventLoop* loop,const InetAddress& listenAddr,const string& nameArg):
            _loop(loop),
            _server(loop,listenAddr,nameArg)
{
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,std::placeholders::_1));
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
    _server.setThreadNum(4);
}
void ChatServer::start(){
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn){
    //断开连接
    if(!conn->connected()){
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
void ChatServer::onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time){
    string Buffer = buf->retrieveAllAsString();
    //反序列化
    json js =json::parse(Buffer);
    //解耦网络模块和业务模块（利用回调思想
    //通过js["msgid"]获取对应业务handler
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn,js,time);
}