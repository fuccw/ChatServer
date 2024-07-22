#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include<functional>
#include<mutex>
#include<vector>
#include"usermodel.hpp"
#include"json.hpp"
#include"friendmodel.hpp"
#include"offlinemessagemodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"
using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;
//处理消息的事件回调
using MsgHandler = std::function<void(const TcpConnectionPtr& conn,json& js,muduo::Timestamp time)>;
//业务层
//业务类
class ChatService
{
public:
    //获取单例对象的接口 
    static ChatService* instance();
    //登录业务
    void login(const TcpConnectionPtr& conn,json& js,muduo::Timestamp time);
    //注册业务
    void reg(const TcpConnectionPtr& conn,json& js,muduo::Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //异常退出处理
    void clientCloseException(const TcpConnectionPtr& conn);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //强制退出，业务重置
    void reset();
    //添加好友业务
    void addFriend(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //创造群组
    void createGroup(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //加入群组
    void addGroup(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //群组聊天
    void groupChat(const TcpConnectionPtr& conn,json& js,Timestamp time);
    //注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);
private:
    ChatService();
    //存储msgid和对应的业务处理方法
    std::unordered_map<int,MsgHandler> _msgHandlerMap;
    //存储在线用户的通信连接
    std::unordered_map<int,TcpConnectionPtr> _userConnectionMap;
    //互斥锁，ConnectionMap线程安全
    std::mutex _connMutex;
    //数据操作类对象
    UserModel _userModel;
    offlineMessageModel _offMsgModel;
    friendModel _friendModel;
    GroupModel _groupModel;
    Redis _redis;
};



#endif