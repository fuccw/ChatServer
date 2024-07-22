#include"chatservice.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h>
ChatService* ChatService::instance(){
    static ChatService service;
    return &service;
}
ChatService::ChatService(){
    _msgHandlerMap.insert({LOGIN_MSG,
        [this](const TcpConnectionPtr& conn, json& js, muduo::Timestamp time) {
            this->login(conn, js, time);
        }
    });
    _msgHandlerMap.insert({REG_MSG,
        [this](const TcpConnectionPtr& conn, json& js, muduo::Timestamp time) {
            this->reg(conn, js, time);
        }
    });
    _msgHandlerMap.insert({ONE_CHAT_MESSAGE,
        [this](const TcpConnectionPtr& conn, json& js, muduo::Timestamp time) {
            this->oneChat(conn, js, time);
        }
    });
    _msgHandlerMap.insert({ADD_FRIEND_MSG,
        [this](const TcpConnectionPtr& conn, json& js, muduo::Timestamp time) {
            this->addFriend(conn, js, time);
        }
    });
    _msgHandlerMap.insert({LOGINOUT_MSG,
        [this](const TcpConnectionPtr& conn, json& js, muduo::Timestamp time) {
            this->loginout(conn, js, time);
        }
    });

    //连接redis
    if(_redis.connect()){
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,std::placeholders::_1,std::placeholders::_2));
    }
}


MsgHandler ChatService::getHandler(int msgid){
    if(_msgHandlerMap.find(msgid)==_msgHandlerMap.end()){
        return [=](const TcpConnectionPtr& conn,json& js,muduo::Timestamp time){
            LOG_ERROR<<"msgid:"<<msgid<<"can't find handler!";
        };
    }
    else return _msgHandlerMap[msgid];
}
//登录业务  
void ChatService::login(const TcpConnectionPtr& conn,json& js,muduo::Timestamp time){
    LOG_INFO<<"do login service!";
    int id = js["id"];
    std::string pwd = js["password"];
    User user = _userModel.query(id);
    if(user.getId()==id&&user.getPwd()==pwd){
        if(user.getState()=="online"){
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "The user has logged in!";
            conn->send(response.dump());
            return;
        }
        //登录成功

        //记录连接
        {
            std::lock_guard<std::mutex> lock(_connMutex);
            _userConnectionMap.insert({id,conn});
        }
        //向redis订阅channel
        _redis.subscribe(id);

        //设置状态
        user.setState("online");
        _userModel.updateState(user);
        //发送消息
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        response["name"] = user.getName();
        //查询离线消息
        std::vector<std::string> msgvec = _offMsgModel.query(id);
        std::vector<User> friendvec = _friendModel.query(id);
        if(!msgvec.empty()){
            response["offlinemsg"] = msgvec;
            _offMsgModel.remove(id);
        }
        //查询好友列表
        if(!friendvec.empty()){
            std::vector<std::string> tempvec;
            for(User &user:friendvec){
                json js;
                js["id"] = user.getId();
                js["name"] = user.getName();
                js["state"] = user.getState();
                tempvec.push_back(js.dump());
            }
            response["friends"] = tempvec;
        }
        //查询群组列表
        std::vector<Group> groupuserVec = _groupModel.queryGroups(id);
        if(!groupuserVec.empty()){
            std::vector<std::string> groupVec;
            for(Group& group:groupuserVec){
                json grpjson;
                grpjson["id"]=group.getId();
                grpjson["groupname"] = group.getName();
                grpjson["groupdesc"] = group.getDesc();
                std::vector<std::string> userVec;
                for(GroupUser& user:group.getUsers()){
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    js["role"] = user.getRole();
                    userVec.push_back(js.dump());
                }
                grpjson["users"]=userVec;
                groupVec.push_back(grpjson.dump());
            }
            response["groups"] = groupVec;
        }

        conn->send(response.dump());
    }
    else{
        //登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "Username or password incorrect!";
        conn->send(response.dump());
    }
}

//注册业务
void ChatService::reg(const TcpConnectionPtr& conn,json& js,muduo::Timestamp time){
    LOG_INFO<<"do register service!";
    std::string name = js["name"];
    std::string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if(state){
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else{
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
}

//连接断开业务
void ChatService::clientCloseException(const TcpConnectionPtr& conn){
    User user;
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        for(auto it = _userConnectionMap.begin();it!=_userConnectionMap.end();it++){
            if(it->second==conn){
                user.setId(it->first);
                _userConnectionMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId()); 

    if(user.getId()!=-1){
        user.setState("offline");
        _userModel.updateState(user);
    }
}

//发送消息
void ChatService::oneChat(const TcpConnectionPtr& conn,json& js,Timestamp time){
    int toId = js["to"];
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(toId);
        if(it!=_userConnectionMap.end()){
            //发送消息
            it->second->send(js.dump());
            return;
        }
    }

    // 不在当前服务器 
    User user = _userModel.query(toId);
    if (user.getState() == "online")
    {
        _redis.publish(toId, js.dump());
        return;
    }

    //存储消息
    _offMsgModel.insert(toId,js.dump());
}

//重置业务
void ChatService::reset(){
    _userModel.resetState();
}

//添加好友业务
void ChatService::addFriend(const TcpConnectionPtr& conn,json& js,Timestamp time){
    int usrid = js["id"];
    int friendid = js["friendid"];
    _friendModel.insert(usrid,friendid);
}
//创建群组
void ChatService::createGroup(const TcpConnectionPtr& conn,json& js,Timestamp time){
    int id = js["id"];
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];
    Group group(-1,name,desc);
    if(_groupModel.createGroup(group)){
        _groupModel.addGroup(id,group.getId(),"creator");
    }
}
//加入群组
void ChatService::addGroup(const TcpConnectionPtr& conn,json& js,Timestamp time){
    int id = js["id"];
    int groupid = js["groupid"];
    _groupModel.addGroup(id,groupid,"normal");
}

//群组聊天
void ChatService::groupChat(const TcpConnectionPtr& conn,json& js,Timestamp time){
    int userid = js["id"];
    int groupid = js["groupid"];
    std::vector<int> vec = _groupModel.queryGroupUsers(userid,groupid);
    std::lock_guard<std::mutex> lock(_connMutex);
    for(int id:vec){
        auto it = _userConnectionMap.find(id);
        if(it!=_userConnectionMap.end()){
            it->second->send(js.dump());
        }
        else{
            // 查询是否在其他服务器
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        std::lock_guard<std::mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(userid);
        if (it != _userConnectionMap.end())
        {
            _userConnectionMap.erase(it);
        }
    }

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid); 

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

void ChatService::handleRedisSubscribeMessage(int userid, string msg){
    std::lock_guard<std::mutex> lock(_connMutex);
    auto it = _userConnectionMap.find(userid);
    if (it != _userConnectionMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offMsgModel.insert(userid, msg);
}